import {assert, point, range} from './lib';

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  FRAME_RATE: 60,
};

type Glyph = string;

type Map = Tile[][];

interface Tile {
  blocked: boolean,
  glyph: Glyph;
};

const kTiles: {[ch: string]: Tile} = {
  '.': {blocked: false, glyph: '.'},
  '"': {blocked: false, glyph: '{10-fg}"{/10-fg}'},
  '#': {blocked: true, glyph: '{2-fg}#{/2-fg}'},
  '^': {blocked: true, glyph: '{3-fg}^{/3-fg}'},
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
  return {map, source, target};
};

const updateState = (state: State, inputs: Input[]) => {
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
  const show = (p: point, glyph: string) => {
    text[p.x + (width + 1) * p.y] = glyph;
  };
  for (let i = 0; i < height; i++) {
    show({x: width, y: i}, '\n');
  }

  const {map, source, target} = state;
  map.forEach((line, x) => line.forEach(
    (tile, y) => { show({x, y}, tile.glyph); }));
  show(source, '@');
  show(target, '{1-fg}C{/1-fg}');
  return text.join('');
};

const renderFrameRate = (cpu: number, fps: number): string => {
  return `CPU: ${cpu.toFixed(2)}%; FPS: ${fps.toFixed(2)}  `;
};

const initializeIO = (state: State): IO => {
  const blessed = require('../extern/blessed');
  const screen = blessed.screen({smartCSR: true});

  const [width, height] = [state.map.length, state.map[0]!.length];
  const [left, top, tags, wrap] = ['center', 'center', true, false];
  const map = blessed.box({height, left, top, tags, width, wrap});
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
