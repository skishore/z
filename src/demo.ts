import {assert, flatten, int, point, range, LOS, Glyph} from './lib';

//////////////////////////////////////////////////////////////////////////////

interface Particle {point: point, glyph: Glyph};
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
    frame.forEach(x => result[i]!.push(x));
  }));
  return result;
};

const SerialEffect = (effects: Effect[]): Effect => {
  return flatten(effects);
};

const ExplosionEffect = (point: point): Effect => {
  const {x, y} = point;
  const base = [
    [{point: {x, y}, glyph: Glyph('*', 'red')}],
    [
      {point: {x, y}, glyph: Glyph('+', 'red')},
      {point: {x: x - 1, y}, glyph: Glyph('-', 'red')},
      {point: {x: x + 1, y}, glyph: Glyph('-', 'red')},
      {point: {x, y: y - 1}, glyph: Glyph('|', 'red')},
      {point: {x, y: y + 1}, glyph: Glyph('|', 'red')},
    ],
    [
      {point: {x: x - 1, y}, glyph: Glyph('|', 'red')},
      {point: {x: x + 1, y}, glyph: Glyph('|', 'red')},
      {point: {x, y: y - 1}, glyph: Glyph('-', 'red')},
      {point: {x, y: y + 1}, glyph: Glyph('-', 'red')},
      {point: {x: x - 1, y: y - 1}, glyph: Glyph('/', 'red')},
      {point: {x: x + 1, y: y + 1}, glyph: Glyph('/', 'red')},
      {point: {x: x - 1, y: y + 1}, glyph: Glyph('\\', 'red')},
      {point: {x: x + 1, y: y - 1}, glyph: Glyph('\\', 'red')},
    ],
  ];
  return ExtendEffect(base, 4);
};

const ImplosionEffect = (point: point): Effect => {
  const {x, y} = point;
  const base = [
    [
      {point: {x, y}, glyph: Glyph('*', 'red')},
      {point: {x: x - 1, y: y - 1}, glyph: Glyph('\\', 'red')},
      {point: {x: x + 1, y: y + 1}, glyph: Glyph('\\', 'red')},
      {point: {x: x - 1, y: y + 1}, glyph: Glyph('/', 'red')},
      {point: {x: x + 1, y: y - 1}, glyph: Glyph('/', 'red')},
    ],
    [
      {point: {x, y}, glyph: Glyph('#', 'red')},
      {point: {x: x - 1, y: y - 1}, glyph: Glyph('\\', 'red')},
      {point: {x: x + 1, y: y + 1}, glyph: Glyph('\\', 'red')},
      {point: {x: x - 1, y: y + 1}, glyph: Glyph('/', 'red')},
      {point: {x: x + 1, y: y - 1}, glyph: Glyph('/', 'red')},
    ],
    [{point: {x, y}, glyph: Glyph('*', 'red')}],
    [{point: {x, y}, glyph: Glyph('#', 'red')}],
  ];
  return ExtendEffect(base, 3);
};

const RayEffect = (source: point, target: point, speed: int): Effect => {
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
    result.push(range(i).map(j => ({point: line[j + 1]!, glyph: beam})));
  }
  return result;
};

const SummonEffect = (source: point, target: point, glyph: Glyph): Effect => {
  const base: Effect = [];
  const line = LOS(source, target);
  const ball = Glyph('*', 'red');
  for (let i = 1; i < line.length - 1; i++) {
    base.push([{point: line[i]!, glyph: ball}]);
  }
  const masked = UnderlayEffect(base, {point: target, glyph});
  return SerialEffect([masked, ExplosionEffect(target)]);
};

const WithdrawEffect = (source: point, target: point, glyph: Glyph): Effect => {
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

const SwitchEffect = (source: point, target: point, glyph: Glyph): Effect => {
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

type Map = Tile[][];

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
  map: Map,
  source: point,
  target: point,
  effect: Effect,
};

type Input = string;

const add = (a: point, b: point): point => {
  return {x: a.x + b.x, y: a.y + b.y};
};

const equal = (a: point, b: point): boolean => {
  return a.x === b.x && a.y === b.y;
};

const bounded = (p: point, map: Map): boolean => {
  const {x, y} = p;
  return 0 <= x && x < map.length && 0 <= y && y < map[0]!.length;
};

const unblocked = (p: point, map: Map): boolean => {
  return bounded(p, map) && !map[p.x]![p.y]!.blocked;
};

const processInput = (state: State, input: Input) => {
  if (input === 'f') {
    const glyph = state.map[state.target.x]![state.target.y]!.glyph;
    state.effect = SwitchEffect(state.source, state.target, glyph);
  }
  const deltas: {[key: string]: point} = {
    'y': {x: -1, y: -1},
    'u': {x: 1, y: -1},
    'h': {x: -1, y: 0},
    'j': {x: 0, y: 1},
    'k': {x: 0, y: -1},
    'l': {x: 1, y: 0},
    'b': {x: -1, y: 1},
    'n': {x: 1, y: 1},
  };
  const delta = deltas[input];
  if (!delta) return;
  const source = add(state.source, delta);
  if (unblocked(source, state.map) && !equal(source, state.target)) {
    state.source = source;
  }
};

const initializeState = (): State => {
  const lines = kMap.trim().split('\n');
  const [rows, cols] = [lines.length, lines[0]!.length];
  lines.forEach(x => assert(x.length === cols));

  const source = {x: 0, y: 0};
  const target = {x: 0, y: 0};

  const map = range(cols).map(x => range(rows).map(y => {
    const ch = lines[y]![x]!;
    const tile = kTiles[ch];
    if (tile) return tile;
    switch (ch) {
      case '@': source.x = x; source.y = y; break;
      case 'C': target.x = x; target.y = y; break;
      default: assert(false, () => `Unknown char: ${ch}`);
    }
    return kTiles['.']!;
  }));
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
  const [width, height] = [state.map.length, state.map[0]!.length];
  const text: string[] = Array((width + 1) * height).fill(' ');
  const show = (point: point, glyph: Glyph) => {
    text[point.x + (width + 1) * point.y] = glyph;
  };
  for (let i = 0; i < height; i++) {
    show({x: width, y: i}, Glyph('\n'));
  }

  const {map, source, target} = state;
  map.forEach((line, x) => line.forEach(
    (tile, y) => { show({x, y}, tile.glyph); }));
  show(source, Glyph('@'));
  show(target, Glyph('C', 'red'));

  if (state.effect.length) {
    state.effect[0]!.forEach(({point, glyph}) => show(point, glyph));
  }

  return text.join('');
};

const renderFrameRate = (cpu: number, fps: number): string => {
  return `CPU: ${cpu.toFixed(2)}%; FPS: ${fps.toFixed(2)}  `;
};

const initializeIO = (state: State): IO => {
  const blessed = require('../extern/blessed');
  const screen = blessed.screen({smartCSR: true});

  const [width, height] = [state.map.length, state.map[0]!.length];
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

  const last = io.timing[io.timing.length - 1]!;
  const base = io.timing.reduce((acc, x) => acc += x.end - x.start, 0);

  last.end = Date.now();
  const total = Math.max(last.end - io.timing[0]!.start, 1);
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
  const delay = Math.max(io.timing[0]!.start + shift - Date.now(), 1);
  setTimeout(tick(io), Math.min(delay, Math.floor(1000 / fps)));
};

const main = () => {
  const state = initializeState();
  const io = initializeIO(state);
  tick(io)();
};

main();

export {};
