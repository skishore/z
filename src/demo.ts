import {Color, Glyph} from './lib';
import {assert, flatten, int, nonnull, only, range, sample, weighted} from './lib';
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

const shout = (trainer: Trainer, pokemon: Pokemon): string => {
  const {name, player} = trainer.data;
  const prefix = player ? 'You shout' : `${name} shouts`;
  return `${prefix}: "${pokemon.data.self.species.name}, attack!"`;
};

//////////////////////////////////////////////////////////////////////////////

// {Action,Command,Entity,Target}Type enums:
enum AT { Attack, Idle, Move, Shout, Summon, WaitForInput };
enum CT { Attack, };
enum ET { Pokemon, Trainer };
enum TT { Attack, Summon };

type Action =
  {type: AT.Attack, attack: Attack, target: Point} |
  {type: AT.Idle} |
  {type: AT.Move, direction: Direction} |
  {type: AT.Shout, command: Command, entity: Entity} |
  {type: AT.Summon, index: int, target: Point} |
  {type: AT.WaitForInput};

type GenEffect = (board: Board, source: Point, target: Point) => AttackEffect;

interface Result { success: boolean, moves: number, turns: number };

interface Command {type: CT.Attack, attack: Attack, target: Entity | Point};

interface Attack { name: string, range: int, damage: int, effect: GenEffect };

interface AttackEffect { effect: Effect, hitFrame: int };

interface PokemonSpeciesData {
  name: string,
  glyph: Glyph,
  hp: int,
  speed: number,
  attacks: Attack[],
};

interface PokemonIndividualData {
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
  const data = {input: null, name: '', player, pokemon: [], summons: []};
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
  const command = nonnull(commands[0]);
  switch (command.type) {
    case CT.Attack: {
      const [kRange, kLimit] = [command.attack.range, int(6)];
      const {attack, target: ent_or_pt} = command;
      if (!(ent_or_pt instanceof Point) && ent_or_pt.removed) {
        commands.shift();
        return plan(board, entity);
      }

      const source = entity.pos;
      const target = ent_or_pt instanceof Point ? ent_or_pt : ent_or_pt.pos;
      if (move_ready(entity) && hasLineOfSight(board, source, target, kRange)) {
        commands.shift();
        return {type: AT.Attack, attack, target};
      }

      const check = board.getStatus.bind(board);
      const found = (x: Point) => hasLineOfSight(board, x, target, kRange);
      const dirs = BFS(source, found, kLimit, check);
      const options = dirs.map(x => source.add(x));
      if (found(source)) options.push(source);

      if (options.length > 0) {
        const scores = options.map(
            x => Math.abs(x.distanceNethack(target) - kRange));
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
    }
  }
};

const checkFollowerSquare = (board: Board, leader: Entity, pos: Point,
                             ignoreOccupant?: boolean): boolean => {
  const status = board.getStatus(pos);
  const free = status === Status.FREE ||
               (ignoreOccupant && status === Status.OCCUPIED);
  if (!free) return false;

  const dn = leader.pos.distanceNethack(pos);
  if (dn > 2) return false;

  const vision = board.getVision(leader);
  const max = vision.getOrNull(leader.pos);
  const vn = vision.getOrNull(pos) ?? -1;
  return (dn <= 1 && vn > 0) || (dn === 2 && vn === max);
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
  const direction = path
    ? Direction.assert(nonnull(path[0]).sub(ep))
    : sample(Direction.all);
  return {type: AT.Move, direction};
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
      const {commands, self: {species, trainer}} = entity.data;
      if (commands.length > 0) return followCommands(board, entity, commands);

      const rivals = findRivals(board, entity);
      if (rivals.length > 0 && species.attacks.length > 0) {
        const target = sample(rivals).pos;
        const attack = sample(species.attacks);
        const commands: Command[] = [{type: CT.Attack, attack, target}];
        return followCommands(board, entity, commands);
      }
      if (trainer) return followLeader(board, entity, trainer);
      return {type: AT.Move, direction: sample(Direction.all)};
    }
    case ET.Trainer: {
      const {input, player} = entity.data;
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
      const {attack, target} = action;
      const attack_effect = attack.effect(board, entity.pos, target);
      board.addEffect(ApplyAttack(board, attack_effect, target));
      const user = capitalize(describe(entity));

      const target_entity = board.getEntity(target);
      if (target_entity?.type !== ET.Pokemon) {
        board.log(`${user} used ${attack.name}!`);
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
      board.moveEntity(entity.pos, pos);
      entity.dir = action.direction;
      return kSuccess;
    }
    case AT.Shout: {
      const listener = action.entity;
      if (listener.type !== ET.Pokemon) return kFailure;
      if (trainer(listener) !== entity) return kFailure;
      listener.data.commands.push(action.command);
      board.log(shout(entity, listener));
      return kSuccess;
    }
    case AT.Summon: {
      const {index, target} = action;
      if (entity.type !== ET.Trainer) return kFailure;
      const pokemon = entity.data.pokemon[index];
      if (!pokemon || pokemon.entity || !pokemon.self.cur_hp) return kFailure;
      if (entity.data.summons.length >= kSummonedKeys.length) return kFailure;
      if (board.getStatus(target) !== Status.FREE) return kFailure;
      const glyph = board.getTile(target).glyph;
      const summoned = makePokemon(target, pokemon.self);
      entity.data.summons.push(summoned);
      pokemon.entity = summoned;
      board.addEntity(summoned);
      board.addEffect(SummonEffect(entity.pos, target, glyph));
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
      } else if (player.pos.distanceNethack(point) > range) {
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
  MAP_SIZE:      int(47),
  FOV_RADIUS:    int(24),
  WORLD_SIZE:    int(100),
  STATUS_SIZE:   int(80),
  CHOICE_SIZE:   int(48),
  FRAME_RATE:    int(60),
  MOVE_TIMER:    int(960),
  TURN_TIMER:    int(120),
  SUMMON_ANIM:   int(60),
  SUMMON_RANGE:  int(12),
  TRAINER_SPEED: 1/10,
};

interface Tile {
  readonly blocked: boolean,
  readonly obscure: boolean,
  readonly glyph: Glyph,
};

const kPlayerKey = 'a';
const kSummonedKeys = 'sdf';
const kPartyKeys = `${'a'}sdfgh`;
const kDirectionKeys = 'kulnjbhy';
const kAlphabetKeys = 'abcdefghijklmnopqrstuvwxyz';

const kTiles: {[ch: string]: Tile} = {
  '.': {blocked: false, obscure: false, glyph: new Glyph('.')},
  '"': {blocked: false, obscure: true,  glyph: new Glyph('"', '231')},
  '#': {blocked: true,  obscure: true,  glyph: new Glyph('#', '010')},
};

const kAttacks: Attack[] = [
  {name: 'Ember',    range: 12, damage: int(40), effect: EmberEffect},
  {name: 'Ice Beam', range: 12, damage: int(60), effect: IceBeamEffect},
  {name: 'Blizzard', range: 12, damage: int(80), effect: BlizzardEffect},
  {name: 'Headbutt', range: 8,  damage: int(80), effect: HeadbuttEffect},
  {name: 'Tackle',   range: 4,  damage: int(40), effect: HeadbuttEffect},
];

const species = (name: string, hp: int, speed: number,
                 attack_names: string[], glyph: Glyph): PokemonSpeciesData => {
  attack_names = attack_names.slice();
  attack_names.push('Tackle');
  const attacks = attack_names.map(x=> only(kAttacks.filter(y => y.name === x)));
  return {name, glyph, hp, speed, attacks};
};

const kPokemon: PokemonSpeciesData[] = [
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
};

type Choice = {
  index: int,
};

type Summon = {
  error: string,
  frame: int
  index: int,
  path: [Point, boolean][],
  range: int,
  target: Point,
};

const outsideMap = (state: State, point: Point): boolean => {
  const delta = point.sub(state.player.pos);
  const limit = Math.floor((Constants.MAP_SIZE - 1) / 2);
  return Math.abs(delta.x) > limit || Math.abs(delta.y) > limit;
};

const processInput = (state: State, input: Input): void => {
  const {board, player, choice, summon} = state;
  const enter = input === 'enter' || input === '.';

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
        state.board.logMenu(Color(`${name} is already out!`, '422'));
      } else if (!pokemon.self.cur_hp) {
        state.board.logMenu(Color(`${name} has no strength left!`, '422'));
      } else {
        const range = Constants.SUMMON_RANGE;
        state.board.logMenu(Color(`Choose where to send out ${name}:`, '234'));
        state.summon = initSummon(state, chosen, range);
        state.choice = null;
      }
    } else if (input === 'escape') {
      state.board.logMenu(Color('Canceled.', '234'));
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
        state.board.logMenu(Color(summon.error, '422'));
      } else {
        const {index, target} = summon;
        const name = player.data.pokemon[index]?.self.species.name;
        state.board.logMenu(Color(`Go! ${name}!`, '231'), true);
        player.data.input = {type: AT.Summon, index, target};
        state.summon = null;
      }
    } else if (input === 'escape') {
      state.board.logMenu(Color('Canceled.', '234'));
      state.summon = null;
    }
    return;
  }

  if (player.data.input !== null) return;

  const summoned = kSummonedKeys.indexOf(input);
  if (summoned >= player.data.summons.length) {
    state.choice = {index: 0};
    state.board.logMenu(Color('Choose a Pokemon to send out with J/K:', '234'));
  }

  const direction = Direction.all[kDirectionKeys.indexOf(input)];
  if (direction) player.data.input = {type: AT.Move, direction};
  if (input === '.') player.data.input = {type: AT.Idle};
};

const initializeBoard = (): Board => {
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

const initializeState = (): State => {
  const start = int(Math.floor((Constants.WORLD_SIZE + 1) / 2));
  const point = new Point(start, start);
  const board = (() => {
    while (true) {
      const board = initializeBoard();
      if (!board.getTile(point).blocked) return board;
    }
  })();
  const player = makeTrainer(point, true);
  board.addEntity(player);

  const n = Math.min(kPartyKeys.length, 5);
  kPokemon.slice(0, n).forEach(species => {
    const self = {species, trainer: player, cur_hp: int(100), max_hp: int(100)};
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
    const species = sample(kPokemon.slice(n));
    const data = {species, trainer: null, cur_hp: int(50), max_hp: int(50)};
    board.addEntity(makePokemon(pos, data));
  }

  return {board, player, choice: null, summon: null};
};

const updateState = (state: State, inputs: Input[]): void => {
  const {board, player} = state;
  if (board.advanceEffect()) return;

  const active = board.getActiveEntity();
  while (inputs.length && !board.getEffect().length &&
         active === player && player.data.input === null) {
    processInput(state, nonnull(inputs.shift()));
  }
  if (state.summon) return animateSummon(state, state.summon);

  while (!board.getEffect().length) {
    const entity = board.getActiveEntity();
    if (!turn_ready(entity)) {
      board.advanceEntity();
      continue;
    }
    const result = act(board, entity, plan(board, entity));
    if (entity === player && !result.success) return;
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
  status: Element,
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
  const width  = Constants.MAP_SIZE;
  const height = Constants.MAP_SIZE;
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

  const shade = (point: Point, glyph: Glyph, force: boolean) => {
    const out_of_range = summon && point.distanceNethack(pos) > summon.range;
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
    shade(x.pos, x.glyph, trainer(x) === player);
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

const renderBar = (width: int, value: number, color: Color | null): string => {
  const total = width - 6;
  const chars = value > 0 ? Math.max(1, Math.round(value * total)) : 0;
  return Color('='.repeat(chars), color) + ' '.repeat(total - chars);
};

const renderKey = (key?: string): string => {
  return key ? `[${key}] ` : '';
};

const renderEmptyStatus = (width: int, key?: string): string[] => {
  return ['', Color(`${renderKey(key)}---`, '111'), '', '', ''];
};

const renderPokemonStatus = (self: PokemonIndividualData, width: int,
                             key?: string, color?: Color | null): string[] => {
  const hp = self.cur_hp / Math.max(self.max_hp, 1);
  color = color ? color : hp > 0 ? null : '111';
  const pp = 1;

  const result = [''];
  const prefix = renderKey(key);
  const spacer = ' '.repeat(prefix.length);
  const bar = int(width - prefix.length);
  result.push(`${prefix}${self.species.name}`);
  result.push(`${spacer}HP: [${renderBar(bar, hp, color ? null : '020')}]`);
  result.push(`${spacer}PP: [${renderBar(bar, pp, color ? null : '234')}]`);
  result.push('');

  return color ? result.map(x => x.length ? Color(x, color) : x) : result;
};

const renderEntityStatus =
    (entity: Entity, width: int, key?: string): string[] => {
  if (entity.type === ET.Pokemon) {
    return renderPokemonStatus(entity.data.self, width, key);
  }

  const name = entity.data.player ? 'You' : entity.data.name;
  const status = entity.data.pokemon.map(
      x => x.self.cur_hp > 0 ? '*' : Color('*', '111'));

  const result = [''];
  const prefix = renderKey(key);
  const spacer = ' '.repeat(prefix.length);
  const bar = int(width - prefix.length);
  result.push(`${renderKey(key)}${name}`);
  result.push(`${spacer}HP: [${renderBar(bar, 1, '020')}]`);
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
      const color = pokemon.entity ? '111' : null;
      return renderPokemonStatus(pokemon.self, width, key, color)
    })();
    status.forEach((line, j) => {
      const selected = i === index;
      const use_arrow = selected && j === 1;
      result.push(`${use_arrow ? arrow : space}${selected ? ' ' : ''}${line}`);
    });
  }
  return result.join('\n');
};

const renderStatus = (state: State): string => {
  const kNumColumns = 2;
  const kColumnPadding = 2;
  const w = Constants.STATUS_SIZE;
  const h = Constants.MAP_SIZE + 2;
  const extra = renderKey('a').length;
  const rwidth = int(Math.floor((w - extra) / kNumColumns) - 2 * kColumnPadding);
  const lwidth = int(rwidth + extra);

  const player = state.player;
  const vision = state.board.getVision(player);

  const lcol: string[] = [];
  const rcol: string[] = [];
  const key = kPlayerKey;
  renderEntityStatus(state.player, lwidth, key).forEach(x => lcol.push(x));

  let index = 0;
  for (const pokemon of player.data.summons) {
    const self = pokemon.data.self;
    const key = kSummonedKeys[index++];
    renderPokemonStatus(self, lwidth, key).forEach(x => lcol.push(x));
  }

  while (index < kSummonedKeys.length) {
    const key = kSummonedKeys[index++]!;
    renderEmptyStatus(lwidth, key).forEach(x => lcol.push(x));
  }

  const source = player.pos;
  const pokemon: Pokemon[] = [];
  state.board.getEntities().forEach(entity => {
    if (entity.type !== ET.Pokemon) return;
    if (entity.data.self.trainer === player) return;
    if ((vision.getOrNull(entity.pos) ?? -1) < 0) return;
    if (outsideMap(state, entity.pos)) return;
    pokemon.push(entity);
  });

  pokemon.sort((entity_a, entity_b): number => {
    const a = entity_a.pos, b = entity_b.pos;
    const base = a.distanceSquared(source) - b.distanceSquared(source);
    const xopt = a.x - b.x, yopt = a.y - b.y;
    return base || xopt || yopt;
  });
  pokemon.slice(0, 32).forEach(
      entity => renderEntityStatus(entity, rwidth).forEach(x => rcol.push(x)));

  const rows: string[] = [];
  const space = ' '.repeat(kColumnPadding);
  const nrows = Math.max(lcol.length, rcol.length);
  const pad = (x: string | undefined, w: int): string => {
    x = x || '';
    const padding = w - x.replace(/\x1b\[[\d;]*m/g, '').length;
    return `${space}${x}${' '.repeat(padding)}${space}`;
  };
  for (let i = 0; i < nrows; i++) {
    rows.push(`${pad(lcol[i], lwidth)}${pad(rcol[i], rwidth)}`);
  }
  return rows.join('\n');
};

const initializeIO = (state: State): IO => {
  const blessed = require('../extern/blessed');
  const window = blessed.screen({fullUnicode: true});

  const kSpacer = 1;
  const x = Constants.MAP_SIZE;
  const y = Constants.MAP_SIZE;
  const w = 2 * x + 2 + kSpacer + Constants.STATUS_SIZE;
  const h = y + 2 + kSpacer + Constants.LOG_SIZE;

  const [lw, lh, ll, lt] = [w - 4, Constants.LOG_SIZE, 2, 0];
  const [mw, mh, ml, mt] = [2 * x + 2, y + 2, 0, lh + lt + kSpacer];
  const [sw, sh, sl, st] = [Constants.STATUS_SIZE, mh, mw + kSpacer, mt];
  const [left, top, attr, wrap] = ['center', 'center', false, false];
  const content = blessed.box({width: w, height: h, left, top, attr, wrap});

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

  const log = element(lw, lh, ll, lt);
  const map = element(mw, mh, ml, mt, {border: {type: 'line'}});
  const status = element(sw, sh, sl, st);
  const choice = element(cw, ch, cl, ct, {border: {type: 'line'}});
  const fps = blessed.box({align: 'right', top: '100%-1'});
  [content, fps].map(x => window.append(x));
  const ui: UI = {choice, fps, log, map, status, window};

  const inputs: Input[] = [];
  window.key(['S-a', 'C-c'], () => process.exit(0));
  const kFastDirectionKeys = Array.from(kDirectionKeys).map(x => `S-${x}`);
  const kAllKeys = ['enter', 'escape', '.']
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
  refresh = cachedSetContent(io.ui.status, renderStatus(io.state)) || refresh;
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
  const state = initializeState();
  const io = initializeIO(state);
  tick(io)();
};

main();

export {};
