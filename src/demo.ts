import {assert, flatten, int, nonnull, range, sample, Glyph} from './lib';
import {Point, Direction, Matrix, LOS, FOV, AStar} from './geo';

//////////////////////////////////////////////////////////////////////////////

interface Vision { dirty: boolean, value: Matrix<int> };

class Board {
  private fov: FOV;
  private map: Matrix<Tile>;
  private entity: Entity[];
  private entityAtPos: Map<int, Entity>;
  private entityIndex: int;
  private entityVision: Map<Entity, Vision>;
  private defaultTile: Tile;

  constructor(size: Point) {
    this.fov = new FOV(Math.max(size.x, size.y));
    this.map = new Matrix(size, nonnull(kTiles['.']));
    this.entity = [];
    this.entityAtPos = new Map();
    this.entityIndex = 0;
    this.entityVision = new Map();
    this.defaultTile = nonnull(kTiles['#']);
  }

  // Reads

  getSize(): Point {
    return this.map.size;
  }

  getTile(pos: Point): Tile {
    return this.map.getOrNull(pos) || this.defaultTile;
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

  free(pos: Point): boolean {
    return !this.getTile(pos).blocked && !this.entityAtPos.has(pos.key());
  }

  // Writes

  setTile(pos: Point, tile: Tile) {
    this.map.set(pos, tile);
    this.entity.forEach(x => this.dirtyVision(x));
  }

  advanceEntity() {
    charge(nonnull(this.entity[this.entityIndex]));
    this.entityIndex = (this.entityIndex + 1) % this.entity.length;;
  }

  addEntity(pos: Point, entity: Entity) {
    assert(this.getEntity(pos) === null);

    this.entityAtPos.set(pos.key(), entity);
    this.entity.push(entity);
    entity.pos = pos;
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

  // Cached field-of-vision

  getVision(entity: Entity): Matrix<int> {
    const vision = (() => {
      const cached = this.entityVision.get(entity);
      if (cached) return cached;
      const result = {dirty: true, value: new Matrix(this.map.size, -1)};
      this.entityVision.set(entity, result);
      return result;
    })();

    if (vision.dirty) {
      const pos = entity.pos;
      const value = vision.value;

      const blocked = (p: Point, parent: Point | null) => {
        const q = p.add(pos);
        const cached = value.getOrNull(q);
        if (cached === null) return true;

        const tile = this.getTile(q);
        const prev = parent ? value.get(parent.add(pos)) : 2;
        const loss = tile.blocked ? 2 : tile.obscure ? 1 : 0;
        const next = Math.max(prev - loss, 0);
        if (next > cached) value.set(q, next);
        return next === 0;
      };

      value.fill(-1);
      value.set(pos, 2);
      this.fov.fieldOfVision(blocked);
      vision.dirty = false;
    }

    return vision.value;
  }

  private dirtyVision(entity: Entity) {
    const vision = this.entityVision.get(entity);
    if (vision) vision.dirty = true;
  }
};

//////////////////////////////////////////////////////////////////////////////

enum AT { Idle, Move, WaitForInput };
enum ET { Pokemon, Trainer };

type Action =
  {type: AT.Idle} |
  {type: AT.WaitForInput} |
  {type: AT.Move, direction: Direction};

interface Result { success: boolean, turns: int };

interface PokemonData {
  trainer: Trainer | null,
};

interface TrainerData {
  input: Action | null,
  player: boolean,
  pokemon: Pokemon[],
};

interface EntityData {
  glyph: Glyph,
  speed: int,
  timer: int,
  pos: Point,
};

type Entity =
  {type: ET.Pokemon, data: PokemonData} & EntityData |
  {type: ET.Trainer, data: TrainerData} & EntityData;

type Pokemon = Entity & {type: ET.Pokemon};
type Trainer = Entity & {type: ET.Trainer};

const charge = (entity: Entity) => {
  entity.timer -= entity.speed;
};

const ready = (entity: Entity): boolean => {
  return entity.timer <= 0;
};

const wait = (entity: Entity, turns: int): void => {
  entity.timer += turns * Constants.TURN_TIMER;
};

const plan = (board: Board, entity: Entity): Action => {
  switch (entity.type) {
    case ET.Pokemon: {
      const {trainer} = entity.data;
      if (!trainer) return {type: AT.Move, direction: sample(Direction.all)};
      const [a, b] = [entity.pos, trainer.pos];
      const sight = () => board.getVision(trainer).get(a) >= 0;
      if (a.distanceSquared(b) < 16 && sight()) return {type: AT.Idle};
      const path = AStar(a, b, x => board.getTile(x).blocked);
      const direction = path
        ? nonnull(path[0]).sub(entity.pos) as Direction
        : sample(Direction.all);
      return {type: AT.Move, direction};
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

const act = (board: Board, entity: Entity, action: Action): Result => {
  switch (action.type) {
    case AT.Idle: return {success: true, turns: 1};
    case AT.WaitForInput: return {success: false, turns: 0};
    case AT.Move: {
      const pos = entity.pos.add(action.direction);
      if (board.getTile(pos).blocked) return {success: false, turns: 0};
      const other = board.getEntity(pos);
      if (other) {
        if (other.type === ET.Pokemon && other.data.trainer === entity) {
          board.swapEntities(entity.pos, pos);
          return {success: true, turns: 1};
        }
        return {success: false, turns: 0};
      }
      board.moveEntity(entity.pos, pos);
      return {success: true, turns: 1};
    }
  }
};

//////////////////////////////////////////////////////////////////////////////

interface Particle {point: Point, glyph: Glyph};
interface Frame extends Array<Particle> {};
interface Effect extends Array<Frame> {};

const ConstantEffect = (particle: Particle, n: int): Effect => {
  return Array(n).fill([particle]);
};

const PauseEffect = (n: int): Effect => {
  return Array(n).fill([]);
};

const OverlayEffect = (effect: Effect, particle: Particle): Effect => {
  return ParallelEffect([effect, ConstantEffect(particle, effect.length)]);
};

const UnderlayEffect = (effect: Effect, particle: Particle): Effect => {
  return ParallelEffect([ConstantEffect(particle, effect.length), effect]);
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

  const ch = (() => {
    const dx = target.x - source.x;
    const dy = target.y - source.y;
    if (Math.abs(dx) > 2 * Math.abs(dy)) return '-';
    if (Math.abs(dy) > 2 * Math.abs(dx)) return '|';
    return ((dx > 0) === (dy > 0)) ? '\\' : '/';
  })();

  const beam = Glyph(ch, 'red');
  const mod = (line.length - 2 + speed) % speed;
  for (let i = mod ? mod : mod + speed; i < line.length - 1; i += speed) {
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
    ParallelEffect([ExtendEffect([full], impl.length), impl]),
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

const SearchEffect = (source: Point, target: Point,
                      blocked: (p: Point) => boolean): Effect => {
  const record: Point[] = [];
  const path = (AStar(source, target, blocked, record) || []).reverse();

  const phase1: Effect = [];
  const g1 = Glyph('?', 'yellow');
  for (let i = 0; i < record.length; i++) {
    phase1.push(range(i + 1).map(j => ({point: record[j]!, glyph: g1})));
  }

  const phase2: Effect = [];
  const g2 = Glyph('*', 'blue');
  for (let i = 0; i < path.length; i++) {
    phase2.push(range(i + 1).map(j => ({point: path[j]!, glyph: g2})));
  }

  const delay = 4;
  const frame = phase1[phase1.length - 1] || [];
  const result = SerialEffect([
    phase1,
    ParallelEffect([
      Array(phase2.length * delay).fill(frame),
      ExtendEffect(phase2, delay),
    ]),
  ]);

  return ParallelEffect([
    result,
    Array(result.length).fill([
      {point: source, glyph: Glyph('@')},
      {point: target, glyph: Glyph('C', 'red')},
    ]),
  ]);
};

export {OverlayEffect, PauseEffect};

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  BLOCKED: 0,
  FRAME_RATE: 60,
  TURN_TIMER: 120,
};

interface Tile {
  blocked: boolean,
  obscure: boolean,
  glyph: Glyph;
};

const kTiles: {[ch: string]: Tile} = {
  '.': {blocked: false, obscure: false, glyph: Glyph('.')},
  '"': {blocked: false, obscure: true, glyph: Glyph('"', 'green', true)},
  '#': {blocked: true, obscure: true, glyph: Glyph('#', 'green')},
  '^': {blocked: true, obscure: true, glyph: Glyph('^', 'yellow')},
};

const kMap = `
........""""""""""##############################
..............."""""""""""""""##################
""""....................""""""""""""""""""""####
"""""""................##.."""""""""""""""""""##
""""##"................##...."""""""""""""""....
""""##.......##...............""""""""""""......
"""""........##...............""""""""""".......
"""..............S............""""""""""........
.............................""""##""""...##....
...............@.................##.......##....
...................C............................
................................................
......##.................##.....................
......##.................##.."""""""............
..........................""""""###"""".........
......................."""""""""######""........
..............##.."""""""""##"""""""""""........
..............##"""""""""#####"""######.........
...........""""""""""""""""""""######...........
......""""""""""""""""""""""""""""""............
""""##""""""""""""""""""""""##"""""""...........
""""##""""""""""""""""""""""##""""""""".........
#################"""""""""""""""""""""""""".....
^^^^^^###############################"""""""""""
^^^^^^^^^^^^^^^^^^^#############################
`;

interface State {
  board: Board,
  effect: Effect,
  player: Trainer,
};

type Input = string;

const addBlocks = (state: State): State => {
  const board = state.board;
  const size = board.getSize();

  let add = Math.floor(size.x * size.y * Constants.BLOCKED);
  for (let x = 0; x < size.x; x++) {
    for (let y = 0; y < size.y; y++) {
      if (board.getTile(new Point(x, y)).blocked) add--;
    }
  }

  const tile = nonnull(kTiles['#']);
  for (let i = 0; i < add; i++) {
    const x = Math.floor(Math.random() * size.x);
    const y = Math.floor(Math.random() * size.y);
    const point = new Point(x, y);
    if (!board.free(point)) continue;
    board.setTile(point, tile);
    i--;
  }
  return state;
};

const processInput = (state: State, input: Input) => {
  const {board, player} = state;
  const others = board.getEntities().filter(x => x !== player);
  const target = nonnull(others[0]);

  if (input === 's') {
    state.effect = SearchEffect(
      player.pos, target.pos, x => board.getTile(x).blocked);
  }
  if (input === 'f') {
    const glyph = board.getTile(target.pos).glyph;
    state.effect = SwitchEffect(player.pos, target.pos, glyph);
  }

  if (player.data.input !== null) return;

  const directions: {[key: string]: Direction} = {
    'h': Direction.w,
    'j': Direction.s,
    'k': Direction.n,
    'l': Direction.e,
    'y': Direction.nw,
    'u': Direction.ne,
    'b': Direction.sw,
    'n': Direction.se,
  };
  const direction = directions[input];
  if (direction) player.data.input = {type: AT.Move, direction};
  if (input === '.') player.data.input = {type: AT.Idle};
};

const initializeState = (): State => {
  const lines = kMap.trim().split('\n');
  const [rows, cols] = [lines.length, nonnull(lines[0]).length];
  lines.forEach(x => assert(x.length === cols));
  const board = new Board(new Point(cols, rows));

  range(cols).forEach(x => range(rows).forEach(y => {
    const ch = nonnull(nonnull(lines[y])[x]);
    const pos = new Point(x, y);
    const tile = kTiles[ch];
    if (tile) return board.setTile(pos, tile);
    const entity = ((): Entity => {
      switch (ch) {
        case '@': {
          const glyph = Glyph('@');
          const speed = Constants.TURN_TIMER / 10;
          const data = {input: null, player: true, pokemon: []};
          return {type: ET.Trainer, data, pos, glyph, speed, timer: 0};
        }
        case 'C': {
          const speed = Constants.TURN_TIMER / 5;
          const [data, glyph] = [{trainer: null}, Glyph('C', 'red')];
          return {type: ET.Pokemon, data, pos, glyph, speed, timer: 0};
        }
        case 'S': {
          const speed = Constants.TURN_TIMER / 4;
          const [data, glyph] = [{trainer: null}, Glyph('S', 'blue')];
          return {type: ET.Pokemon, data, pos, glyph, speed, timer: 0};
        }
        default: {
          assert(false, () => `Unknown char: ${ch}`);
          return null as unknown as Entity;
        }
      }
    })();
    board.addEntity(pos, nonnull(entity));
  }));

  const players = board.getEntities().filter(
    x => x.type === ET.Trainer && x.data.player);
  const player = nonnull(players[0]) as Trainer;

  board.getEntities().forEach(x => {
    if (x.type !== ET.Pokemon || x.data.trainer !== null) return;
    player.data.pokemon.push(x);
    x.data.trainer = player;
  });

  return addBlocks({board, player, effect: []});
};

const updateState = (state: State, inputs: Input[]) => {
  const {board, effect, player} = state;
  if (effect.length) {
    effect.shift();
    return;
  }
  inputs.forEach(x => processInput(state, x));
  inputs.length = 0;

  for (; !effect.length; board.advanceEntity()) {
    const entity = board.getActiveEntity();
    if (!ready(entity)) continue;
    const result = act(board, entity, plan(board, entity));
    if (entity === player && !result.success) return;
    wait(entity, result.turns);
  }
};

//////////////////////////////////////////////////////////////////////////////

declare const console: any;
declare const process: any;
declare const require: any;
declare const setTimeout: any;

interface Element {
  render: () => void;
  setContent: (x: string) => void;
};

interface Timing {
  start: number,
  end: number,
};

interface IO {
  fps: Element,
  map: Element,
  inputs: Input[],
  screen: Element,
  state: State,
  timing: Timing[],
};

const renderMap = (state: State): string => {
  const {board, player} = state;
  const {x: width, y: height} = board.getSize();
  const text: string[] = Array((width + 1) * height).fill(' ');
  const newline = Glyph('\n');
  for (let i = 0; i < height; i++) {
    text[width + (width + 1) * i] = newline;
  }

  const show = (x: int, y: int, glyph: Glyph, force?: boolean) => {
    if (!(0 <= x && x < width && 0 <= y && y < height)) return;
    const index = x + (width + 1) * y;
    if (force || text[index] !== ' ') text[index] = glyph;
  };

  const vision = board.getVision(player);
  for (let x = 0; x < width; x++) {
    for (let y = 0; y < height; y++) {
      const point = new Point(x, y);
      if (vision.get(point) < 0) continue;
      show(x, y, board.getTile(point).glyph, true);
    }
  }

  board.getEntities().forEach(x => {
    const force = x.type === ET.Pokemon && x.data.trainer === player;
    show(x.pos.x, x.pos.y, x.glyph, force);
  });

  if (state.effect.length) {
    const frame = nonnull(state.effect[0]);
    frame.forEach(({point: {x, y}, glyph}) => show(x, y, glyph));
  }

  return text.join('');
};

const renderFrameRate = (cpu: number, fps: number): string => {
  return `CPU: ${cpu.toFixed(2)}%; FPS: ${fps.toFixed(2)}  `;
};

const initializeIO = (state: State): IO => {
  const blessed = require('../extern/blessed');
  const screen = blessed.screen();

  const {x: width, y: height} = state.board.getSize();
  const [attr, left, top, wrap] = [false, 'center', 'center', false];
  const map = blessed.box({attr, height, left, top, width, wrap});
  const fps = blessed.box({align: 'right', top: '100%-1'});
  [fps, map].map(x => screen.append(x));

  const inputs: Input[] = [];
  screen.key(['C-c', 'escape'], () => process.exit(0));
  'hjklyubnfs.'.split('').forEach(x => screen.key([x], () => inputs.push(x)));
  return {fps, map, inputs, screen, state, timing: []};
};

const update = (io: IO) => {
  const start = Date.now();
  io.timing.push({start, end: start});
  if (io.timing.length > Constants.FRAME_RATE) io.timing.shift();
  assert(io.timing.length <= Constants.FRAME_RATE);

  updateState(io.state, io.inputs);
};

const render = (io: IO) => {
  io.map.setContent(renderMap(io.state));

  const last = nonnull(io.timing[io.timing.length - 1]);
  const base = io.timing.reduce((acc, x) => acc += x.end - x.start, 0);

  last.end = Date.now();
  const total = Math.max(last.end - nonnull(io.timing[0]).start, 1);
  const cpu = 100 * (last.end - last.start + base) / total;
  const fps = 1000 * io.timing.length / total;

  io.fps.setContent(renderFrameRate(cpu, fps));
};

const tick = (io: IO) => () => {
  try {
    update(io);
    render(io);
  } catch (error) {
    console.error(error);
  }
  io.screen.render();
  const fps = Constants.FRAME_RATE;
  const shift = Math.floor(1000 * io.timing.length / fps);
  const delay = Math.max(nonnull(io.timing[0]).start + shift - Date.now(), 1);
  setTimeout(tick(io), Math.min(delay, Math.floor(1000 / fps)));
};

const main = () => {
  const state = initializeState();
  const io = initializeIO(state);
  tick(io)();
};

main();

export {};
