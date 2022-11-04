import {assert, flatten, int, nonnull, range, sample, weighted, Color, Glyph} from './lib';
import {Point, Direction, Matrix, LOS, FOV, AStar, BFS, Status} from './geo';

//////////////////////////////////////////////////////////////////////////////

interface Vision { dirty: boolean, blockers: Point[], value: Matrix<int> };

class Board {
  private fov: FOV;
  private map: Matrix<Tile>;
  private effect: Effect;
  private entity: Entity[];
  private entityAtPos: Map<int, Entity>;
  private entityIndex: int;
  private entityVision: Map<Entity, Vision>;
  private defaultTile: Tile;
  private logs: string[];

  constructor(size: Point) {
    this.fov = new FOV(int(Math.max(size.x, size.y)));
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
    return this.logs;
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

  log(line: string) {
    this.logs.push(line);
    if (this.logs.length > Constants.LOG_SIZE) this.logs.shift();
  }

  logIfPlayer(entity: Entity, line: string) {
    if (entity.type === ET.Trainer && entity.data.player) this.log(line);
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
          const kVisionRadius = 3;
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

const describe = (entity: Entity): string => {
  if (entity.type === ET.Trainer) {
    const {name, player} = entity.data;
    return player ? 'you' : name;
  } else {
    const {species: {name: species}, trainer} = entity.data.self;
    if (!trainer) return `the wild ${species}`;
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

interface Result { success: boolean, turns: number };

interface Command {type: CT.Attack, attack: Attack, target: Point};

interface Attack {
  name: string,
  range: int,
  effect: (board: Board, source: Point, target: Point) => Effect,
};

interface PokemonSpeciesData {
  glyph: Glyph;
  name: string;
  speed: number;
};

interface PokemonIndividualData {
  species: PokemonSpeciesData,
  trainer: Trainer | null,
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
  player: boolean,
  pokemon: PokemonEdge[],
  name: string,
};

interface EntityData {
  glyph: Glyph,
  speed: number,
  timer: int,
  pos: Point,
};

type Entity =
  {type: ET.Pokemon, data: PokemonData} & EntityData |
  {type: ET.Trainer, data: TrainerData} & EntityData;

type Pokemon = Entity & {type: ET.Pokemon};
type Trainer = Entity & {type: ET.Trainer};

const charge = (entity: Entity) => {
  entity.timer -= Math.round(Constants.TURN_TIMER * entity.speed);
};

const ready = (entity: Entity): boolean => {
  return entity.timer <= 0;
};

const trainer = (entity: Entity): Trainer | null => {
  return entity.type === ET.Pokemon ? entity.data.self.trainer : null;
};

const wait = (entity: Entity, turns: number): void => {
  entity.timer += Math.round(Constants.TURN_TIMER * turns);
};

const makePokemon = (pos: Point, self: PokemonIndividualData): Pokemon => {
  const {glyph, speed} = self.species;
  const data = {commands: [], self};
  return {type: ET.Pokemon, data, pos, glyph, speed, timer: 0};
};

const makeTrainer = (pos: Point, player: boolean): Trainer => {
  const glyph = Glyph('@');
  const speed = Constants.TRAINER_SPEED;
  const data = {input: null, player, pokemon: [], name: ''};
  return {type: ET.Trainer, data, pos, glyph, speed, timer: 0};
};

const hasLineOfSight =
    (board: Board, source: Point, target: Point, range: int): boolean => {
  if (source.distanceNethack(target) > range) return false;

  const line = LOS(source, target);
  const last = line.length - 1;
  return line.every((x, i) => {
    return i === 0 || i === last || board.getStatus(x) === Status.FREE;
  });
};

const followCommands =
    (board: Board, entity: Entity, commands: Command[]): Action => {
  const command = nonnull(commands[0]);
  switch (command.type) {
    case CT.Attack: {
      const [kRange, kLimit] = [command.attack.range, int(6)];
      const {attack, target} = command;
      const source = entity.pos;
      if (hasLineOfSight(board, source, target, kRange)) {
        commands.shift();
        return {type: AT.Attack, attack, target};
      }

      const check = board.getStatus.bind(board);
      const found = (x: Point) => hasLineOfSight(board, x, target, kRange);
      const dirs = BFS(source, found, kLimit, check);
      if (dirs.length > 0) return {type: AT.Move, direction: sample(dirs)};

      const path = AStar(source, target, check);
      const direction = path
        ? Direction.assert(nonnull(path[0]).sub(source))
        : sample(Direction.all);
      return {type: AT.Move, direction};
    }
  }
};

const followLeader = (board: Board, entity: Entity, leader: Entity): Action => {
  const [ep, tp] = [entity.pos, leader.pos];
  const vision = board.getVision(leader);
  const max = vision.getOrNull(tp);
  const okay = (pos: Point): boolean => {
    if (!pos.equal(ep) && board.getEntity(pos)) return false;
    const dn = tp.distanceNethack(pos);
    if (dn > 2) return false;
    const vn = vision.getOrNull(pos) || 0;
    return (dn <= 1 && vn > 0) || (dn === 2 && vn === max);
  };
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

const plan = (board: Board, entity: Entity): Action => {
  switch (entity.type) {
    case ET.Pokemon: {
      const {commands, self: {trainer}} = entity.data;
      if (commands.length > 0) return followCommands(board, entity, commands);
      return trainer ? followLeader(board, entity, trainer)
                     : {type: AT.Move, direction: sample(Direction.all)};
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

const kSuccess: Result = {success: true, turns: 1};
const kFailure: Result = {success: false, turns: 1};

const act = (board: Board, entity: Entity, action: Action): Result => {
  switch (action.type) {
    case AT.Attack: {
      const {attack, target} = action;
      board.addEffect(attack.effect(board, entity.pos, target));
      board.log(`${describe(entity)} used ${attack.name}!`);
      return kSuccess;
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
        return kSuccess;
      }
      board.moveEntity(entity.pos, pos);
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
      if (!pokemon || pokemon.entity) return kFailure;
      if (board.getStatus(target) !== Status.FREE) return kFailure;
      const glyph = board.getTile(target).glyph;
      pokemon.entity = makePokemon(target, pokemon.self);
      board.addEntity(nonnull(pokemon.entity));
      board.addEffect(SummonEffect(entity.pos, target, glyph));
      return kSuccess;
    }
    case AT.WaitForInput: return kFailure;
  }
};

//////////////////////////////////////////////////////////////////////////////

const findOptionAtDirection =
    (board: Board, blocked: boolean, pos: Point, dir: Direction,
     range: int, vision: Matrix<int>): Point => {
  let prev = pos;
  range = range > 0 ? range : int(255);

  while (true) {
    const next = prev.add(dir);
    if (pos.distanceNethack(next) > range) return prev;
    const sight = vision.getOrNull(next);
    if (sight === null || sight < 0) return prev;
    if (board.getTile(next).blocked) return blocked ? next : prev;
    prev = next;
  }
};

const findOptions =
    (board: Board, source: Entity, type: TT, range: int): Options => {
  const summon = type === TT.Summon;
  const options: Options = new Map();
  const trainer = source.type === ET.Pokemon && source.data.self.trainer;
  const entity = trainer || source;
  const vision = board.getVision(entity);

  const ep = entity.pos;
  const sp = entity.pos;

  Direction.all.forEach((dir, i) => {
    const point = findOptionAtDirection(board, !summon, sp, dir, range, vision);
    if (point.equal(sp)) return;
    options.set(nonnull(kDirectionKeys[i]), {hidden: true, point});
  });
  if (summon) return options;

  const kMinDistance = 4;
  const kMinAngle = Math.PI / 12;

  const p = entity.pos;
  const blockers = board.getBlockers(entity).slice();
  blockers.sort((a, b) => a.distanceSquared(p) - b.distanceSquared(p));

  const used: Point[] = [];
  for (let i = 0, j = 0; i < kAllKeys.length && j < blockers.length; i++) {
    const key = nonnull(kAllKeys[i]);
    if (kDirectionKeys.includes(key)) continue;
    while (j < blockers.length) {
      const point = nonnull(blockers[j++]);
      const found =
        point.distanceNethack(ep) >= kMinDistance &&
        used.every(x => point.distanceNethack(x) >= kMinDistance &&
                        point.sub(ep).angle(x.sub(ep)) >= kMinAngle);
      if (!found) continue;
      options.set(key, {hidden: false, point});
      used.push(point);
      break;
    }
  }
  return options;
};

const targetsForAttack =
    (board: Board, source: Entity, attack: Attack): Target => {
  const type = TT.Attack;
  const options = findOptions(board, source, type, attack.range);
  return {type, options, source, attack};
};

const targetsForSummon = (board: Board, source: Trainer, index: int): Target => {
  const type = TT.Summon;
  const options = findOptions(board, source, type, Constants.SUMMON_RANGE);
  return {type, options, index};
};

const resolveTarget = (base: Target, target: Point): Action => {
  switch (base.type) {
    case TT.Attack: {
      const attack = base.attack;
      if (base.source.type as ET === ET.Trainer) {
        return {type: AT.Attack, attack, target};
      }
      const command = {type: CT.Attack, attack, target};
      return {type: AT.Shout, command, entity: base.source};
    }
    case TT.Summon: return {type: AT.Summon, index: base.index, target};
  }
};

//////////////////////////////////////////////////////////////////////////////

interface Particle {point: Point, glyph: Glyph};
interface Frame extends Array<Particle> {};
interface Effect extends Array<Frame> {};

type Sparkle = [int, string, Color?, boolean?][];

const add_particle = (effect: Effect, frame: int, particle: Particle) => {
  while (frame >= effect.length) effect.push([]);
  nonnull(effect[frame]).push(particle);
};

const add_sparkle =
    (effect: Effect, sparkle: Sparkle, frame: int, point: Point) => {
  sparkle.forEach(x => {
    const [delay, chars, color, light] = x;
    for (let i = 0; i < delay; i++, frame++) {
      const index = Math.floor(Math.random() * chars.length);
      const glyph = Glyph(nonnull(chars[index]), color, light);
      add_particle(effect, frame, {glyph, point});
    }
  });
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
  const base = [
    [{point, glyph: Glyph('*', 'red')}],
    [
      {point, glyph: Glyph('+', 'red')},
      {point: point.add(Direction.n), glyph: Glyph('|', 'red')},
      {point: point.add(Direction.s), glyph: Glyph('|', 'red')},
      {point: point.add(Direction.e), glyph: Glyph('-', 'red')},
      {point: point.add(Direction.w), glyph: Glyph('-', 'red')},
    ],
    [
      {point: point.add(Direction.n), glyph: Glyph('-', 'red')},
      {point: point.add(Direction.s), glyph: Glyph('-', 'red')},
      {point: point.add(Direction.e), glyph: Glyph('|', 'red')},
      {point: point.add(Direction.w), glyph: Glyph('|', 'red')},
      {point: point.add(Direction.ne), glyph: Glyph('\\', 'red')},
      {point: point.add(Direction.sw), glyph: Glyph('\\', 'red')},
      {point: point.add(Direction.nw), glyph: Glyph('/', 'red')},
      {point: point.add(Direction.se), glyph: Glyph('/', 'red')},
    ],
  ];
  return ExtendEffect(base, 4);
};

const ImplosionEffect = (point: Point): Effect => {
  const base = [
    [
      {point, glyph: Glyph('*', 'red')},
      {point: point.add(Direction.ne), glyph: Glyph('/', 'red')},
      {point: point.add(Direction.sw), glyph: Glyph('/', 'red')},
      {point: point.add(Direction.nw), glyph: Glyph('\\', 'red')},
      {point: point.add(Direction.se), glyph: Glyph('\\', 'red')},
    ],
    [
      {point, glyph: Glyph('#', 'red')},
      {point: point.add(Direction.ne), glyph: Glyph('/', 'red')},
      {point: point.add(Direction.sw), glyph: Glyph('/', 'red')},
      {point: point.add(Direction.nw), glyph: Glyph('\\', 'red')},
      {point: point.add(Direction.se), glyph: Glyph('\\', 'red')},
    ],
    [{point, glyph: Glyph('*', 'red')}],
    [{point, glyph: Glyph('#', 'red')}],
  ];
  return ExtendEffect(base, 3);
};

const RayEffect = (source: Point, target: Point, speed: int): Effect => {
  const result: Effect = [];
  const line = LOS(source, target);
  if (line.length <= 2) return result;

  const beam = Glyph(ray_character(source, target), 'red');
  const mod = int((line.length - 2 + speed) % speed);
  const start = int(mod ? mod : mod + speed);
  for (let i = start; i < line.length - 1; i = int(i + speed)) {
    result.push(range(i).map(j => ({point: nonnull(line[j + 1]), glyph: beam})));
  }
  return result;
};

const SummonEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  const base: Effect = [];
  const line = LOS(source, target);
  const ball = Glyph('*', 'red');
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

const EmberEffect = (board: Board, source: Point, target: Point) => {
  const effect: Effect = [];
  const line = LOS(source, target);

  const trail = (): Sparkle => [
    [random_delay(0), '*^^', 'red'],
    [random_delay(1), '*^', 'yellow'],
    [random_delay(2), '**^', 'yellow', true],
    [random_delay(3), '**^#%', 'yellow'],
    [random_delay(4), '#%', 'red'],
  ];
  const flame = (): Sparkle => [
    [random_delay(0), '*^^', 'red'],
    [random_delay(1), '*^', 'yellow'],
    [random_delay(2), '**^#%', 'yellow', true],
    [random_delay(3), '*^#%', 'yellow'],
    [random_delay(4), '*^#%', 'red'],
  ];

  for (let i = 1; i < line.length - 1; i++) {
    const frame = int(Math.floor((i - 1) / 2));
    add_sparkle(effect, trail(), frame, nonnull(line[i]));
  }
  for (const direction of [Direction.none].concat(Direction.all)) {
    const norm = direction.distanceTaxicab(Direction.none);
    const frame = int(2 * norm + Math.floor((line.length - 1) / 2));
    add_sparkle(effect, flame(), frame, target.add(direction));
  }
  return effect;
};

const IceBeamEffect = (board: Board, source: Point, target: Point) => {
  const effect: Effect = [];
  const line = LOS(source, target);
  const ch = ray_character(source, target);

  const trail: Sparkle = [
    [2, ch, 'white'],
    [2, ch, 'cyan'],
    [2, ch, 'blue'],
  ];
  const flame: Sparkle = [
    [2, '*', 'white'],
    [2, '*', 'cyan'],
    [2, '*', 'blue'],
    [2, '*', 'white'],
    [2, '*', 'cyan'],
    [2, '*', 'blue'],
  ];

  for (let i = 1; i < line.length; i++) {
    const frame = int(Math.floor((i - 1) / 2));
    add_sparkle(effect, trail, frame, nonnull(line[i]));
  }
  const frame = int(Math.floor((line.length - 1) / 2));
  add_sparkle(effect, flame, frame, target);
  return effect;
};

const BlizzardEffect = (board: Board, source: Point, target: Point) => {
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
    [1, ch, 'white'],
    [1, ch, 'cyan'],
    [1, ch, 'blue'],
  ];
  const flame: Sparkle = [
    [2, '*', 'white'],
    [2, '*', 'cyan'],
    [2, '*', 'blue'],
    [2, '*', 'white'],
    [2, '*', 'cyan'],
    [2, '*', 'blue'],
  ];

  for (let p = 0; p < points.length; p++) {
    const d = 12 * p;
    const next = nonnull(points[p]);
    const line = LOS(source, next);
    for (let i = 1; i < line.length; i++) {
      add_sparkle(effect, trail, int(d + i), nonnull(line[i]));
    }
    add_sparkle(effect, flame, int(d + line.length - 1), next);
  }
  return effect.filter((_, i) => i % 3);
};

const HeadbuttEffect = (board: Board, source: Point, target: Point) => {
  const source_tile = board.getTile(source);
  const source_entity = board.getEntity(source);
  const glyph = source_entity ? source_entity.glyph : source_tile.glyph;
  const ch = ray_character(source, target);

  const trail = (): Sparkle => [
    [8, '#', 'white'],
  ];

  const line = LOS(source, target);

  const move_along_line = (line: Point[]) => {
    const effect: Effect = [];
    for (let i = 1; i < line.length - 1; i++) {
      const point = nonnull(line[i]);
      add_particle(effect, int(i), {point, glyph});
      add_sparkle(effect, trail(), int(i + 1), point);
    }
    return effect;
  };

  const hold_pause = PauseEffect(int(line.length - 2));
  const hold_point = line[line.length - 2] || source;
  const hold_effect = ConstantEffect({point: hold_point, glyph}, int(32));

  const towards = move_along_line(line);
  const hold    = SerialEffect([hold_pause, hold_effect]);
  const away    = move_along_line(line.reverse());

  const result = UnderlayEffect(
      SerialEffect([ParallelEffect([towards, hold]), away]),
      {point: source, glyph: source_tile.glyph});

  return result.filter((_, i) => i % 2);
};

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  LOG_SIZE:      int(4),
  MAP_SIZE:      int(37),
  STATUS_SIZE:   int(80),
  FRAME_RATE:    int(60),
  TURN_TIMER:    int(120),
  SUMMON_RANGE:  int(3),
  TRAINER_SPEED: 1/10,
};

interface Tile {
  blocked: boolean,
  obscure: boolean,
  glyph: Glyph,
};

const kPokemonKeys = 'sdfwer';
const kDirectionKeys = 'kulnjbhy';
const kAllKeys = 'abcdefghijklmnopqrstuvwxyz';

const kTiles: {[ch: string]: Tile} = {
  '.': {blocked: false, obscure: false, glyph: Glyph('.')},
  '"': {blocked: false, obscure: true, glyph: Glyph('"', 'green', true)},
  '#': {blocked: true, obscure: true, glyph: Glyph('#', 'green')},
};

const kPokemon: {[key: string]: PokemonSpeciesData} = {
  B: {name: 'Bulbasaur', glyph: Glyph('B', 'green'), speed: 1/6},
  C: {name: 'Charmander', glyph: Glyph('C', 'red'), speed: 1/5},
  S: {name: 'Squirtle', glyph: Glyph('S', 'blue'), speed: 1/4},
  E: {name: 'Eevee', glyph: Glyph('E', 'yellow'), speed: 1/5},
  P: {name: 'Pikachu', glyph: Glyph('P', 'yellow', true), speed: 1/4},
};

const kAttacks: Attack[] = [
  {name: 'Ember', range: 12, effect: EmberEffect},
  {name: 'Ice Beam', range: 12, effect: IceBeamEffect},
  {name: 'Blizzard', range: 12, effect: BlizzardEffect},
  {name: 'Headbutt', range: 8, effect: HeadbuttEffect},
];

type Input = string;

interface State {
  board: Board,
  player: Trainer,
  target: Target | null,
};

interface Option {
  hidden: boolean;
  point: Point;
};

type Options = Map<string, Option>;

type Target =
  {type: TT.Attack, options: Options, source: Entity, attack: Attack} |
  {type: TT.Summon, options: Options, index: int};

const processInput = (state: State, input: Input) => {
  const {board, player} = state;

  if (state.target) {
    const target = state.target;
    const option = target.options.get(input);
    if (option) {
      player.data.input = resolveTarget(target, option.point);
      state.target = null;
    } else if (input === 'escape') {
      state.target = null;
    }
    return;
  }

  if (player.data.input !== null) return;

  const index = int(kPokemonKeys.indexOf(input));
  if (0 <= index && index < player.data.pokemon.length) {
    const pokemon = nonnull(player.data.pokemon[index]).entity;
    state.target = pokemon
      ? targetsForAttack(board, pokemon, sample(kAttacks))
      : targetsForSummon(board, player, index);
  }

  if (input === 'a') {
    const attack = sample(kAttacks);
    state.target = targetsForAttack(board, player, attack);
  }

  const direction = Direction.all[kDirectionKeys.indexOf(input)];
  if (direction) player.data.input = {type: AT.Move, direction};
  if (input === '.') player.data.input = {type: AT.Idle};
};

const initializeBoard = (): Board => {
  const base = Constants.MAP_SIZE;
  const size = new Point(base, base);
  const board = new Board(size);

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
  const start = int(Math.floor((Constants.MAP_SIZE + 1) / 2));
  const point = new Point(start, start);
  let board: Board | null = null;
  while (!board || board.getTile(point).blocked) {
    board = initializeBoard();
  }
  const player = makeTrainer(point, true);
  board.addEntity(player);

  const n = kPokemonKeys.length;
  Object.entries(kPokemon).slice(0, n).forEach(([_, species]) => {
    player.data.pokemon.push({entity: null, self: {species, trainer: player}});
  });

  return {board, player, target: null};
};

const updateState = (state: State, inputs: Input[]) => {
  const {board, player} = state;
  if (board.advanceEffect()) return;

  while (inputs.length && !board.getEffect().length) {
    processInput(state, nonnull(inputs.shift()));
  }

  while (!board.getEffect().length) {
    const entity = board.getActiveEntity();
    if (!ready(entity)) {
      board.advanceEntity();
      continue;
    }
    const result = act(board, entity, plan(board, entity));
    if (entity === player && !result.success) return;
    wait(entity, result.turns);
  }
};

//////////////////////////////////////////////////////////////////////////////

declare const console: any;
declare const process: any;
declare const require: any;

declare const setTimeout: (fn: () => void, delay: number) => void;

interface Element {
  render: () => void;
  setContent: (x: string) => void;
  cachedContent: string | null,
};

interface Timing {
  start: number,
  end: number,
};

interface IO {
  fps: Element,
  log: Element,
  map: Element,
  status: Element,
  inputs: Input[],
  screen: Element,
  state: State,
  timing: Timing[],
  count: int,
};

const kBreakGlyph = Glyph('\n');
const kEmptyGlyph = Glyph(' ');

const renderFrameRate = (cpu: number, fps: number): string => {
  return `CPU: ${cpu.toFixed(2)}%; FPS: ${fps.toFixed(2)}  `;
};

const renderLog = (state: State): string => {
  return state.board.getLog().join('\n');
};

const renderMap = (state: State): string => {
  const {board, player} = state;
  const {x: width, y: height} = board.getSize();
  const text: string[] = Array((width + 1) * height).fill(kEmptyGlyph);
  for (let i = 0; i < height; i++) {
    text[width + (width + 1) * i] = kBreakGlyph;
  }

  const show = (x: int, y: int, glyph: Glyph, force?: boolean) => {
    if (!(0 <= x && x < width && 0 <= y && y < height)) return;
    const index = x + (width + 1) * y;
    if (force || text[index] !== kEmptyGlyph) text[index] = glyph;
  };

  const vision = board.getVision(player);
  for (let x = int(0); x < width; x++) {
    for (let y = int(0); y < height; y++) {
      const point = new Point(x, y);
      if (vision.get(point) < 0) continue;
      show(x, y, board.getTile(point).glyph, true);
    }
  }

  board.getEntities().forEach(x => {
    show(x.pos.x, x.pos.y, x.glyph, trainer(x) === player);
  });

  // TODO(kshaunak): Use a matching algorithm like the Hungarian algorithm to
  // select label directions here. Precompute the match on action selection.
  if (state.target) {
    for (const [key, option] of state.target.options.entries()) {
      assert(key.length === 1);
      const {hidden, point: {x, y}} = option;
      if (hidden) continue;
      const label = key.toUpperCase();
      const index = x + (width + 1) * y;
      text[index] = `\x1b[41m${text[index]}\x1b[0m`;
      const dash = `\x1b[31m-\x1b[0m`;
      const name = `\x1b[1;31m${label}\x1b[0m`;
      const left = x === width - 1 || (0 < x && x < player.pos.x);
      left ? show(int(x - 1), y, `${name}${dash}` as Glyph, true)
           : show(int(x + 1), y, `${dash}${name}` as Glyph, true);
    }
  }

  const effect = board.getEffect();
  if (effect.length) {
    const frame = nonnull(effect[0]);
    frame.forEach(({point: {x, y}, glyph}) => show(x, y, glyph));
  }

  return text.join('');
};

const renderStatus = (state: State): string => {
  const w = Constants.STATUS_SIZE;
  const h = state.board.getSize().y + 2;

  const kPadding = 2;
  const kPokemonPerRow = 3;
  const kRows = Math.ceil(kPokemonKeys.length / kPokemonPerRow);
  const outer = Math.floor(w / kPokemonPerRow);
  const width = outer - 2 * kPadding;

  const row_space = 3;
  const row_total = (3 + row_space) * kRows - row_space
  const top_space = Math.floor((h - row_total) / 2);
  const lines: string[] = [];
  for (let i = 0; i < kRows; i++) {
    const space = i === 0 ? top_space : row_space;
    ['\n'.repeat(space - 1), '', '', ''].forEach(x => lines.push(x));
  }

  const base = Math.floor((w - outer * kPokemonPerRow) / 2) + kPadding;
  const append = (i: int, j: int, text: string) => {
    const k = j + 4 * (kRows - Math.floor(i / kPokemonPerRow)) - 3;
    const left = base + outer * (i % kPokemonPerRow);
    const padding = left - lines[k]!.replace(/\x1b\[[\d;]*m/g, '').length;
    if (padding > 0) lines[k] += ' '.repeat(padding);
    lines[k] += text;
  };

  Array.from(kPokemonKeys).forEach((key, ii) => {
    const i = int(ii);
    const pokemon = state.player.data.pokemon[i] || null;
    const entity = pokemon ? pokemon.entity : null;
    const header = `${key}) ${pokemon ? pokemon.self.species.name : '---'}`;
    if (entity) {
      append(i, 0, header);
      append(i, 1, `HP: [${Color('='.repeat(width - 6), 'green')}]`);
      append(i, 2, `PP: [${Color('='.repeat(width - 6), 'blue')}]`);
    } else if (pokemon) {
      append(i, 0, Color(header, 'white'));
      append(i, 1, Color(`HP: [${'='.repeat(width - 6)}]`, 'white'));
      append(i, 2, Color(`PP: [${'='.repeat(width - 6)}]`, 'white'));
    } else {
      append(i, 0, Color(header, 'white'));
    }
  });

  return lines.join('\n');
};

const initializeIO = (state: State): IO => {
  const blessed = require('../extern/blessed');
  const screen = blessed.screen({fullUnicode: true});

  const kSpacer = 1;
  const {x, y} = state.board.getSize();
  const h = x + 2 + kSpacer + Constants.LOG_SIZE;
  const w = 2 * x + 2 + kSpacer + Constants.STATUS_SIZE;

  const [lw, lh, ll, lt] = [w - 4, Constants.LOG_SIZE, 2, 0];
  const [mw, mh, ml, mt] = [2 * x + 2, y + 2, 0, lh + lt + kSpacer];
  const [sw, sh, sl, st] = [Constants.STATUS_SIZE, mh, mw + kSpacer, mt];
  const [left, top, attr, wrap] = ['center', 'center', false, false];
  const content = blessed.box({width: w, height: h, left, top, attr, wrap});

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
  const fps = blessed.box({align: 'right', top: '100%-1'});
  [content, fps].map(x => screen.append(x));

  const inputs: Input[] = [];
  screen.key(['C-c'], () => process.exit(0));
  ['escape', '.'].concat(Array.from(kAllKeys)).forEach(
    x => screen.key([x], () => inputs.push(x)));
  return {fps, log, map, status, inputs, screen, state, timing: [], count: 0};
};

const update = (io: IO) => {
  const start = Date.now();
  io.timing.push({start, end: start});
  if (io.timing.length > Constants.FRAME_RATE) io.timing.shift();
  assert(io.timing.length <= Constants.FRAME_RATE);
  io.count++;

  updateState(io.state, io.inputs);
};

const cachedSetContent = (element: Element, content: string): boolean => {
  if (content === element.cachedContent) return false;
  element.cachedContent = content;
  element.setContent(content);
  return true;
};

const render = (io: IO) => {
  let refresh = io.count % 10 === 0;
  refresh = cachedSetContent(io.log, renderLog(io.state)) || refresh;
  refresh = cachedSetContent(io.map, renderMap(io.state)) || refresh;
  refresh = cachedSetContent(io.status, renderStatus(io.state)) || refresh;
  if (!refresh) return;

  const last = nonnull(io.timing[io.timing.length - 1]);
  const base = io.timing.reduce((acc, x) => acc += x.end - x.start, 0);

  last.end = Date.now();
  const total = Math.max(last.end - nonnull(io.timing[0]).start, 1);
  const cpu = 100 * (last.end - last.start + base) / total;
  const fps = 1000 * io.timing.length / total;

  io.fps.setContent(renderFrameRate(cpu, fps));
  io.screen.render();
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
