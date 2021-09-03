import {assert, point, range, FOV, LOS} from './lib';

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  BLOCKED: 0.1,
  FRAME_RATE: 60,
  COLS: 79,
  ROWS: 23,
};

interface State {
  fov: FOV,
  map: boolean[][],
  source: point,
  target: point | null,
};

type Input = string;

const add = (a: point, b: point): point => {
  return {x: a.x + b.x, y: a.y + b.y};
};

const addBlocks = (state: State): State => {
  const [width, height] = [state.map.length, state.map[0]!.length];
  const count = Math.floor(width * height * Constants.BLOCKED);
  state.map[state.source.x]![state.source.y] = true;

  for (let i = 0; i < count;) {
    const x = Math.floor(Math.random() * width);
    const y = Math.floor(Math.random() * height);
    if (state.map[x]![y]) continue;
    state.map[x]![y] = true;
    i++;
  }

  state.map[state.source.x]![state.source.y] = false;
  return state;
};

const bounded = (p: point, map: boolean[][]): boolean => {
  const {x, y} = p;
  return 0 <= x && x < map.length && 0 <= y && y < map[0]!.length;
};

const unblocked = (p: point, map: boolean[][]): boolean => {
  return bounded(p, map) && !map[p.x]![p.y];
};

const processInput = (state: State, input: Input) => {
  if (input == 'f') {
    state.target = state.target ? null : state.source;
    return;
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
  if (state.target) {
    const target = add(state.target, delta);
    if (bounded(target, state.map)) state.target = target;
  } else {
    const source = add(state.source, delta);
    if (unblocked(source, state.map)) state.source = source;
  }
};

const initializeState = (): State => {
  const {COLS, ROWS} = Constants;
  const fov = new FOV(Math.max(COLS, ROWS));
  const map = range(COLS).map(_ => range(ROWS).map(_ => false));
  const source = {x: Math.floor(COLS / 2), y: Math.floor(ROWS / 2)};
  return addBlocks({fov, map, source, target: null});
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

  const {fov, map, source, target} = state;
  const blocked = (p: point) => {
    const q = add(p, source);
    if (!bounded(q, map)) return true;
    const result = map[q.x]![q.y]!;
    show(q, result ? '{2-fg}#{/2-fg}' : '.');
    return result;
  };
  fov.fieldOfVision(blocked);
  show(source, '{1-fg}@{/1-fg}');

  if (target) {
    show(target, '{1-fg}*{/1-fg}');
    const line = LOS(source, target)
    const last = line.length - 1;
    line.forEach((p, i) => {
      if (i === 0 || i === last) return;
      show(p, '{1-fg}+{/1-fg}');
    });
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
