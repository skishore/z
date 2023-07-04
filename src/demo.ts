import {Color, Glyph} from './lib';
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
    this.effect = [];
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

  getEffect(): Effect {
    return this.effect;
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

  getStatus(pos: Point): Status {
    if (this.getTile(pos).blocked) return Status.BLOCKED;
    if (this.entityAtPos.has(pos.key())) return Status.OCCUPIED;
    return Status.FREE;
  }

  // Writes

  setTile(pos: Point, tile: Tile) {
    this.map.set(pos, tile);
    this.entity.forEach(x => this.dirtyVision(x));
  }

  addEffect(effect: Effect) {
    this.effect = ParallelEffect([this.effect, effect]);
  }

  advanceEffect(): boolean {
    return !!this.effect.shift();
  }

  addEntity(entity: Entity) {
    const pos = entity.pos;
    assert(this.getEntity(pos) === null);
    this.entityAtPos.set(pos.key(), entity);
    this.entity.push(entity);
  }

  advanceEntity() {
    charge(nonnull(this.entity[this.entityIndex]));
    this.entityIndex = int((this.entityIndex + 1) % this.entity.length);
  }

  moveEntity(from: Point, to: Point) {
    const key = from.key();
    const entity = nonnull(this.entityAtPos.get(key));
    assert(this.getEntity(to) === null);
    assert(entity.pos.equal(from));

    this.entityAtPos.delete(key);
    this.entityAtPos.set(to.key(), entity);
    entity.pos = to;
    this.dirtyVision(entity);
  }

  removeEntity(entity: Entity) {
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

  swapEntities(a: Point, b: Point) {
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

  log(line: string, menu?: boolean) {
    if (menu && this.logs[this.logs.length - 1]?.menu) this.logs.pop();
    this.logs.push({line, menu: !!menu});
    if (this.logs.length > Constants.LOG_SIZE) this.logs.shift();
  }

  logIfPlayer(entity: Entity, line: string) {
    if (entity.type === ET.Trainer && entity.data.player) this.log(line);
  }

  logMenu(line: string, done?: boolean) {
    this.log(line, true);
    if (done && this.logs.length > 0) {
      this.logs[this.logs.length - 1]!.menu = false;
    }
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
enum TT { Attack, Summon };

type Action =
  {type: AT.Attack, attack: Attack, target: Point} |
  {type: AT.Idle} |
  {type: AT.Move, direction: Direction} |
  {type: AT.Shout, command: Command, pokemon: Pokemon} |
  {type: AT.Summon, index: int, target: Point} |
  {type: AT.Withdraw, pokemon: Pokemon} |
  {type: AT.WaitForInput};

type GenEffect = (board: Board, source: Point, target: Point) => AttackEffect;

interface Result { success: boolean, moves: number, turns: number };

type Command =
  {type: CT.Attack, attack: Attack, target: Entity | Point} |
  {type: CT.Return};

interface Attack { name: string, range: int, damage: int, effect: GenEffect };

interface AttackEffect { effect: Effect, hitFrame: int };

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

const move_ready = (entity: Entity): boolean => {
  return entity.move_timer <= 0;
};

const turn_ready = (entity: Entity): boolean => {
  return entity.turn_timer <= 0;
};

const trainer = (entity: Entity): Trainer | null => {
  return entity.type === ET.Pokemon ? entity.data.self.trainer : null;
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
    const prev = line[i - 1]!;
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
      if (move_ready(entity) && hasLineOfSight(board, source, target, range)) {
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

  const getTrainer = (entity: Entity) => {
    return entity.type === ET.Trainer ? entity : entity.data.self.trainer;
  };
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
  const getTrainer = (entity: Entity) => {
    return entity.type === ET.Trainer ? entity : entity.data.self.trainer;
  };
  const trainer = getTrainer(entity);
  const all = board.getEntities().filter(x => getTrainer(x) !== trainer);
  return all.filter(x => hasLineOfSight(board, entity.pos, x.pos, 12));
};

const plan = (board: Board, entity: Entity): Action => {
  switch (entity.type) {
    case ET.Pokemon: {
      const {commands, self: {attacks, trainer}} = entity.data;
      if (commands.length > 0) return followCommands(board, entity, commands);

      const ready = !move_ready(entity);
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

const act = (board: Board, entity: Entity, action: Action): Result => {
  switch (action.type) {
    case AT.Attack: {
      const source = entity.pos;
      const {attack, target} = action;
      if (!hasLineOfSight(board, source, target, attack.range)) return kFailure;

      // success
      const attack_effect = attack.effect(board, entity.pos, target);
      board.addEffect(ApplyAttack(board, attack_effect, target));
      const user = capitalize(describe(entity));
      const target_entity = board.getEntity(target);

      if (target_entity === null) {
        board.log(`${user} used ${attack.name}!`);
      } else if (target_entity.type === ET.Trainer) {
        const data = target_entity.data;
        data.cur_hp = int(Math.max(data.cur_hp - 1, 0));

        const target_name = describe(target_entity);
        const base = `${user} attacked ${target_name} with ${attack.name}!`;
        const and_ = data.cur_hp ? '' : ` ${capitalize(target_name)} blacked out!`;
        board.log(`${base}${and_}`);

        if (!data.cur_hp) board.removeEntity(target_entity);
      } else {
        const data = target_entity.data.self;
        const damage = int(Math.random() * attack.damage);
        data.cur_hp = int(Math.max(data.cur_hp - damage, 0));

        const target_name = describe(target_entity);
        const base = `${user} attacked ${target_name} with ${attack.name}!`;
        const and_ = data.cur_hp ? '' : ` ${capitalize(target_name)} fainted!`;
        board.log(`${base}${and_}`);

        if (!data.cur_hp) board.removeEntity(target_entity);
      }
      return {success: true, moves: 1, turns: 1};
    }
    case AT.Idle: return kSuccess;
    case AT.Move: {
      const pos = entity.pos.add(action.direction);
      if (pos.equal(entity.pos)) return kSuccess;
      if (board.getTile(pos).blocked) return kFailure;
      const other = board.getEntity(pos);
      if (other) {
        if (trainer(other) !== entity) return kFailure;
        board.swapEntities(entity.pos, pos);
        board.logIfPlayer(entity, `You swap places with ${describe(other)}.`);
        entity.dir = action.direction;
        return kSuccess;
      }

      // success
      board.moveEntity(entity.pos, pos);
      entity.dir = action.direction;
      return kSuccess;
    }
    case AT.Shout: {
      const pokemon = action.pokemon;
      if (trainer(pokemon) !== entity) return kFailure;

      // success
      const range = Constants.SUMMON_RANGE;
      const [source, target] = [entity.pos, pokemon.pos];
      if (action.command.type === CT.Return &&
          hasLineOfSight(board, source, target, range)) {
        board.removeEntity(pokemon);
        board.addEffect(WithdrawEffect(source, target, pokemon.glyph));
      } else {
        pokemon.data.commands.push(action.command);
      }
      shout(board, entity, form(action.command, pokemon));
      return kSuccess;
    }
    case AT.Summon: {
      const {index, target} = action;
      if (entity.type !== ET.Trainer) return kFailure;
      const pokemon = entity.data.pokemon[index];
      if (!pokemon || pokemon.entity || !pokemon.self.cur_hp) return kFailure;
      if (entity.data.summons.length >= kSummonedKeys.length) return kFailure;
      if (board.getStatus(target) !== Status.FREE) return kFailure;

      // success
      const glyph = board.getTile(target).glyph;
      const summoned = makePokemon(target, pokemon.self);
      entity.data.summons.push(summoned);
      pokemon.entity = summoned;
      board.addEntity(summoned);
      board.addEffect(SummonEffect(entity.pos, target, glyph));
      shout(board, entity, `Go! ${pokemon.self.species.name}!`);
      return kSuccess;
    }
    case AT.Withdraw: {
      const pokemon = action.pokemon;
      const range = Constants.SUMMON_RANGE;
      const [source, target] = [entity.pos, pokemon.pos];
      if (trainer(pokemon) !== entity) return kFailure;
      if (!hasLineOfSight(board, source, target, range)) return kFailure;

      // success
      board.removeEntity(pokemon);
      board.addEffect(WithdrawEffect(source, target, pokemon.glyph));
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

const animateSummon = (state: State, summon: Summon): void => {
  const frame = summon.frame;
  summon.frame = int((frame + 1) % Constants.SUMMON_ANIM);
};

const initSummon = (state: State, index: int, range: int): Summon => {
  const player = state.player;
  const target = state.player.pos;
  const result = {error: '', frame: int(0), index, path: [], range, target};

  const defend = defendSquare(state.board, target, player);
  if (defend) {
    const line = LOS(target, defend).slice(1).reverse();
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
  const best = target.add(player.dir.scale(2));
  const next = target.add(player.dir.scale(1));
  if (okay(best)) return result;
  if (okay(next)) return result;

  for (let dx = -2; dx <= 2; dx++) {
    for (let dy = -2; dy <= 2; dy++) {
      const pos = new Point(int(target.x + dx), int(target.y + dy));
      if (okay(pos)) options.push(pos);
    }
  }

  options.sort((a, b) => best.distanceSquared(a) - best.distanceSquared(b));
  updateSummonTarget(state, result, options[0] || target);
  return result;
};

const updateSummonTarget = (state: State, summon: Summon, target: Point): void => {
  const range = summon.range;
  const {board, player} = state;
  const los = LOS(player.pos, target);
  const start = los.length > 1 ? 1 : 0;

  summon.error = '';
  summon.path.length = 0;
  for (let i = start; i < los.length; i++) {
    const point = los[i]!;
    if (summon.error.length === 0) {
      if (board.getStatus(point) !== Status.FREE) {
        summon.error = 'That location is blocked.';
      } else if (player.pos.distanceL2(point) > range - 0.5) {
        summon.error = `You can't throw that far.`;
      } else if ((board.getVision(player).getOrNull(point) ?? -1) < 0) {
        summon.error = `You can't see a clear path there.`;
      }
    }
    summon.path.push([point, summon.error.length === 0]);
  }

  summon.frame = 0;
  summon.target = target;
};

//////////////////////////////////////////////////////////////////////////////

interface Particle {point: Point, glyph: Glyph};
interface Frame extends Array<Particle> {};
interface Effect extends Array<Frame> {};

type Sparkle = [int, string, Color?][];

const add_particle = (effect: Effect, frame: int, particle: Particle): void => {
  while (frame >= effect.length) effect.push([]);
  nonnull(effect[frame]).push(particle);
};

const add_sparkle =
    (effect: Effect, sparkle: Sparkle, frame: int, point: Point): int => {
  sparkle.forEach(x => {
    const [delay, chars, color] = x;
    for (let i = 0; i < delay; i++, frame++) {
      const index = Math.floor(Math.random() * chars.length);
      const glyph = new Glyph(nonnull(chars[index]), color);
      add_particle(effect, frame, {glyph, point});
    }
  });
  return frame;
};

const random_delay = (n: int): int => {
  let count = int(1);
  const limit = Math.floor(1.5 * n);
  while (count < limit && Math.floor(Math.random() * n)) count++;
  return count;
};

const ray_character = (source: Point, target: Point): string => {
  const dx = target.x - source.x;
  const dy = target.y - source.y;
  if (Math.abs(dx) > 2 * Math.abs(dy)) return '-';
  if (Math.abs(dy) > 2 * Math.abs(dx)) return '|';
  return ((dx > 0) === (dy > 0)) ? '\\' : '/';
};

const ConstantEffect = (particle: Particle, n: int): Effect => {
  return Array(n).fill([particle]);
};

const PauseEffect = (n: int): Effect => {
  return Array(n).fill([]);
};

const OverlayEffect = (effect: Effect, particle: Particle): Effect => {
  const constant = ConstantEffect(particle, int(effect.length));
  return ParallelEffect([effect, constant]);
};

const UnderlayEffect = (effect: Effect, particle: Particle): Effect => {
  const constant = ConstantEffect(particle, int(effect.length));
  return ParallelEffect([constant, effect]);
};

const ExtendEffect = (effect: Effect, n: int): Effect => {
  const result: Effect = [];
  effect.forEach(frame => range(n).forEach(_ => result.push(frame)));
  return result;
};

const ParallelEffect = (effects: Effect[]): Effect => {
  const result: Effect = [];
  effects.forEach(effect => effect.forEach((frame, i) => {
    if (i >= result.length) result.push([]);
    frame.forEach(x => nonnull(result[i]).push(x));
  }));
  return result;
};

const SerialEffect = (effects: Effect[]): Effect => {
  return flatten(effects);
};

const ExplosionEffect = (point: Point): Effect => {
  const color = '400';
  const base = [
    [{point, glyph: new Glyph('*', color)}],
    [
      {point, glyph: new Glyph('+', color)},
      {point: point.add(Direction.n), glyph: new Glyph('|', color)},
      {point: point.add(Direction.s), glyph: new Glyph('|', color)},
      {point: point.add(Direction.e), glyph: new Glyph('-', color)},
      {point: point.add(Direction.w), glyph: new Glyph('-', color)},
    ],
    [
      {point: point.add(Direction.n),  glyph: new Glyph('-', color)},
      {point: point.add(Direction.s),  glyph: new Glyph('-', color)},
      {point: point.add(Direction.e),  glyph: new Glyph('|', color)},
      {point: point.add(Direction.w),  glyph: new Glyph('|', color)},
      {point: point.add(Direction.ne), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.sw), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.nw), glyph: new Glyph('/', color)},
      {point: point.add(Direction.se), glyph: new Glyph('/', color)},
    ],
  ];
  return ExtendEffect(base, 4);
};

const ImplosionEffect = (point: Point): Effect => {
  const color = '400';
  const base = [
    [
      {point, glyph: new Glyph('*', color)},
      {point: point.add(Direction.ne), glyph: new Glyph('/', color)},
      {point: point.add(Direction.sw), glyph: new Glyph('/', color)},
      {point: point.add(Direction.nw), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.se), glyph: new Glyph('\\', color)},
    ],
    [
      {point, glyph: new Glyph('#', color)},
      {point: point.add(Direction.ne), glyph: new Glyph('/', color)},
      {point: point.add(Direction.sw), glyph: new Glyph('/', color)},
      {point: point.add(Direction.nw), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.se), glyph: new Glyph('\\', color)},
    ],
    [{point, glyph: new Glyph('*', color)}],
    [{point, glyph: new Glyph('#', color)}],
  ];
  return ExtendEffect(base, 3);
};

const RayEffect = (source: Point, target: Point, speed: int): Effect => {
  const color = '400';
  const result: Effect = [];
  const line = LOS(source, target);
  if (line.length <= 2) return result;

  const beam = new Glyph(ray_character(source, target), color);
  const mod = int((line.length - 2 + speed) % speed);
  const start = int(mod ? mod : mod + speed);
  for (let i = start; i < line.length - 1; i = int(i + speed)) {
    result.push(range(i).map(j => ({point: nonnull(line[j + 1]), glyph: beam})));
  }
  return result;
};

const SummonEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  const color = '400';
  const base: Effect = [];
  const line = LOS(source, target);
  const ball = new Glyph('*', color);
  for (let i = 1; i < line.length - 1; i++) {
    base.push([{point: nonnull(line[i]), glyph: ball}]);
  }
  const masked = UnderlayEffect(base, {point: target, glyph});
  return SerialEffect([masked, ExplosionEffect(target)]);
};

const WithdrawEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  const base = RayEffect(source, target, 4);
  const hide = {point: target, glyph};
  const impl = UnderlayEffect(ImplosionEffect(target), hide);
  const full = base[base.length - 1];
  if (!full) return impl;

  return SerialEffect([
    base,
    ParallelEffect([ExtendEffect([full], int(impl.length)), impl]),
    UnderlayEffect(base.slice().reverse(), hide),
  ]);
};

const SwitchEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  return SerialEffect([
    WithdrawEffect(source, target, glyph),
    ConstantEffect({point: target, glyph}, 4),
    SummonEffect(source, target, glyph),
  ]);
};

const EmberEffect = (board: Board, source: Point, target: Point): AttackEffect => {
  const effect: Effect = [];
  const line = LOS(source, target);

  const trail = (): Sparkle => [
    [random_delay(0), '*^^',   '400'],
    [random_delay(1), '*^',    '420'],
    [random_delay(2), '**^',   '440'],
    [random_delay(3), '**^#%', '420'],
    [random_delay(4), '#%',    '400'],
  ];
  const flame = (): Sparkle => [
    [random_delay(0), '*^^',   '400'],
    [random_delay(1), '*^',    '420'],
    [random_delay(2), '**^#%', '440'],
    [random_delay(3), '*^#%',  '420'],
    [random_delay(4), '*^#%',  '400'],
  ];

  for (let i = 1; i < line.length - 1; i++) {
    const frame = int(Math.floor((i - 1) / 2));
    add_sparkle(effect, trail(), frame, nonnull(line[i]));
  }
  let hitFrame: int = 0;
  for (const direction of [Direction.none].concat(Direction.all)) {
    const norm = direction.distanceTaxicab(Direction.none);
    const frame = int(2 * norm + Math.floor((line.length - 1) / 2));
    const finish = add_sparkle(effect, flame(), frame, target.add(direction));
    if (norm === 0) hitFrame = finish;
  }
  return {effect, hitFrame};
};

const IceBeamEffect = (board: Board, source: Point, target: Point): AttackEffect => {
  const effect: Effect = [];
  const line = LOS(source, target);
  const ch = ray_character(source, target);

  const trail: Sparkle = [
    [2, ch, '555'],
    [2, ch, '044'],
    [2, ch, '004'],
  ];
  const flame: Sparkle = [
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
  ];

  for (let i = 1; i < line.length; i++) {
    const frame = int(Math.floor((i - 1) / 2));
    add_sparkle(effect, trail, frame, nonnull(line[i]));
  }
  const frame = int(Math.floor((line.length - 1) / 2));
  const hitFrame = add_sparkle(effect, flame, frame, target);
  return {effect, hitFrame};
};

const BlizzardEffect = (board: Board, source: Point, target: Point): AttackEffect => {
  const effect: Effect = [];
  const ch = ray_character(source, target);

  const points = [target];
  const used = new Set<int>();
  while (points.length < 4) {
    const alt = target.add(sample(Direction.all));
    const key = alt.key();
    if (used.has(key)) continue;
    points.push(alt);
    used.add(key);
  }

  const trail: Sparkle = [
    [1, ch, '555'],
    [1, ch, '044'],
    [1, ch, '004'],
  ];
  const flame: Sparkle = [
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
  ];

  let hitFrame: int = 0;
  for (let p = 0; p < points.length; p++) {
    const d = 12 * p;
    const next = nonnull(points[p]);
    const line = LOS(source, next);
    for (let i = 1; i < line.length; i++) {
      add_sparkle(effect, trail, int(d + i), nonnull(line[i]));
    }
    const finish = add_sparkle(effect, flame, int(d + line.length - 1), next);
    if (p === 0) hitFrame = finish;
  }
  hitFrame = int(hitFrame - Math.ceil(hitFrame / 3));
  return {effect: effect.filter((_, i) => i % 3), hitFrame};
};

const HeadbuttEffect = (board: Board, source: Point, target: Point): AttackEffect => {
  const source_tile = board.getTile(source);
  const source_entity = board.getEntity(source);
  const glyph = source_entity ? source_entity.glyph : source_tile.glyph;
  const ch = ray_character(source, target);

  const trail = (): Sparkle => [
    [2, '#', '444'],
    [2, '#', '333'],
    [2, '#', '222'],
    [2, '#', '111'],
  ];

  const line = LOS(source, target);

  const move_along_line = (line: Point[]) => {
    const effect: Effect = [];
    for (let i = 1; i < line.length - 1; i++) {
      const point = nonnull(line[i]);
      add_particle(effect, int(i - 1), {point, glyph});
      add_sparkle(effect, trail(), int(i), point);
    }
    return effect;
  };

  const move_length = int(line.length - 2);
  const hold_pause = PauseEffect(move_length);
  const hold_point = line[move_length] || source;
  const hold_effect = ConstantEffect({point: hold_point, glyph}, int(32));
  let hitFrame = int(Math.max(move_length, 0));

  const towards = move_along_line(line);
  const hold    = SerialEffect([hold_pause, hold_effect]);
  const away    = move_along_line(line.reverse());

  const back_length = int(hold.length + hitFrame);
  const back_pause = PauseEffect(back_length);
  const back_effect = ConstantEffect({point: source, glyph}, int(away.length - hitFrame));
  const back = SerialEffect([back_pause, back_effect]);

  const result = UnderlayEffect(
      ParallelEffect([SerialEffect([ParallelEffect([towards, hold]), away]), back]),
      {point: source, glyph: source_tile.glyph});

  hitFrame = int(hitFrame - Math.ceil(hitFrame / 2));
  return {effect: result.filter((_, i) => i % 2), hitFrame};
};

const ApplyAttack = (board: Board, init: AttackEffect, target: Point): Effect => {
  let effect = init.effect;
  const frame = init.hitFrame;
  const entity = board.getEntity(target);
  if (entity !== null) {
    const source = {point: target, glyph: entity.glyph}
    const damage = {point: target, glyph: entity.glyph.recolor('black', '400')};
    effect = ParallelEffect([
      ConstantEffect(source, frame),
      SerialEffect([PauseEffect(frame), ConstantEffect(damage, 8)]),
      effect,
    ]);
  }
  return effect;
};

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  LOG_SIZE:      int(4),
  MAP_SIZE_X:    int(43),
  MAP_SIZE_Y:    int(43),
  FOV_RADIUS:    int(21),
  WORLD_SIZE:    int(100),
  STATUS_SIZE:   int(28),
  CHOICE_SIZE:   int(40),
  FRAME_RATE:    int(60),
  MOVE_TIMER:    int(960),
  TURN_TIMER:    int(120),
  SUMMON_ANIM:   int(60),
  SUMMON_RANGE:  int(12),
  TRAINER_HP:    int(8),
  TRAINER_SPEED: 1/10,
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
  {name: 'Ember',    range: 12, damage: int(40), effect: EmberEffect},
  {name: 'Ice Beam', range: 12, damage: int(60), effect: IceBeamEffect},
  {name: 'Blizzard', range: 12, damage: int(80), effect: BlizzardEffect},
  {name: 'Headbutt', range: 8,  damage: int(80), effect: HeadbuttEffect},
  {name: 'Tackle',   range: 4,  damage: int(40), effect: HeadbuttEffect},
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

interface State {
  board: Board,
  player: Trainer,
  choice: Choice | null,
  summon: Summon | null,
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

interface Summon {
  error: string,
  frame: int
  index: int,
  path: [Point, boolean][],
  range: int,
  target: Point,
};

interface PokemonPublicState {
  species: PokemonSpeciesData,
  hp: number,
  pp: number,
  pos: Point | null,
};

interface Target {
  known: PokemonPublicState,
  pokemon: Pokemon,
  stale: boolean,
};

const getPartyPokemonPublicState =
    (self: PokemonIndividualData): PokemonPublicState => {
  const hp = self.cur_hp / Math.max(self.max_hp, 1);
  return {species: self.species, hp, pp: 1, pos: null};
};

const getPokemonPublicState = (pokemon: Pokemon): PokemonPublicState => {
  const result = getPartyPokemonPublicState(pokemon.data.self);
  const bp = (pokemon.move_timer ?? 0) / Constants.MOVE_TIMER;
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
  const {board, player, choice, summon, menu} = state;
  const enter = input === 'enter' || input === '.';
  const escape = input === 'escape';

  if (input === 'tab' || input === 'S-tab') {
    const rivals = findRivalPokemon(board, player);
    if (rivals.length === 0) return;

    const n = rivals.length;
    const tab = input === 'tab';
    const start = state.target ? rivals.indexOf(state.target.pokemon) : -1;
    const index = start >= 0 ? (start + n + (tab ? 1 : -1)) % n
                             : tab ? 0 : n - 1;
    const pokemon = nonnull(rivals[index]);
    const known = getPokemonPublicState(pokemon);
    state.target = {known, pokemon, stale: false};
    return;
  }

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
        state.summon = initSummon(state, chosen, range);
        state.choice = null;
      }
    } else if (escape) {
      board.logMenu(Color('Canceled.', '234'));
      state.choice = null;
    }
    return;
  }

  if (summon) {
    const lower = input.startsWith('S-') ? input.substr(2) : input;
    const direction = Direction.all[kDirectionKeys.indexOf(lower)];
    if (direction) {
      let target = summon.target;
      const scale = input === lower ? 1 : 4;
      for (let i = 0; i < scale; i++) {
        const next_target = target.add(direction);
        if (outsideMap(state, next_target)) break;
        target = next_target;
      }
      if (!target.equal(summon.target)) {
        updateSummonTarget(state, summon, target);
      }
    } else if (enter) {
      if (summon.error.length > 0) {
        board.logMenu(Color(summon.error, '422'));
      } else {
        const {index, target} = summon;
        const name = player.data.pokemon[index]?.self.species.name;
        player.data.input = {type: AT.Summon, index, target};
        state.summon = null;
      }
    } else if (escape) {
      board.logMenu(Color('Canceled.', '234'));
      state.summon = null;
    }
    return;
  }

  if (menu) {
    assert(0 <= menu.summon);
    assert(menu.summon < player.data.summons.length);

    const summon = player.data.summons[menu.summon]!;
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
        const rivals = findRivalPokemon(board, player);
        if (!attack) {
          board.logMenu(Color(`${name} does not have that attack.`, '422'));
        } else if (rivals.length === 0) {
          board.logMenu(Color(`There are no targets for ${name}'s ${attack.name}.`, '422'));
        } else {
          const target = rivals[0]!;
          const target_name = target.data.self.species.name;
          const command = {type: CT.Attack, attack, target};
          player.data.input = {type: AT.Shout, command, pokemon: summon};
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

  return {board, player, choice: null, summon: null, target: null, menu: null};
};

const updatePlayerTarget = (state: State): void => {
  const {board, player, target} = state;
  if (!target) return;

  if (target.pokemon.removed) {
    state.target = null;
  } else {
    const vision = board.getVision(player);
    target.stale = (vision.getOrNull(target.pokemon.pos) ?? -1) < 0;
    if (!target.stale) target.known = getPokemonPublicState(target.pokemon);
  }
}

const updateState = (state: State, inputs: Input[]): void => {
  const {board, player} = state;
  if (board.advanceEffect()) return;

  const active = board.getActiveEntity();
  while (!player.removed && !board.getEffect().length &&
         inputs.length && active === player && player.data.input === null) {
    processInput(state, nonnull(inputs.shift()));
  }
  if (state.summon) return animateSummon(state, state.summon);

  while (!player.removed && !board.getEffect().length) {
    const entity = board.getActiveEntity();
    if (!turn_ready(entity)) {
      board.advanceEntity();
      continue;
    }
    const result = act(board, entity, plan(board, entity));
    if (entity === player && !result.success) break;
    wait(entity, result.moves, result.turns);
  }

  updatePlayerTarget(state);
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
  const {board, player, summon} = state;
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
  const target = state.target?.pokemon;

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

  const shade = (point: Point, glyph: Glyph, force: boolean) => {
    const out_of_range = summon && point.distanceL2(pos) > summon.range - 0.5;
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
    const glyph = x === target ? x.glyph.recolor('black', '440') : x.glyph;
    shade(x.pos, glyph, trainer(x) === player);
  });

  if (summon) {
    const {path, target} = summon;
    const color: Color = summon.error.length === 0 ? '440' : '400';
    recolor(target.x, target.y, 'black', color);

    const length = path.length - 1;
    const midpoint = Math.floor((Constants.SUMMON_ANIM - length) / 2);
    for (let i = -1; i < 1; i++) {
      const index = summon.frame - midpoint + i;
      if (0 <= index && index < length) {
        const [{x, y}, ok] = summon.path[index]!;
        const ch = ray_character(player.pos, summon.target);
        show(x, y, new Glyph(ch, ok ? '440' : '400'), true);
      }
    }
  }

  const effect = board.getEffect();
  if (effect.length) {
    const frame = nonnull(effect[0]);
    frame.forEach(({point: {x, y}, glyph}) => show(x, y, glyph, false));
  }

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

const renderEmptyStatus = (width: int, key?: string): string[] => {
  return ['', Color(`${renderKey(key)}---`, '111'), '', '', ''];
};

const renderBasicPokemonStatus =
    (known: PokemonPublicState, width: int,
     key?: string | null, color?: Color | null): string[] => {
  const {species, hp, pp} = known;
  color = color ? color : known.hp > 0 ? null : '111';

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

  return color ? result.map(x => Color(x, color)) : result;
};

const renderFriendlyPokemonStatus =
    (pokemon: Pokemon, width: int, key?: string | null,
     color?: Color | null, menu: int = -1): string[] => {
  const known = getPokemonPublicState(pokemon);
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

const renderRivalPokemonStatus = (pokemon: Pokemon, width: int): string[] => {
  const known = getPokemonPublicState(pokemon);
  return renderBasicPokemonStatus(known, width);
};

const renderTrainerStatus =
    (trainer: Trainer, width: int, key?: string): string[] => {
  const name = capitalize(describe(trainer));
  const status = trainer.data.pokemon.map(
      x => x.self.cur_hp > 0 ? '*' : Color('*', '111'));
  const hp = trainer.data.cur_hp / Math.max(trainer.data.max_hp, 1);

  const result = [''];
  const prefix = renderKey(key);
  const spacer = ' '.repeat(prefix.length);
  const bar = int(width - prefix.length);
  const hp_color = getHPColor(hp);

  result.push(`${renderKey(key)}${name}`);
  result.push(`${spacer}HP: [${renderBar(bar, hp, hp_color)}]`);
  result.push(`${spacer}     ${status.join(' ')}`);
  result.push('');
  return result;
};

const renderChoice = (state: State): string => {
  if (!state.choice) return kHiddenFlag;

  const kColumnPadding = 2;
  const width = int(Constants.CHOICE_SIZE - 2 * kColumnPadding - 5);

  const result: string[] = [];
  const player = state.player;
  const index = state.choice.index;
  const space = ' '.repeat(kColumnPadding + 1);
  const arrow = ' '.repeat(kColumnPadding) + '>';

  for (let i = 0; i < kPartyKeys.length; i++) {
    const key = kPartyKeys[i]!;
    const pokemon = player.data.pokemon[i];
    const status = (() => {
      if (!pokemon) return renderEmptyStatus(width, key);
      const entity = pokemon.entity;
      return entity ? renderFriendlyPokemonStatus(entity, width, key, '111')
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
  const {board, menu, player} = state;
  const target = state.target?.pokemon;

  const rows: string[] = [];
  const rivals = findRivalPokemon(board, player).filter(x => x !== target);
  rivals.forEach(y => renderRivalPokemonStatus(y, width).forEach(x => rows.push(x)));
  return rows.join('\n');
};

const renderStatus = (state: State): string => {
  const kl = renderKey('a').length;
  const width = int(Constants.STATUS_SIZE + kl);
  const {board, menu, player} = state;
  const vision = board.getVision(player);

  const rows: string[] = [];
  const key = menu ? '-' : kPlayerKey;
  renderTrainerStatus(player, width, key).forEach(x => rows.push(x));

  let summon = 0;
  while (summon < player.data.summons.length) {
    const pokemon = player.data.summons[summon]!;
    const key = menu ? '-' : kSummonedKeys[summon];
    const index = menu && menu.summon === summon ? menu.index : -1;
    renderFriendlyPokemonStatus(pokemon, width, key, null, index)
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
  const width = Constants.STATUS_SIZE;
  const target = state.target;

  if (target === null) {
    const rows = [
      '',
      'No target selected.',
      '',
      '[Tab] cycle through targets',
    ];
    return rows.join('\n');
  }

  const known = target.known;
  const color = known.hp > 0 && !target.stale ? null : '111';
  const rows = renderBasicPokemonStatus(known, width, null, color);

  if (known.pos) {
    const tile = state.board.getTile(known.pos);
    const edit = color ? tile.glyph.recolor() : tile.glyph;
    const text = `Standing on: ${edit.toShortString()} (${tile.description})`;
    rows.push(Color(text, color));
  }
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
  const [tw, th, tl, tt] = [ss + kl, 7, ml + mw + kColSpace, st];
  const wt = tt + th + 2 * kRowSpace + 1;
  const [rw, rh, rl, rt] = [ss + kl, mh - wt - kRowSpace - 1, tl, wt];
  const [lw, lh, ll, lt] = [w, 4, 0, mt + mh + kRowSpace];
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
  let refresh = io.count % 10 === 0;
  refresh = cachedSetContent(io.ui.log, renderLog(io.state)) || refresh;
  refresh = cachedSetContent(io.ui.map, renderMap(io.state)) || refresh;
  refresh = cachedSetContent(io.ui.choice, renderChoice(io.state)) || refresh;
  refresh = cachedSetContent(io.ui.rivals, renderRivals(io.state)) || refresh;
  refresh = cachedSetContent(io.ui.status, renderStatus(io.state)) || refresh;
  refresh = cachedSetContent(io.ui.target, renderTarget(io.state)) || refresh;
  if (!refresh) return;

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
