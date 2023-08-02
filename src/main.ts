import {Color, Glyph} from './lib';
import {Effect, Effects, Event, Frame, FT, ray_character} from './effect';
import {assert, flatten, int, nonnull, only, permute, range, sample, weighted} from './lib';
import {Point, Direction, Matrix, LOS, FOV, AStar, BFS, Status} from './geo';

//////////////////////////////////////////////////////////////////////////////

interface Log { line: string, menu: boolean};
interface Vision { dirty: boolean, blockers: Point[], value: Matrix<int> };

const kVisionRadius = 3;

class Board {
  private fov: FOV;
  private map: Matrix<Tile>;
  private effect: Effect;
  private entity: Entity[];
  private entityAtPos: Map<int, Entity>;
  private entityIndex: int;
  private entityVision: Map<Entity, Vision>;
  private defaultTile: Tile;
  private logs: Log[];

  constructor(size: Point, fov: int) {
    this.fov = new FOV(int(Math.max(fov, fov)));
    this.map = new Matrix(size, nonnull(kTiles['.']));
    this.effect = new Effect();
    this.entity = [];
    this.entityAtPos = new Map();
    this.entityIndex = 0;
    this.entityVision = new Map();
    this.defaultTile = nonnull(kTiles['#']);
    this.logs = [];
  }

  // Reads

  getLog(): string[] {
    return this.logs.map(x => x.line);
  }

  getSize(): Point {
    return this.map.size;
  }

  getTile(pos: Point): Tile {
    return this.map.getOrNull(pos) || this.defaultTile;
  }

  getCurrentFrame(): Frame | null {
    return this.effect.frames[0] ?? null;
  }

  getEntity(pos: Point): Entity | null {
    return this.entityAtPos.get(pos.key()) || null;
  }

  getEntities(): Entity[] {
    return this.entity;
  }

  getActiveEntity(): Entity {
    return nonnull(this.entity[this.entityIndex]);
  }

  getGlyphAt(pos: Point): Glyph {
    return this.getEntity(pos)?.glyph ?? this.getUnderlyingGlyphAt(pos);
  }

  getUnderlyingGlyphAt(pos: Point): Glyph {
    return this.getTile(pos).glyph;
  }

  getStatus(pos: Point): Status {
    if (this.getTile(pos).blocked) return Status.BLOCKED;
    if (this.entityAtPos.has(pos.key())) return Status.OCCUPIED;
    return Status.FREE;
  }

  // Writes

  setTile(pos: Point, tile: Tile): void {
    this.map.set(pos, tile);
    this.entity.forEach(x => this.dirtyVision(x));
  }

  addEffect(effect: Effect): void {
    this.effect = Effect.Parallel([this.effect, effect]);
    this.executeEffectCallbacks();
  }

  advanceEffect(): boolean {
    const result = !!this.effect.frames.shift();
    if (!result) return false;

    this.effect.events.forEach(x => x.frame--);
    this.executeEffectCallbacks();
    return true;
  }

  private executeEffectCallbacks(): void {
    while (this.executeOneEffectCallback()) {}
  }

  private executeOneEffectCallback(): boolean {
    const {events, frames} = this.effect;
    if (events.length === 0) return false;

    const event = nonnull(events[0]);
    if (frames.length > 0 && event.frame > 0) return false;

    events.shift();
    if (event.type === FT.Callback) event.callback();
    return true;
  }

  addEntity(entity: Entity): void {
    const pos = entity.pos;
    assert(this.getEntity(pos) === null);
    this.entityAtPos.set(pos.key(), entity);
    this.entity.push(entity);
  }

  advanceEntity(): void {
    charge(this.getActiveEntity());
    this.entityIndex = int((this.entityIndex + 1) % this.entity.length);
  }

  moveEntity(entity: Entity, to: Point): void {
    const key = entity.pos.key();
    assert(this.getEntity(to) === null);
    assert(this.entityAtPos.get(key) === entity);

    this.entityAtPos.delete(key);
    this.entityAtPos.set(to.key(), entity);
    entity.pos = to;
    this.dirtyVision(entity);
  }

  removeEntity(entity: Entity): void {
    if (entity.type === ET.Trainer && entity.data.player) {
      entity.glyph = entity.glyph.recolor('400');
      entity.removed = true;
      return;
    }

    const pos = entity.pos;
    assert(this.getEntity(pos) === entity);
    this.entityAtPos.delete(pos.key());

    const index = this.entity.indexOf(entity);
    assert(0 <= index && index < this.entity.length);
    this.entity.splice(index, 1);
    if (index <= this.entityIndex) this.entityIndex--;

    if (entity.type === ET.Pokemon) {
      const trainer = entity.data.self.trainer;
      if (trainer) {
        const index = trainer.data.summons.indexOf(entity);
        assert(0 <= index && index < trainer.data.summons.length);
        trainer.data.summons.splice(index, 1);
        trainer.data.pokemon.forEach(
            x => { if (x.entity === entity) x.entity = null; });
      }
    }
    entity.removed = true;
  }

  swapEntities(a: Point, b: Point): void {
    const ak = a.key();
    const bk = b.key();
    const ae = nonnull(this.entityAtPos.get(ak));
    const be = nonnull(this.entityAtPos.get(bk));
    this.entityAtPos.set(ak, be);
    this.entityAtPos.set(bk, ae);
    ae.pos = b;
    be.pos = a;
    this.dirtyVision(ae);
    this.dirtyVision(be);
  }

  log(line: string, menu?: boolean): void {
    if (menu && this.logs[this.logs.length - 1]?.menu) this.logs.pop();
    this.logs.push({line, menu: !!menu});
    if (this.logs.length > Constants.LOG_SIZE) this.logs.shift();
  }

  logAppend(line: string): void {
    const last = this.logs[this.logs.length - 1];
    if (!last) return this.log(line);
    last.line = `${last.line} ${line}`;
  }

  logIfPlayer(entity: Entity, line: string): void {
    if (entity.type === ET.Trainer && entity.data.player) this.log(line);
  }

  logMenu(line: string, done?: boolean): void {
    this.log(line, true);
    const last = this.logs[this.logs.length - 1];
    if (done && last) last.menu = false;
  }

  // Cached field-of-vision

  getBlockers(entity: Entity): Point[] {
    return this.getCachedVision(entity).blockers;
  }

  getVision(entity: Entity): Matrix<int> {
    return this.getCachedVision(entity).value;
  }

  private getCachedVision(entity: Entity): Vision {
    const vision = (() => {
      const cached = this.entityVision.get(entity);
      if (cached) return cached;
      const value = new Matrix<int>(this.map.size, -1);
      const result = {dirty: true, blockers: [], value};
      this.entityVision.set(entity, result);
      return result;
    })();

    if (vision.dirty) {
      const pos = entity.pos;
      const {blockers, value} = vision;

      const blocked = (p: Point, parent: Point | null) => {
        const q = p.add(pos);
        const cached = value.getOrNull(q);
        if (cached === null) return true;
        let tile: Tile | null = null;

        // The constants in these expressions come from Point.distanceNethack.
        // They're chosen so that, in a field of tall grass, we can only see
        // cells at a distanceNethack of <= kVisionRadius away.
        const visibility = ((): int => {
          if (!parent) return int(100 * (kVisionRadius + 1) - 95 - 46 - 25);

          tile = this.map.getOrNull(q);
          if (!tile || tile.blocked) return 0;

          const diagonal = p.x !== parent.x && p.y !== parent.y;
          const loss = tile.obscure ? 95 + (diagonal ? 46 : 0) : 0;
          const prev = value.get(parent.add(pos));
          return int(Math.max(prev - loss, 0));
        })();

        if (visibility > cached) {
          value.set(q, visibility);
          if (tile && tile.blocked) blockers.push(q);
        }
        return visibility <= 0;
      };

      value.fill(-1);
      blockers.length = 0;
      this.fov.fieldOfVision(blocked);
      vision.dirty = false;
    }

    return vision;
  }

  private dirtyVision(entity: Entity) {
    const vision = this.entityVision.get(entity);
    if (vision) vision.dirty = true;
  }
};

//////////////////////////////////////////////////////////////////////////////

const capitalize = (input: string): string => {
  const ch = input[0];
  if (!ch || ch.toUpperCase() === ch) return input;
  return `${ch.toUpperCase()}${input.slice(1)}`;
};

const describe = (entity: Entity): string => {
  if (entity.type === ET.Trainer) {
    const {name, player} = entity.data;
    return player ? 'you' : name;
  } else {
    const {species: {name: species}, trainer} = entity.data.self;
    if (!trainer) return `wild ${species}`;
    const {name, player} = trainer.data;
    return player ? species : `${name}'s ${species}`;
  }
};

const cur_hp_fraction = (entity: Entity): number => {
  return entity.type === ET.Trainer
      ? entity.data.cur_hp / Math.max(entity.data.max_hp, 1)
      : entity.data.self.cur_hp / Math.max(entity.data.self.max_hp, 1);
};

const form = (command: Command, pokemon: Pokemon): string => {
  const name = pokemon.data.self.species.name;
  switch (command.type) {
    case CT.Attack: {
      const target = command.target;
      if (target instanceof Point) {
        return `${name}, use ${command.attack.name}!`;
      }
      const target_name = target.type === ET.Pokemon
          ? target.data.self.species.name
          : target.data.name;
      return `${name}, attack ${target_name} with ${command.attack.name}!`;
    }
    case CT.Return: return `${name}, return!`;
  }
};

const shout = (board: Board, trainer: Trainer, text: string): void => {
  const {name, player} = trainer.data;
  return player ? board.logMenu(Color(text, '231'), true)
                : board.log(Color(`${name} shouts: "${text}"`, '234'));
};

//////////////////////////////////////////////////////////////////////////////

// {Action,Command,Entity,Target}Type enums:
enum AT { Attack, Idle, Move, Shout, Summon, Withdraw, WaitForInput };
enum CT { Attack, Return };
enum ET { Pokemon, Trainer };
enum TT { Attack, Summon, FarLook };

type Action =
  {type: AT.Attack, attack: Attack, target: Point} |
  {type: AT.Idle} |
  {type: AT.Move, direction: Direction} |
  {type: AT.Shout, command: Command, pokemon: Pokemon} |
  {type: AT.Summon, pokemon: PokemonEdge, target: Point} |
  {type: AT.Withdraw, pokemon: Pokemon} |
  {type: AT.WaitForInput};

type GenEffect = (board: Board, source: Point, target: Point) => Effect;

interface Result { success: boolean, moves: number, turns: number };

type Command =
  {type: CT.Attack, attack: Attack, target: Entity | Point} |
  {type: CT.Return};

interface Attack { name: string, range: int, damage: int, effect: GenEffect };

type PokemonSpeciesWithAttacks = PokemonSpeciesData & {attacks: Attack[]};

interface PokemonSpeciesData {
  name: string,
  glyph: Glyph,
  hp: int,
  speed: number,
};

interface PokemonIndividualData {
  attacks: Attack[],
  species: PokemonSpeciesData,
  trainer: Trainer | null,
  cur_hp: int,
  max_hp: int,
};

interface PokemonData {
  commands: Command[],
  self: PokemonIndividualData,
};

interface PokemonEdge {
  entity: Pokemon | null,
  self: PokemonIndividualData,
};

interface TrainerData {
  input: Action | null,
  name: string,
  player: boolean,
  pokemon: PokemonEdge[],
  summons: Pokemon[],
  cur_hp: int,
  max_hp: int,
};

interface EntityData {
  glyph: Glyph,
  speed: number,
  removed: boolean,
  move_timer: int,
  turn_timer: int,
  dir: Direction,
  pos: Point,
};

type Entity =
  {type: ET.Pokemon, data: PokemonData} & EntityData |
  {type: ET.Trainer, data: TrainerData} & EntityData;

type Pokemon = Entity & {type: ET.Pokemon};
type Trainer = Entity & {type: ET.Trainer};

const charge = (entity: Entity): void => {
  const charge = int(Math.round(Constants.TURN_TIMER * entity.speed));
  if (entity.move_timer > 0) entity.move_timer -= charge;
  if (entity.turn_timer > 0) entity.turn_timer -= charge;
};

const moveReady = (entity: Entity): boolean => {
  return entity.move_timer <= 0;
};

const turnReady = (entity: Entity): boolean => {
  return entity.turn_timer <= 0;
};

const getTrainer = (entity: Entity): Trainer | null => {
  return entity.type === ET.Trainer ? entity : entity.data.self.trainer;
};

const wait = (entity: Entity, moves: number, turns: number): void => {
  entity.move_timer += int(Math.round(Constants.MOVE_TIMER * moves));
  entity.turn_timer += int(Math.round(Constants.TURN_TIMER * turns));
};

const makePokemon = (pos: Point, self: PokemonIndividualData): Pokemon => {
  const {glyph, speed} = self.species;
  const data = {commands: [], self};
  return {type: ET.Pokemon, data, dir: Direction.s, pos, glyph, speed,
          removed: false, move_timer: 0, turn_timer: 0};
};

const makeTrainer = (pos: Point, player: boolean): Trainer => {
  const glyph = new Glyph('@');
  const speed = Constants.TRAINER_SPEED;
  const hp = Constants.TRAINER_HP;
  const data = {
    input: null,
    name: '',
    player,
    pokemon: [],
    summons: [],
    cur_hp: hp,
    max_hp: hp,
  };
  return {type: ET.Trainer, data, dir: Direction.s, pos, glyph, speed,
          removed: false, move_timer: 0, turn_timer: 0};
};

const hasLineOfSight =
    (board: Board, source: Point, target: Point, range: int): boolean => {
  if (source.distanceNethack(target) > range) return false;

  // See the calculation in getVision, and the constants in distanceNethack.
  let vision = int(100 * (kVisionRadius + 1) - 95 - 46 - 25);

  const line = LOS(source, target);
  const last = line.length - 1;
  return line.every((point, i) => {
    if (i === 0) return true;
    if (i === last && vision > 0) return true;

    if (vision <= 0) return false;
    if (board.getStatus(point) !== Status.FREE) return false;

    // Run the vision attenuation calculation only along the line of sight.
    const prev = nonnull(line[i - 1]);
    const tile = board.getTile(point);
    const diagonal = point.x !== prev.x && point.y !== prev.y;
    const loss = tile.obscure ? 95 + (diagonal ? 46 : 0) : 0;
    vision = int(vision - loss);
    return true;
  });
};

const followCommands =
    (board: Board, entity: Entity, commands: Command[]): Action => {
  const kBFSLimit = 6;
  const source = entity.pos;

  const path_to_target =
      (range: int, target: Point, valid: (p: Point) => boolean): Action => {
    const check = board.getStatus.bind(board);
    const dirs = BFS(source, valid, kBFSLimit, check);
    const options = dirs.map(x => source.add(x));
    if (valid(source)) options.push(source);

    if (options.length > 0) {
      const scores = options.map(
          x => Math.abs(x.distanceNethack(target) - range));
      const best = Math.min.apply(null, scores);
      const move = sample(options.filter((_, i) => scores[i] === best));
      if (move.equal(source)) return {type: AT.Idle};
      return {type: AT.Move, direction: Direction.assert(move.sub(source))};
    }

    const path = AStar(source, target, check);
    const direction = path
      ? Direction.assert(nonnull(path[0]).sub(source))
      : sample(Direction.all);
    return {type: AT.Move, direction};
  };

  const command = nonnull(commands[0]);
  switch (command.type) {
    case CT.Attack: {
      const {attack, target: ent_or_pt} = command;
      if (!(ent_or_pt instanceof Point) && ent_or_pt.removed) {
        commands.shift();
        return plan(board, entity);
      }

      // We can attack a target if we have line-of-sight to its position.
      const target = ent_or_pt instanceof Point ? ent_or_pt : ent_or_pt.pos;
      const range = attack.range;
      const valid = (p: Point) => hasLineOfSight(board, p, target, range);
      if (moveReady(entity) && hasLineOfSight(board, source, target, range)) {
        commands.shift();
        return {type: AT.Attack, attack, target};
      }
      return path_to_target(range, target, valid);
    }
    case CT.Return: {
      const trainer = entity.type === ET.Pokemon ? entity.data.self.trainer : null;
      if (!trainer || trainer.removed) {
        commands.shift();
        return plan(board, entity);
      }

      // A trainer can withdraw us if they have line-of-sight to our position.
      const range = Constants.SUMMON_RANGE;
      const valid = (p: Point) => hasLineOfSight(board, trainer.pos, p, range);
      return path_to_target(range, trainer.pos, valid);
    }
  }
};

const checkFollowerSquare = (board: Board, leader: Entity, pos: Point,
                             ignoreOccupant?: boolean): boolean => {
  const status = board.getStatus(pos);
  const free = status === Status.FREE ||
               (ignoreOccupant && status === Status.OCCUPIED);
  if (!free) return false;

  const source = leader.pos;
  const dn = source.distanceNethack(pos);
  if (dn > 2) return false;
  if (dn < 2) return true;

  const vision = board.getVision(leader);
  return vision.getOrNull(source) === vision.getOrNull(pos);
};

const defendSquare = (board: Board, start: Point, trainer: Trainer): Point | null => {
  const rivals = findRivalPokemon(board, trainer);
  if (rivals.length === 0) return null;

  const okay = (p: Point) => {
    const other = board.getEntity(p);
    if (other && getTrainer(other) === trainer) return true;
    return !board.getTile(p).blocked;
  };

  const scores: Map<int, number> = new Map();
  for (const rival of rivals) {
    const marked: Set<int> = new Set();
    const los = LOS(rival.pos, trainer.pos);
    const diff = rival.pos.sub(trainer.pos);

    const shift_a = Math.abs(diff.x) > Math.abs(diff.y)
        ? new Point(0, int(Math.sign(diff.y) || 1))
        : new Point(int(Math.sign(diff.x) || 1), 0);
    const shift_b = Point.origin.sub(shift_a);

    const shifts: [Point, number][] =
        [[Point.origin, 64], [shift_a, 8], [shift_b, 1]];
    for (const [shift, score] of shifts) {
      let blocked = false;
      for (const element of los) {
        const point = element.add(shift);
        const entity = point.equal(start) ? null : board.getEntity(point);
        if (entity && entity !== rival && entity !== trainer &&
            getTrainer(entity) === trainer) {
          blocked = true;
        }
      }
      if (!blocked) {
        for (const element of los) {
          const delta = element.add(shift).sub(trainer.pos);
          if (Math.abs(delta.x) > 2 || Math.abs(delta.y) > 2) continue;

          const key = delta.key();
          const size = marked.size;
          marked.add(key);
          if (marked.size === size) continue;
          scores.set(key, (scores.get(key) ?? 0) + score);
        }
      }
    }
  }

  let best_score = -Infinity;
  let best_point: Point | null = null;
  for (let x = -2; x <= 2; x++) {
    for (let y = -2; y <= 2; y++) {
      if (x === 0 && y === 0) continue;

      const offset = new Point(int(x), int(y));
      const point = trainer.pos.add(offset);
      const ignore = point.equal(start);
      if (!checkFollowerSquare(board, trainer, point, ignore)) continue;

      let score = scores.get(offset.key()) ?? -Infinity;
      score += 0.0625 * offset.distanceSquared(Point.origin);
      if (score > best_score) {
        best_score = score;
        best_point = point;
      }
    }
  }
  return best_point;
};

const defendLeader = (board: Board, pokemon: Pokemon): Action | null => {
  const trainer = pokemon.data.self.trainer;
  if (!trainer) return null;

  const check = board.getStatus.bind(board);
  const target = defendSquare(board, pokemon.pos, trainer);
  if (!target) return null;

  if (target.equal(pokemon.pos)) return {type: AT.Idle};
  const path = AStar(pokemon.pos, target, check);
  if (!(path && path.length > 0)) return null;

  const direction = Direction.assert(nonnull(path[0]).sub(pokemon.pos));
  return {type: AT.Move, direction};
};

const followLeader = (board: Board, entity: Entity, leader: Entity): Action => {
  const [ep, tp] = [entity.pos, leader.pos];
  const okay = (p: Point) => checkFollowerSquare(board, leader, p, p.equal(ep));

  if (ep.distanceNethack(tp) <= 3) {
    const moves: [int, Direction][] =
      Direction.all.filter(x => okay(ep.add(x))).map(x => [1, x]);
    if (okay(ep)) moves.push([16, Direction.none]);
    if (moves.length) return {type: AT.Move, direction: weighted(moves)};
  }

  const check = board.getStatus.bind(board);
  const path = AStar(ep, tp, check);
  const direction = path && path.length > 0
    ? Direction.assert(nonnull(path[0]).sub(ep))
    : sample(Direction.all);
  return {type: AT.Move, direction};
};

const findRivalPokemon = (board: Board, trainer: Trainer): Pokemon[] => {
  const result: Pokemon[] = [];
  const source = trainer.pos;
  const vision = board.getVision(trainer);

  board.getEntities().forEach(entity => {
    if (entity.type !== ET.Pokemon) return;
    if (entity.data.self.trainer === trainer) return;
    if ((vision.getOrNull(entity.pos) ?? -1) < 0) return;
    result.push(entity);
  });

  result.sort((entity_a, entity_b): number => {
    const a = entity_a.pos, b = entity_b.pos;
    const base = a.distanceSquared(source) - b.distanceSquared(source);
    const xopt = a.x - b.x, yopt = a.y - b.y;
    return base || xopt || yopt;
  });
  return result.slice(0, 16);
};

const findRivals = (board: Board, entity: Entity): Entity[] => {
  const trainer = getTrainer(entity);
  const all = board.getEntities().filter(x => getTrainer(x) !== trainer);
  return all.filter(x => hasLineOfSight(board, entity.pos, x.pos, 12));
};

const plan = (board: Board, entity: Entity): Action => {
  switch (entity.type) {
    case ET.Pokemon: {
      const {commands, self: {attacks, trainer}} = entity.data;
      if (commands.length > 0) return followCommands(board, entity, commands);

      const ready = !moveReady(entity);
      const defendEarly = ready ? defendLeader(board, entity) : null;
      if (defendEarly) return defendEarly;

      const rivals = findRivals(board, entity);
      if (rivals.length > 0 && attacks.length > 0) {
        const target = sample(rivals).pos;
        const attack = sample(attacks);
        const commands: Command[] = [{type: CT.Attack, attack, target}];
        return followCommands(board, entity, commands);
      }

      const defendLate = !ready ? defendLeader(board, entity) : null;
      if (defendLate) return defendLate;

      if (trainer) return followLeader(board, entity, trainer);
      return {type: AT.Move, direction: sample(Direction.all)};
    }
    case ET.Trainer: {
      const {input, player} = entity.data;
      for (const summon of entity.data.summons) {
        const range = Constants.SUMMON_RANGE;
        const withdraw = summon.data.commands[0]?.type === CT.Return;
        if (withdraw && hasLineOfSight(board, entity.pos, summon.pos, range)) {
          summon.data.commands.shift();
          return {type: AT.Withdraw, pokemon: summon};
        }
      }
      if (!player) return {type: AT.Idle};
      if (!input) return {type: AT.WaitForInput};
      entity.data.input = null;
      return input;
    }
  }
};

const kSuccess: Result = {success: true,  moves: 0, turns: 1};
const kFailure: Result = {success: false, moves: 0, turns: 1};

const act = (anim: Anim, board: Board, entity: Entity, action: Action): Result => {
  switch (action.type) {
    case AT.Attack: {
      const source = entity.pos;
      const {attack, target} = action;
      if (!hasLineOfSight(board, source, target, attack.range)) return kFailure;

      // success
      const user = capitalize(describe(entity));
      const target_entity = board.getEntity(target);
      let callback: () => void = () => {};

      if (target_entity === null) {
        board.log(`${user} used ${attack.name}!`);
      } else if (target_entity.type === ET.Trainer) {
        const target_name = describe(target_entity);
        board.log(`${user} attacked ${target_name} with ${attack.name}!`);

        callback = () => {
          const data = target_entity.data;
          animateDamage(anim, target_entity);
          data.cur_hp = int(Math.max(data.cur_hp - 1, 0));

          board.addEffect(ApplyDamage(board, target, () => {
            if (data.cur_hp) return;
            board.logAppend(`${capitalize(target_name)} blacked out!`);
            board.removeEntity(target_entity);
          }));
        };
      } else {
        const target_name = describe(target_entity);
        board.log(`${user} attacked ${target_name} with ${attack.name}!`);

        callback = () => {
          const data = target_entity.data.self;
          const damage = int(Math.random() * attack.damage);
          animateDamage(anim, target_entity);
          data.cur_hp = int(Math.max(data.cur_hp - damage, 0));

          board.addEffect(ApplyDamage(board, target, () => {
            if (data.cur_hp) return;
            board.logAppend(`${capitalize(target_name)} fainted!`);
            board.removeEntity(target_entity);
          }));
        };
      }

      const effect = attack.effect(board, entity.pos, target);
      board.addEffect(ApplyAttack(board, effect, target, callback));
      return {success: true, moves: 1, turns: 1};
    }
    case AT.Idle: return kSuccess;
    case AT.Move: {
      const pos = entity.pos.add(action.direction);
      if (pos.equal(entity.pos)) return kSuccess;
      if (board.getTile(pos).blocked) return kFailure;
      const other = board.getEntity(pos);
      if (other) {
        if (getTrainer(other) !== entity) return kFailure;
        board.swapEntities(entity.pos, pos);
        board.logIfPlayer(entity, `You swap places with ${describe(other)}.`);
        entity.dir = action.direction;
        return kSuccess;
      }

      // success
      board.moveEntity(entity, pos);
      entity.dir = action.direction;
      return kSuccess;
    }
    case AT.Shout: {
      const pokemon = action.pokemon;
      if (getTrainer(pokemon) !== entity) return kFailure;

      // success
      const range = Constants.SUMMON_RANGE;
      const [source, target] = [entity.pos, pokemon.pos];
      if (action.command.type === CT.Return &&
          hasLineOfSight(board, source, target, range)) {
        board.removeEntity(pokemon);
        board.addEffect(Effects.Withdraw(source, target, pokemon.glyph));
      } else {
        pokemon.data.commands.push(action.command);
      }
      shout(board, entity, form(action.command, pokemon));
      return kSuccess;
    }
    case AT.Summon: {
      const {pokemon, target} = action;
      if (entity.type !== ET.Trainer) return kFailure;
      if (pokemon.entity || !pokemon.self.cur_hp) return kFailure;
      if (entity.data.summons.length >= kSummonedKeys.length) return kFailure;
      if (board.getStatus(target) !== Status.FREE) return kFailure;

      // success
      const glyph = board.getTile(target).glyph;
      const summoned = makePokemon(target, pokemon.self);
      entity.data.summons.push(summoned);
      pokemon.entity = summoned;
      board.addEntity(summoned);
      board.addEffect(Effects.Summon(entity.pos, target, glyph));
      shout(board, entity, `Go! ${pokemon.self.species.name}!`);
      return kSuccess;
    }
    case AT.Withdraw: {
      const pokemon = action.pokemon;
      const range = Constants.SUMMON_RANGE;
      const [source, target] = [entity.pos, pokemon.pos];
      if (getTrainer(pokemon) !== entity) return kFailure;
      if (!hasLineOfSight(board, source, target, range)) return kFailure;

      // success
      board.removeEntity(pokemon);
      board.addEffect(Effects.Withdraw(source, target, pokemon.glyph));
      const name = pokemon.data.self.species.name;
      const text = entity.data.player
          ? `You withdraw ${name}.`
          : `${entity.data.name} withdraws ${name}.`;
      board.log(Color(text, '234'));
      return kSuccess;
    }
    case AT.WaitForInput: return kFailure;
  }
};

//////////////////////////////////////////////////////////////////////////////

const animateDamage = (anim: Anim, entity: Entity): void => {
  const hp = getAnimatedHP(anim, entity);
  anim.damage.set(entity, {hp, frame: 0});
};

const getAnimatedHP = (anim: Anim, entity: Entity): number => {
  const target = cur_hp_fraction(entity);
  const entry = anim.damage.get(entity);
  if (!entry) return target;

  const source = entry.hp;
  const f = (entry.frame + 1) / (Constants.DAMAGE_FRAMES + 2);
  const g = 1 - (1 - f) * (1 - f) * (1 - f);
  return target * g + source * (1 - g);
};

const updateAnimation = (anim: Anim): void => {
  const entries = Array.from(anim.damage.entries());
  for (const [entity, frame] of entries) {
    const done = (++frame.frame) >= Constants.DAMAGE_FRAMES;
    if (done) anim.damage.delete(entity);
  }
};

//////////////////////////////////////////////////////////////////////////////

const initSummonTarget =
    (state: State, pokemon: PokemonEdge, range: int): SummonTarget => {
  const player = state.player;
  const source = state.player.pos;
  const data: SummonTargetData = {type: TT.Summon, pokemon, range};
  const result = initTarget(data, source, source);

  const defend = defendSquare(state.board, source, player);
  if (defend) {
    const line = LOS(source, defend).slice(1).reverse();
    for (const point of line) {
      updateSummonTarget(state, result, point);
      if (result.error.length === 0) return result;
    }
  }

  const okay = (pos: Point) => {
    if (!checkFollowerSquare(state.board, player, pos)) return false;
    updateSummonTarget(state, result, pos);
    return result.error.length === 0;
  };

  const options: Point[] = [];
  const best = source.add(player.dir.scale(2));
  const next = source.add(player.dir.scale(1));
  if (okay(best)) return result;
  if (okay(next)) return result;

  for (let dx = -2; dx <= 2; dx++) {
    for (let dy = -2; dy <= 2; dy++) {
      const pos = new Point(int(source.x + dx), int(source.y + dy));
      if (okay(pos)) options.push(pos);
    }
  }

  options.sort((a, b) => best.distanceSquared(a) - best.distanceSquared(b));
  updateSummonTarget(state, result, options[0] || source);
  return result;
};

//////////////////////////////////////////////////////////////////////////////

const updateAttackTarget =
    (state: State, target: AttackTarget, update: Point): void => {
  const {board, player} = state;
  const vision = board.getVision(player);
  const unseen = (vision.getOrNull(update) ?? -1) < 0;
  const entity = unseen ? null : board.getEntity(update);
  const okay = !unseen && !(entity && getTrainer(entity) === player);
  const los = LOS(target.source, update);

  target.error = okay ? '' : unseen ? `You can't see a clear path there.`
                                    : `That target is friendly.`;
  target.frame = 0;
  target.path = los.slice(1).map(x => [x, okay] as [Point, boolean]);
  target.target = update;
};

const updateSummonTarget =
    (state: State, target: SummonTarget, update: Point): void => {
  const {board, player} = state;
  const los = LOS(player.pos, update);
  const start = los.length > 1 ? 1 : 0;
  const range = target.data.range;

  target.error = '';
  target.path.length = 0;
  for (let i = start; i < los.length; i++) {
    const point = los[i]!;
    if (target.error.length === 0) {
      if (board.getStatus(point) !== Status.FREE) {
        target.error = `There's something in the way.`;
      } else if (range !== null && player.pos.distanceL2(point) > range - 0.5) {
        target.error = `You can't throw that far.`;
      } else if ((board.getVision(player).getOrNull(point) ?? -1) < 0) {
        target.error = `You can't see a clear path there.`;
      }
    }
    target.path.push([point, target.error.length === 0]);
  }

  target.frame = 0;
  target.target = update;
};

const updateFarLookTarget =
    (state: State, target: FarLookTarget, update: Point): void => {
  const {board, player} = state;
  const vision = board.getVision(player);
  const okay = (x: Point) => (vision.getOrNull(x) ?? -1) >= 0;
  const los = LOS(target.source, update);

  target.error = okay(update) ? '' : `You can't see a clear path there.`;
  target.frame = 0;
  target.path = los.slice(1).map(x => [x, okay(x)] as [Point, boolean]);
  target.target = update;

  let last_okay_point = -1;
  for (let i = 0; i < target.path.length; i++) {
    if (target.path[i]![1]) last_okay_point = i;
  }
  for (let i = 0; i < last_okay_point; i++) {
    target.path[i]![1] = true;
  }
};

//////////////////////////////////////////////////////////////////////////////

const animateTarget = (state: State, target: Target): void => {
  target.frame = int((target.frame + 1) % Constants.TARGET_FRAMES);
};

const initTarget = <T extends TargetData>
    (data: T, source: Point, target: Point): Target & {data: T} => {
  return {data, error: '', frame: 0, path: [], source, target};
};

const updateTarget = (state: State, target: Target, update: Point): void => {
  switch (target.data.type) {
    case TT.Attack:  return updateAttackTarget(state,  target as AttackTarget,  update);
    case TT.Summon:  return updateSummonTarget(state,  target as SummonTarget,  update);
    case TT.FarLook: return updateFarLookTarget(state, target as FarLookTarget, update);
  }
};

const selectValidTarget = (state: State, selected: Target): null => {
  switch (selected.data.type) {
    case TT.Summon: {
      const {data: {pokemon}, target} = selected;
      state.player.data.input = {type: AT.Summon, pokemon, target};
      return null;
    }
    case TT.Attack: {
      const {attack, summon: pokemon} = selected.data;
      const entity = state.board.getEntity(selected.target);
      const target = entity ? entity : selected.target;
      const command = {type: CT.Attack, attack, target};
      state.player.data.input = {type: AT.Shout, command, pokemon};
      return null;
    }
    case TT.FarLook: {
      return null;
    }
  }
};

//////////////////////////////////////////////////////////////////////////////

const ApplyAttack = (board: Board, effect: Effect, target: Point,
                     callback: () => void): Effect => {
  const hit = effect.events.filter(x => x.type === FT.Hit)[0];
  if (!hit) return effect;

  const event: Event = {frame: hit.frame, type: FT.Callback, callback};
  effect.mutAddEvent(event);
  return effect;
};

const ApplyDamage = (board: Board, target: Point, callback: () => void): Effect => {
  const entity = board.getEntity(target);
  if (!entity) return new Effect();

  const particle = {point: target, glyph: entity.glyph.recolor('black', '400')};
  const effect = Effect.Constant(particle, Constants.DAMAGE_FRAMES);

  const frame = int(effect.frames.length);
  const event: Event = {frame, type: FT.Callback, callback};
  effect.mutAddEvent(event);
  return effect;
};

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  LOG_SIZE:      int(4),
  MAP_SIZE_X:    int(43),
  MAP_SIZE_Y:    int(43),
  FOV_RADIUS:    int(21),
  WORLD_SIZE:    int(100),
  STATUS_SIZE:   int(30),
  CHOICE_SIZE:   int(42),
  MOVE_TIMER:    int(960),
  TURN_TIMER:    int(120),
  SUMMON_RANGE:  int(12),
  TRAINER_HP:    int(8),
  TRAINER_SPEED: 1/10,

  // Animation frame rates:
  FRAME_RATE:    int(60),
  TARGET_FRAMES: int(20),
  DAMAGE_FRAMES: int(8),
};

interface Tile {
  readonly blocked: boolean,
  readonly obscure: boolean,
  readonly glyph: Glyph,
  readonly description: string,
};

const kPlayerKey = 'a';
const kReturnKey = 'r';
const kSummonedKeys = 'sdf';
const kAttackKeys = `${'a'}sdf`;
const kPartyKeys = `${'a'}sdfgh`;
const kDirectionKeys = 'kulnjbhy';
const kAlphabetKeys = 'abcdefghijklmnopqrstuvwxyz';

const kTiles: {[ch: string]: Tile} = {
  '.': {blocked: false, obscure: false, glyph: new Glyph('.'),        description: 'ground'},
  '"': {blocked: false, obscure: true,  glyph: new Glyph('"', '231'), description: 'tall grass'},
  '#': {blocked: true,  obscure: true,  glyph: new Glyph('#', '010'), description: 'a tree'},
};

const kAttacks: Attack[] = [
  {name: 'Ember',    range: 12, damage: int(40), effect: Effects.Ember},
  {name: 'Ice Beam', range: 12, damage: int(60), effect: Effects.IceBeam},
  {name: 'Blizzard', range: 12, damage: int(80), effect: Effects.Blizzard},
  {name: 'Headbutt', range: 8,  damage: int(80), effect: Effects.Headbutt},
  {name: 'Tackle',   range: 4,  damage: int(40), effect: Effects.Headbutt},
];

const species = (name: string, hp: int, speed: number, attack_names: string[],
                 glyph: Glyph): PokemonSpeciesWithAttacks => {
  attack_names = attack_names.slice();
  attack_names.push('Tackle');
  const attacks = attack_names.map(x=> only(kAttacks.filter(y => y.name === x)));
  return {name, glyph, hp, speed, attacks};
};

const kPokemon: PokemonSpeciesWithAttacks[] = [
  species('Bulbasaur',  int(90), 1/6, [],           new Glyph('B', '020')),
  species('Charmander', int(80), 1/5, ['Ember'],    new Glyph('C', '410')),
  species('Squirtle',   int(70), 1/4, ['Ice Beam'], new Glyph('S', '234')),
  species('Eevee',      int(80), 1/5, ['Headbutt'], new Glyph('E', '420')),
  species('Pikachu',    int(60), 1/4, [],           new Glyph('P', '440')),
  species('Rattata',    int(60), 1/4, ['Headbutt'], new Glyph('R')),
  species('Pidgey',     int(30), 1/3, [],           new Glyph('P')),
];

type Input = string;

interface Anim {
  damage: Map<Entity, {hp: number, frame: int}>;
};

interface State {
  anim: Anim,
  board: Board,
  player: Trainer,
  choice: Choice | null,
  target: Target | null,
  menu: Menu | null,
};

interface Choice {
  index: int,
};

interface Menu {
  index: int,
  summon: int,
};

interface AttackTargetData {
  type: TT.Attack,
  attack: Attack,
  summon: Pokemon,
};

interface SummonTargetData {
  type: TT.Summon,
  pokemon: PokemonEdge,
  range: int,
};

interface FarLookTargetData {
  type: TT.FarLook,
};

type TargetData = AttackTargetData | SummonTargetData | FarLookTargetData;
type AttackTarget  = Target & {data: AttackTargetData};
type SummonTarget  = Target & {data: SummonTargetData};
type FarLookTarget = Target & {data: FarLookTargetData};

const isAttackTarget  = (x: Target): x is AttackTarget => x.data.type === TT.Attack;
const isSummonTarget  = (x: Target): x is SummonTarget => x.data.type === TT.Summon;
const isFarLookTarget = (x: Target): x is SummonTarget => x.data.type === TT.FarLook;

interface Target {
  data: TargetData,
  error: string,
  frame: int,
  path: [Point, boolean][],
  source: Point,
  target: Point,
};

interface PokemonPublicState {
  damaged: boolean,
  species: PokemonSpeciesData,
  hp: number,
  pp: number,
  pos: Point | null,
};

const getPartyPokemonPublicState =
    (self: PokemonIndividualData): PokemonPublicState => {
  const hp = self.cur_hp / Math.max(self.max_hp, 1);
  return {damaged: false, species: self.species, hp, pp: 1, pos: null};
};

const getPokemonPublicState =
    (anim: Anim, pokemon: Pokemon): PokemonPublicState => {
  const result = getPartyPokemonPublicState(pokemon.data.self);
  const bp = (pokemon.move_timer ?? 0) / Constants.MOVE_TIMER;
  result.damaged = anim.damage.has(pokemon);
  result.hp = getAnimatedHP(anim, pokemon);
  result.pp = 1 - Math.max(0, Math.min(bp, 1));
  result.pos = pokemon.pos;
  return result;
};

const outsideMap = (state: State, point: Point): boolean => {
  const delta = point.sub(state.player.pos);
  const limit_x = Math.floor((Constants.MAP_SIZE_X - 1) / 2);
  const limit_y = Math.floor((Constants.MAP_SIZE_Y - 1) / 2);
  return Math.abs(delta.x) > limit_x || Math.abs(delta.y) > limit_y;
};

const processInput = (state: State, input: Input): void => {
  const {board, player, choice, target, menu} = state;
  const enter = input === 'enter' || input === '.';
  const escape = input === 'escape';

  if (choice) {
    const count = player.data.pokemon.length;
    const direction = Direction.all[kDirectionKeys.indexOf(input)];
    const chosen = enter ? choice.index : int(kPartyKeys.indexOf(input));
    if (direction && direction.x === 0) {
      choice.index = int(choice.index + direction.y);
      if (choice.index >= count) choice.index = 0;
      if (choice.index < 0) choice.index = int(Math.max(count - 1, 0));
    } else if (chosen >= 0) {
      const pokemon = player.data.pokemon[chosen];
      const name = pokemon?.self.species.name;
      if (!pokemon) return;
      if (pokemon.entity) {
        board.logMenu(Color(`${name} is already out!`, '422'));
      } else if (!pokemon.self.cur_hp) {
        board.logMenu(Color(`${name} has no strength left!`, '422'));
      } else {
        const range = Constants.SUMMON_RANGE;
        board.logMenu(Color(`Choose where to send out ${name}:`, '234'));
        state.target = initSummonTarget(state, pokemon, range);
        state.choice = null;
      }
    } else if (escape) {
      board.logMenu(Color('Canceled.', '234'));
      state.choice = null;
    }
    return;
  }

  const getUpdatedTarget = (target: Point): Point | null => {
    if (input === 'tab' || input === 'S-tab') {
      const rivals = findRivalPokemon(board, player);
      if (rivals.length === 0) return null;

      const current = board.getEntity(target);
      const pokemon = current && current.type === ET.Pokemon ? current : null;
      const start = pokemon ? rivals.indexOf(pokemon) : -1;

      const n = rivals.length;
      const t = input === 'tab';
      const index = start >= 0 ? (start + n + (t ? 1 : -1)) % n : t ? 0 : n - 1;
      return nonnull(rivals[index]).pos;
    }

    const lower = input.startsWith('S-') ? input.substr(2) : input;
    const direction = Direction.all[kDirectionKeys.indexOf(lower)];
    if (!direction) return null;

    const scale = input === lower ? 1 : 4;
    for (let i = 0; i < scale; i++) {
      const next_target = target.add(direction);
      if (outsideMap(state, next_target)) break;
      target = next_target;
    }
    return target;
  };

  if (target) {
    const update = getUpdatedTarget(target.target);
    if (update && !update.equal(target.target)) {
      updateTarget(state, target, update);
    } else if (enter) {
      if (target.error.length > 0) {
        board.logMenu(Color(target.error, '422'));
      } else {
        state.target = selectValidTarget(state, target);
      }
    } else if (escape) {
      board.logMenu(Color('Canceled.', '234'));
      state.target = null;
    }
    return;
  }

  if (menu) {
    assert(0 <= menu.summon);
    assert(menu.summon < player.data.summons.length);

    const summon = nonnull(player.data.summons[menu.summon]);
    const name = summon.data.self.species.name;
    const keys = kAttackKeys + kReturnKey;
    const count = keys.length;

    const valid = (index: int): boolean => {
      return index === count - 1 || !!summon.data.self.attacks[index];
    };

    const direction = Direction.all[kDirectionKeys.indexOf(input)];
    const chosen = enter ? menu.index : int(keys.indexOf(input));
    if (direction && direction.x === 0) {
      do {
        menu.index = int(menu.index + direction.y);
        if (menu.index >= count) menu.index = 0;
        if (menu.index < 0) menu.index = int(Math.max(count - 1, 0));
      } while (!valid(menu.index));
    } else if (chosen >= 0) {
      if (chosen === count - 1) {
        const command = {type: CT.Return as CT.Return};
        player.data.input = {type: AT.Shout, command, pokemon: summon};
        state.menu = null;
      } else {
        const attack = summon.data.self.attacks[chosen];
        if (!attack) {
          board.logMenu(Color(`${name} does not have that attack.`, '422'));
        } else {
          const source = summon.pos;
          const target = findRivalPokemon(board, player)[0]?.pos || source;
          const data: AttackTargetData = {type: TT.Attack, attack, summon};
          const init = state.target = initTarget(data, source, target);
          updateAttackTarget(state, init, target);
          state.menu = null;
        }
      }
    } else if (escape) {
      board.logMenu(Color('Canceled.', '234'));
      state.menu = null;
    }
    return;
  }

  if (player.data.input !== null) return;

  if (input === 'x') {
    const source = player.pos;
    const target = findRivalPokemon(board, player)[0]?.pos || source;
    const data: FarLookTargetData = {type: TT.FarLook};
    const init = state.target = initTarget(data, source, target);
    updateFarLookTarget(state, init, target);
    board.logMenu(Color(`Use the movement keys to examine a location:`, '234'));
    return;
  }

  const summoned = int(kSummonedKeys.indexOf(input));
  if (summoned >= player.data.summons.length) {
    state.choice = {index: 0};
    board.logMenu(Color('Choose a Pokemon to send out with J/K:', '234'));
  } else if (summoned >= 0) {
    state.menu = {index: 0, summon: summoned};
    board.logMenu(Color('Choose a command with J/K:', '234'));
  }

  const direction = Direction.all[kDirectionKeys.indexOf(input)];
  if (direction) player.data.input = {type: AT.Move, direction};
  if (input === '.') player.data.input = {type: AT.Idle}
};

const initBoard = (): Board => {
  const base = Constants.WORLD_SIZE;
  const size = new Point(base, base);
  const board = new Board(size, Constants.FOV_RADIUS);

  const automata = (): Matrix<boolean> => {
    let result = new Matrix(size, false);
    for (let x = int(0); x < size.x; x++) {
      result.set(new Point(x, 0), true);
      result.set(new Point(x, int(size.y - 1)), true);
    }
    for (let y = int(0); y < size.x; y++) {
      result.set(new Point(0, y), true);
      result.set(new Point(int(size.x - 1), y), true);
    }

    for (let x = int(0); x < size.x; x++) {
      for (let y = int(0); y < size.y; y++) {
        if (Math.random() < 0.45) result.set(new Point(x, y), true);
      }
    }

    for (let i = 0; i < 3; i++) {
      let next = new Matrix(size, false);
      for (let x = int(0); x < size.x; x++) {
        for (let y = int(0); y < size.y; y++) {
          const point = new Point(x, y);
          next.set(point, result.get(point));
        }
      }

      for (let x = int(1); x < size.x - 1; x++) {
        for (let y = int(1); y < size.y - 1; y++) {
          let [adj1, adj2] = [0, 0];
          for (let dx = -2; dx <= 2; dx++) {
            for (let dy = -2; dy <= 2; dy++) {
              if (dx === 0 && dy === 0) continue;
              if (Math.min(Math.abs(dx), Math.abs(dy)) === 2) continue;
              const ax = int(x + dx), ay = int(y + dy);
              const test = result.getOrNull(new Point(ax, ay));
              if (test === false) continue;
              const distance = Math.max(Math.abs(dx), Math.abs(dy));
              if (distance <= 1) adj1++;
              if (distance <= 2) adj2++;
            }
          }

          const blocked = adj1 >= 5 || (i < 2 && adj2 <= 1);
          next.set(new Point(x, y), blocked);
        }
      }

      result = next;
    }

    return result;
  };

  const walls = automata();
  const grass = automata();
  const wt = nonnull(kTiles['#']);
  const gt = nonnull(kTiles['"']);
  for (let x = int(0); x < size.x; x++) {
    for (let y = int(0); y < size.y; y++) {
      const point = new Point(x, y);
      if (walls.get(point)) {
        board.setTile(point, wt);
      } else if (grass.get(point)) {
        board.setTile(point, gt);
      }
    }
  }

  return board;
};

const initState = (): State => {
  const start = int(Math.floor((Constants.WORLD_SIZE + 1) / 2));
  const point = new Point(start, start);
  const board = (() => {
    while (true) {
      const board = initBoard();
      if (!board.getTile(point).blocked) return board;
    }
  })();
  const player = makeTrainer(point, true);
  board.addEntity(player);

  const pokemon = (x: PokemonSpeciesWithAttacks,
                   trainer: Trainer | null): PokemonIndividualData => {
    const {attacks, glyph, hp, name, speed} = x;
    const species = {name, glyph, hp, speed};
    return {attacks, species, trainer, cur_hp: hp, max_hp: hp};
  };

  const n = Math.min(kPartyKeys.length, 5);
  kPokemon.slice(0, n).forEach(species => {
    const self = pokemon(species, player);
    player.data.pokemon.push({entity: null, self});
  });

  for (let i = 0; i < 20; i++) {
    const pos = (() => {
      for (let i = 0; i < 100; i++) {
        const x = int(Math.floor(Math.random() * Constants.WORLD_SIZE));
        const y = int(Math.floor(Math.random() * Constants.WORLD_SIZE));
        const pos = new Point(x, y);
        if (board.getStatus(pos) === Status.FREE) return pos;
      }
      return null;
    })();
    if (!pos) break;
    const self = pokemon(sample(kPokemon.slice(n)), null);
    board.addEntity(makePokemon(pos, self));
  }

  const anim = {damage: new Map()};
  const [choice, target, menu] = [null, null, null];
  return {anim, board, player, choice: null, target: null, menu: null};
};

const updateState = (state: State, inputs: Input[]): void => {
  const {anim, board, player} = state;
  updateAnimation(anim);
  if (board.advanceEffect()) return;

  const active = board.getActiveEntity();
  while (!player.removed && !board.getCurrentFrame() &&
         inputs.length && active === player && player.data.input === null) {
    processInput(state, nonnull(inputs.shift()));
  }
  if (state.target) return animateTarget(state, state.target);

  while (!player.removed && !board.getCurrentFrame()) {
    const entity = board.getActiveEntity();
    if (!turnReady(entity)) {
      board.advanceEntity();
      continue;
    }
    const result = act(anim, board, entity, plan(board, entity));
    if (entity === player && !result.success) break;
    wait(entity, result.moves, result.turns);
  }
};

//////////////////////////////////////////////////////////////////////////////

declare const console: any;
declare const process: any;
declare const require: any;

declare const setTimeout: (fn: () => void, delay: number) => void;

interface Element {
  hidden: boolean,
  render: () => void,
  setContent: (x: string) => void,
  cachedContent: string | null,
};

interface Timing {
  start: number,
  end: number,
};

interface UI {
  choice: Element,
  fps: Element,
  log: Element,
  map: Element,
  rivals: Element,
  status: Element,
  target: Element,
  window: Element,
};

interface IO {
  count: int,
  inputs: Input[],
  state: State,
  timing: Timing[],
  ui: UI,
};

const kHiddenFlag = '<hidden>';
const kBreakGlyph = new Glyph('\n');
const kEmptyGlyph = new Glyph(' ');

const renderFrameRate = (cpu: number, fps: number): string => {
  return `CPU: ${cpu.toFixed(2)}%; FPS: ${fps.toFixed(2)}  `;
};

const renderLog = (state: State): string => {
  return state.board.getLog().join('\n');
};

const renderMap = (state: State): string => {
  const {board, player, target} = state;
  const width  = Constants.MAP_SIZE_X;
  const height = Constants.MAP_SIZE_Y;
  const text: Glyph[] = Array((width + 1) * height).fill(kEmptyGlyph);
  for (let i = 0; i < height; i++) {
    text[width + (width + 1) * i] = kBreakGlyph;
  }

  const pos = player.pos;
  const offset_x = int(pos.x - (width - 1) / 2);
  const offset_y = int(pos.y - (height - 1) / 2);
  const offset = new Point(offset_x, offset_y);

  const show = (x: int, y: int, glyph: Glyph, force: boolean) => {
    x -= offset_x; y -= offset_y;
    if (!(0 <= x && x < width && 0 <= y && y < height)) return;
    const index = x + (width + 1) * y;
    if (force || text[index] !== kEmptyGlyph) text[index] = glyph;
  };

  const recolor = (x: int, y: int, fg?: Color | null, bg?: Color | null) => {
    x -= offset_x; y -= offset_y;
    if (!(0 <= x && x < width && 0 <= y && y < height)) return;
    const index = x + (width + 1) * y;
    text[index] = text[index]!.recolor(fg, bg);
  };

  const range = target?.data.type === TT.Summon ? target.data.range : null;
  const shade = (point: Point, glyph: Glyph, force: boolean) => {
    const out_of_range = range !== null && point.distanceL2(pos) > range - 0.5;
    const shaded_glyph = out_of_range ? glyph.recolor('gray') : glyph;
    show(point.x, point.y, shaded_glyph, force);
  };

  const vision = board.getVision(player);
  for (let x = int(0); x < width; x++) {
    for (let y = int(0); y < height; y++) {
      const point = (new Point(x, y)).add(offset);
      if ((vision.getOrNull(point) ?? -1) < 0) continue;
      shade(point, board.getTile(point).glyph, true);
    }
  }

  board.getEntities().forEach(x => {
    shade(x.pos, x.glyph, getTrainer(x) === player);
  });

  if (state.target) {
    const frame = state.target.frame >> 1;
    const {error, path, source, target} = state.target;
    const color: Color = error.length === 0 ? '440' : '400';

    recolor(source.x, source.y, 'black', '222');
    recolor(target.x, target.y, 'black', color);

    const count = Constants.TARGET_FRAMES >> 1;
    const ch = ray_character(source, target);
    for (let i = 0; i < path.length - 1; i++) {
      if ((i + count - frame) % count < 2) {
        const [{x, y}, ok] = path[i]!;
        show(x, y, new Glyph(ch, ok ? '440' : '400'), true);
      }
    }
  }

  const frame = board.getCurrentFrame();
  if (frame) frame.forEach(({point: {x, y}, glyph}) => show(x, y, glyph, false));

  return text.join('');
};

const getHPColor = (hp: number): Color => {
  return hp <= 0.25 ? '300' : hp <= 0.50 ? '330' : '020';
};

const renderBar = (width: int, value: number, color: Color | null): string => {
  const total = width - 6;
  const chars = value > 0 ? Math.max(1, Math.round(value * total)) : 0;
  return Color('='.repeat(chars), color) + ' '.repeat(total - chars);
};

const renderKey = (key?: string | null): string => {
  return key ? `[${key}] ` : '';
};

const renderEmptyStatus =
    (width: int, key: string, color?: Color | null): string[] => {
  return ['', Color(`${renderKey(key)}---`, color), '', '', ''];
};

const renderBasicPokemonStatus =
    (known: PokemonPublicState, width: int,
     key?: string | null, color?: Color | null): string[] => {
  const {damaged, species, hp, pp} = known;
  color = color ? color : damaged ? getHPColor(0) : known.hp > 0 ? null : '111';
  const bold = damaged;

  const result = [''];
  const prefix = renderKey(key);
  const spacer = ' '.repeat(prefix.length);
  const bar = int(width - prefix.length);
  const hp_color = color ? null : getHPColor(hp);
  const pp_color = color ? null : '123';

  result.push(`${prefix}${known.species.name}`);
  result.push(`${spacer}HP: [${renderBar(bar, hp, hp_color)}]`);
  result.push(`${spacer}PP: [${renderBar(bar, pp, pp_color)}]`);
  result.push('');

  return color ? result.map(x => Color(x, color, null, bold)) : result;
};

const renderFriendlyPokemonStatus =
    (anim: Anim, pokemon: Pokemon, width: int, key?: string | null,
     color?: Color | null, menu: int = -1): string[] => {
  const known = getPokemonPublicState(anim, pokemon);
  color = color ? color : known.hp > 0 ? null : '111';

  const prefix = renderKey(key);
  const spacer = ' '.repeat(prefix.length);
  const result = renderBasicPokemonStatus(known, width, key, color);

  if (menu >= 0) {
    const indent = `${spacer}  `;
    const chosen = `${spacer} > `;
    const attacks = pokemon.data.self.attacks;
    for (let i = 0; i < kAttackKeys.length; i++) {
      const key = kAttackKeys[i];
      const name = attacks[i]?.name || null;
      const prefix = menu === i ? chosen : indent;
      const line = name ? `${prefix}${renderKey(key)}${name}`
                        : Color(`${prefix}${renderKey(key)}---`, '111');
      [Color(line, color), ''].forEach(x => result.push(x));
    }
    const prefix = menu === kAttackKeys.length ? chosen : indent;
    const line = `${prefix}${renderKey('r')}Call back`;
    [Color(line, color), ''].forEach(x => result.push(x));
  }
  return result;
};

const renderPartyPokemonStatus =
    (pokemon: PokemonIndividualData, width: int, key: string): string[] => {
  const known = getPartyPokemonPublicState(pokemon);
  return renderBasicPokemonStatus(known, width, key);
};

const renderRivalPokemonStatus =
    (anim: Anim, pokemon: Pokemon, width: int): string[] => {
  const known = getPokemonPublicState(anim, pokemon);
  return renderBasicPokemonStatus(known, width);
};

const renderTrainerStatus =
    (anim: Anim, trainer: Trainer, width: int, key?: string): string[] => {
  const damaged = anim.damage.has(trainer);
  const color = damaged ? getHPColor(0) : null;
  const bold = damaged;

  const name = capitalize(describe(trainer));
  const status = trainer.data.pokemon.map(
      x => x.self.cur_hp > 0 ? '*' : Color('*', '111'));
  const hp = getAnimatedHP(anim, trainer);

  const result = [''];
  const prefix = renderKey(key);
  const spacer = ' '.repeat(prefix.length);
  const bar = int(width - prefix.length);
  const hp_color = color ? null : getHPColor(hp);

  result.push(`${renderKey(key)}${name}`);
  result.push(`${spacer}HP: [${renderBar(bar, hp, hp_color)}]`);
  result.push(`${spacer}     ${status.join(' ')}`);
  result.push('');

  return color ? result.map(x => Color(x, color, null, bold)) : result;
};

const renderChoice = (state: State): string => {
  if (!state.choice) return kHiddenFlag;

  const kColumnPadding = 2;
  const width = int(Constants.CHOICE_SIZE - 2 * kColumnPadding - 5);
  const {anim, player} = state;

  const result: string[] = [];
  const index = state.choice.index;
  const space = ' '.repeat(kColumnPadding + 1);
  const arrow = ' '.repeat(kColumnPadding) + '>';

  for (let i = 0; i < kPartyKeys.length; i++) {
    const key = kPartyKeys[i]!;
    const pokemon = player.data.pokemon[i];
    const status = (() => {
      if (!pokemon) return renderEmptyStatus(width, key, '111');
      const entity = pokemon.entity;
      return entity ? renderFriendlyPokemonStatus(anim, entity, width, key, '111')
                    : renderPartyPokemonStatus(pokemon.self, width, key);
    })();
    status.forEach((line, j) => {
      const selected = i === index;
      const use_arrow = selected && j === 1;
      result.push(`${use_arrow ? arrow : space}${selected ? ' ' : ''}${line}`);
    });
  }
  return result.join('\n');
};

const renderRivals = (state: State): string => {
  const width = Constants.STATUS_SIZE;
  const {anim, board, menu, player} = state;

  const rows: string[] = [];
  const rivals = findRivalPokemon(board, player);
  rivals.forEach(y => renderRivalPokemonStatus(anim, y, width)
                          .forEach(x => rows.push(x)));
  return rows.join('\n');
};

const renderStatus = (state: State): string => {
  const kl = renderKey('a').length;
  const width = int(Constants.STATUS_SIZE + kl);
  const {anim, board, menu, player} = state;
  const vision = board.getVision(player);

  const rows: string[] = [];
  const key = menu ? '-' : kPlayerKey;
  renderTrainerStatus(anim, player, width, key).forEach(x => rows.push(x));

  let summon = 0;
  while (summon < player.data.summons.length) {
    const pokemon = player.data.summons[summon]!;
    const key = menu ? '-' : kSummonedKeys[summon];
    const index = menu && menu.summon === summon ? menu.index : -1;
    renderFriendlyPokemonStatus(anim, pokemon, width, key, null, index)
        .forEach(x => rows.push(x));
    summon++;
  }

  while (summon < kSummonedKeys.length) {
    const key = kSummonedKeys[summon++]!;
    renderEmptyStatus(width, key).forEach(x => rows.push(x));
  }
  return rows.join('\n');
};

const renderTarget = (state: State): string => {
  const {anim, board, player, target} = state;
  const width = Constants.STATUS_SIZE;
  const rows: string[] = [];

  if (target) {
    const explanation = (() => {
      switch (target.data.type) {
        case TT.Attack: {
          const name = target.data.summon.data.self.species.name;
          return `Using ${name}'s ${target.data.attack.name}...`;
        }
        case TT.Summon: {
          const name = target.data.pokemon.self.species.name;
          return `Sending out ${name}...`;
        }
        case TT.FarLook: return `Examining...`;
      }
    })();

    const vision = board.getVision(state.player);
    const seen = (vision.getOrNull(target.target) ?? -1) >= 0;
    const tile = seen ? state.board.getTile(target.target) : null;
    const entity = seen ? board.getEntity(target.target) : null;

    rows.push('');
    rows.push(explanation);
    if (entity) {
      if (entity.type === ET.Trainer) {
        renderTrainerStatus(anim, entity, width).forEach(x => rows.push(x));
      } else {
        const known = getPokemonPublicState(anim, entity);
        renderBasicPokemonStatus(known, width).forEach(x => rows.push(x));
      }
    } else {
      rows.push('');
    }
    if (tile) {
      const prefix = entity ? 'Standing on' : 'You see';
      rows.push(`${prefix}: ${tile.glyph} (${tile.description})`);
    } else {
      rows.push(`You see: (unseen location)`);
    }
    return rows.join('\n');
  }

  rows.push('');
  rows.push('No target selected.');
  rows.push('');
  rows.push('[x] examine your surroundings');
  return rows.join('\n');
};

const initIO = (state: State): IO => {
  const blessed = require('../extern/blessed');
  const window = blessed.screen({fullUnicode: true});

  const kColSpace = 2;
  const kRowSpace = 1;
  const x = Constants.MAP_SIZE_X;
  const y = Constants.MAP_SIZE_Y;
  const ss = Constants.STATUS_SIZE;
  const kl = renderKey('a').length;
  const w = 2 * x + 2 + 2 * (ss + kl + 2 * kColSpace);
  const h = y + 2 + kRowSpace + Constants.LOG_SIZE + kRowSpace + 1;

  const [sw, sh, sl, st] = [ss + kl, y - kRowSpace, kColSpace, kRowSpace + 1];
  const [mw, mh, ml, mt] = [2 * x + 2, y + 2, sl + sw + kColSpace, 0];
  const [tw, th, tl, tt] = [ss + kl, 9, ml + mw + kColSpace, st];
  const wt = tt + th + 2 * kRowSpace + 1;
  const [rw, rh, rl, rt] = [ss + kl, mh - wt - kRowSpace - 1, tl, wt];
  const [lw, lh, ll, lt] = [w, Constants.LOG_SIZE, 0, mt + mh + kRowSpace];
  const [left, top, attr, wrap] = ['center', 'center', false, false];
  const content = blessed.box({width: w, height: h, left, top, attr, wrap});

  assert(tl + tw + kColSpace === w, () => `${tl + tw + kColSpace} === ${w}`);
  assert(lt + lh + kRowSpace + 1 === h, () => `${lt + lh + kRowSpace + 1} === ${h}`);

  let [cw, ch] = [Constants.CHOICE_SIZE, 6 * 5 + 2];
  let [cl, ct] = [int(Math.floor(w - cw) / 2), int(Math.floor(h - ch) / 2)];
  if (ml % 2 === cl % 2) { cl -= 1; cw += 2; Constants.CHOICE_SIZE += 2; }
  if (Constants.CHOICE_SIZE % 2 !== 0) { cw += 1; Constants.CHOICE_SIZE += 1; }

  const element = (width: number, height: number, left: number, top: number,
                   extra?: {[k: string]: unknown}): Element => {
    const data: {[k: string]: unknown} = {width, height, left, top, attr, wrap};
    Object.entries(extra || {}).forEach(([k, v]) => data[k] = v);
    const result = blessed.box(data);
    content.append(result);
    return result;
  };

  const divider = (width: number, left: number, top: number, text: string) => {
    const length = text.length;
    const result: Element = blessed.box({width, height: 1, left, top});
    const label = length ? `--${text}${'-'.repeat(width - length - 2)}`
                         : '-'.repeat(width)
    result.setContent(Color(label, '430'));
    content.append(result);
  };

  const color = 178;
  divider(ml, 0, 0, 'Party');
  divider(ml, ml + mw, 0, 'Target');
  divider(ml, ml + mw, tt + th + kRowSpace, 'Wild Pokemon');
  divider(ml, 0, mh - 1, 'Log');
  divider(ml, ml + mw, mh - 1, '');
  divider(w, 0, h - 1, '');

  const log = element(lw, lh, ll, lt);
  const map = element(mw, mh, ml, mt, {border: {type: 'line', fg: color}});
  const rivals = element(rw, rh, rl, rt);
  const status = element(sw, sh, sl, st);
  const target = element(tw, th, tl, tt);
  const choice = element(cw, ch, cl, ct, {border: {type: 'line', fg: color}});
  const fps = blessed.box({align: 'right', top: '100%-1'});
  [content, fps].map(x => window.append(x));
  const ui: UI = {choice, fps, log, map, rivals, status, target, window};

  const inputs: Input[] = [];
  window.key(['S-a', 'C-c'], () => process.exit(0));
  const kFastDirectionKeys = Array.from(kDirectionKeys).map(x => `S-${x}`);
  const kAllKeys = ['enter', 'escape', 'tab', 'S-tab', '.']
    .concat(Array.from(kAlphabetKeys))
    .concat(kFastDirectionKeys);
  kAllKeys.forEach(x => window.key([x], () => inputs.push(x)));
  return {count: 0, inputs, state, timing: [], ui};
};

const update = (io: IO) => {
  const start = Date.now();
  io.timing.push({start, end: start});
  if (io.timing.length > Constants.FRAME_RATE) io.timing.shift();
  assert(io.timing.length <= Constants.FRAME_RATE);
  io.count = int((io.count + 1) & 0xffff);

  updateState(io.state, io.inputs);
};

const cachedSetContent = (element: Element, content: string): boolean => {
  if (content === element.cachedContent) return false;
  element.hidden = content === kHiddenFlag;
  element.cachedContent = content;
  element.setContent(content);
  return true;
};

const render = (io: IO) => {
  const refresh = [
    cachedSetContent(io.ui.log, renderLog(io.state)),
    cachedSetContent(io.ui.map, renderMap(io.state)),
    cachedSetContent(io.ui.choice, renderChoice(io.state)),
    cachedSetContent(io.ui.rivals, renderRivals(io.state)),
    cachedSetContent(io.ui.status, renderStatus(io.state)),
    cachedSetContent(io.ui.target, renderTarget(io.state)),
    io.count % 10 === 0,
  ];
  if (!refresh.some(x => x)) return;

  const last = nonnull(io.timing[io.timing.length - 1]);
  const base = io.timing.reduce((acc, x) => acc += x.end - x.start, 0);

  last.end = Date.now();
  const total = Math.max(last.end - nonnull(io.timing[0]).start, 1);
  const cpu = 100 * (last.end - last.start + base) / total;
  const fps = 1000 * io.timing.length / total;

  io.ui.fps.setContent(renderFrameRate(cpu, fps));
  io.ui.window.render();
};

const tick = (io: IO) => () => {
  try {
    update(io);
    render(io);
  } catch (error) {
    console.error(error);
  }
  const len = io.timing.length;
  const fps = Constants.FRAME_RATE;
  const next_a = nonnull(io.timing[0]).start + Math.floor(1000 * len / fps);
  const next_b = nonnull(io.timing[len - 1]).start + Math.floor(500 / fps);
  const delay = Math.max(Math.max(next_a, next_b) - Date.now(), 1);
  setTimeout(tick(io), Math.min(delay, Math.floor(1000 / fps)));
};

const main = () => {
  const state = initState();
  const io = initIO(state);
  tick(io)();
};

main();

export {};
