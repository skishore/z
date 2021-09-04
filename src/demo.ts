import {assert, flatten, int, nonnull, range, Glyph} from './lib';
import {Point, Direction, Matrix, LOS} from './geo';

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

assert(!!{OverlayEffect, ParallelEffect, PauseEffect});

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  FRAME_RATE: 60,
};

interface Tile {
  blocked: boolean,
  glyph: Glyph;
};

const kTiles: {[ch: string]: Tile} = {
  '.': {blocked: false, glyph: Glyph('.')},
  '"': {blocked: false, glyph: Glyph('"', 'green', true)},
  '#': {blocked: true, glyph: Glyph('#', 'green')},
  '^': {blocked: true, glyph: Glyph('^', 'yellow')},
};

const kMap = `
........""""""""""##############################
..............."""""""""""""""##################
""""....................""""""""""""""""""""####
"""""""...................."""""""""""""""""""##
"""""""......................"""""""""""""""....
""""".........................""""""""""""......
""""".........................""""""""""".......
"""...........................""""""""""........
............................."""""""""".........
............................."""""""""..........
...................C.........""""""""...........
............................""""""".............
............................""""""..............
....@......................"""""""..............
..........................""""""""..............
.......................""""""""""...............
.................."""""""""""""""...............
..............""""""""""""""""""""..............
..........."""""""""""""""""""""""..............
......""""""""""""""""""""""""""""""............
"""""""""""""""""""""""""""""""""""""...........
""""""""""""""""""""""""""""""""""""""".........
#################"""""""""""""""""""""""""".....
^^^^^^###############################"""""""""""
^^^^^^^^^^^^^^^^^^^#############################
`;

interface State {
  map: Matrix<Tile>,
  source: Point,
  target: Point,
  effect: Effect,
};

type Input = string;

const processInput = (state: State, input: Input) => {
  if (input === 'f') {
    const glyph = state.map.get(state.target).glyph;
    state.effect = SwitchEffect(state.source, state.target, glyph);
  }
  const deltas: {[key: string]: Direction} = {
    'h': Direction.w,
    'j': Direction.s,
    'k': Direction.n,
    'l': Direction.e,
    'y': Direction.nw,
    'u': Direction.ne,
    'b': Direction.sw,
    'n': Direction.se,
  };
  const delta = deltas[input];
  if (!delta) return;
  const source = state.source.add(delta);
  const tile = state.map.getOrNull(source);
  if (tile && !tile.blocked && !source.equal(state.target)) {
    state.source = source;
  }
};

const initializeState = (): State => {
  const lines = kMap.trim().split('\n');
  const [rows, cols] = [lines.length, nonnull(lines[0]).length];
  lines.forEach(x => assert(x.length === cols));

  let source: Point | null = null;
  let target: Point | null = null;

  const map = new Matrix(new Point(cols, rows), nonnull(kTiles['.']));
  range(cols).forEach(x => range(rows).forEach(y => {
    const ch = nonnull(nonnull(lines[y])[x]);
    const tile = kTiles[ch];
    if (tile) return map.set(new Point(x, y), tile);
    switch (ch) {
      case '@': source = new Point(x, y); break;
      case 'C': target = new Point(x, y); break;
      default: assert(false, () => `Unknown char: ${ch}`);
    }
  }));

  source = nonnull(source);
  target = nonnull(target);
  return {map, source, target, effect: []};
};

const updateState = (state: State, inputs: Input[]) => {
  if (state.effect.length) {
    state.effect.shift();
    return;
  }
  inputs.forEach(x => processInput(state, x));
  inputs.length = 0;
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
  const {x: width, y: height} = state.map.size;
  const text: string[] = Array((width + 1) * height).fill(' ');
  const show = (x: int, y: int, glyph: Glyph) => {
    text[x + (width + 1) * y] = glyph;
  };
  for (let i = 0; i < height; i++) {
    show(width, i, Glyph('\n'));
  }

  const {map, source, target} = state;
  for (let x = 0; x < map.size.x; x++) {
    for (let y = 0; y < map.size.y; y++) {
      show(x, y, map.getXY(x, y).glyph);
    }
  }
  show(source.x, source.y, Glyph('@'));
  show(target.x, target.y, Glyph('C', 'red'));

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
  const screen = blessed.screen({smartCSR: true});

  const {x: width, y: height} = state.map.size;
  const [left, top, wrap] = ['center', 'center', false];
  const map = blessed.box({height, left, top, width, wrap});
  const fps = blessed.box({align: 'right', top: '100%-1'});
  [fps, map].map(x => screen.append(x));

  const inputs: Input[] = [];
  screen.key(['C-c', 'escape'], () => process.exit(0));
  'hjklyubnf'.split('').forEach(x => screen.key([x], () => inputs.push(x)));
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
