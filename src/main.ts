import {assert, nonnull} from './lib';
import {Point, Direction, Matrix, LOS, FOV} from './geo';

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  BLOCKED: 0.1,
  FRAME_RATE: 60,
  COLS: 79,
  ROWS: 23,
};

interface State {
  fov: FOV,
  map: Matrix<boolean>,
  source: Point,
  target: Point | null,
};

type Input = string;

const addBlocks = (state: State): State => {
  const {x: width, y: height} = state.map.size;
  const count = Math.floor(width * height * Constants.BLOCKED);
  state.map.set(state.source, true);

  for (let i = 0; i < count;) {
    const point = new Point(Math.random() * width, Math.random() * height);
    if (state.map.get(point)) continue;
    state.map.set(point, true);
    i++;
  }

  state.map.set(state.source, false);
  return state;
};

const processInput = (state: State, input: Input) => {
  if (input == 'f') {
    state.target = state.target ? null : state.source;
    return;
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
  if (state.target) {
    const target = state.target.add(delta);
    if (state.map.contains(target)) state.target = target;
  } else {
    const source = state.source.add(delta);
    if (state.map.getOrNull(source) === false) state.source = source;
  }
};

const initializeState = (): State => {
  const {COLS, ROWS} = Constants;
  const fov = new FOV(Math.max(COLS, ROWS));
  const map = new Matrix(new Point(COLS, ROWS), false);
  const source = new Point(Math.floor(COLS / 2), Math.floor(ROWS / 2));
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
  const {x: width, y: height} = state.map.size;
  const text: string[] = Array((width + 1) * height).fill(' ');
  const show = (point: Point, glyph: string) => {
    text[point.x + (width + 1) * point.y] = glyph;
  };
  for (let i = 0; i < height; i++) {
    show(new Point(width, i), '\n');
  }

  const {fov, map, source, target} = state;
  const blocked = (p: Point) => {
    const q = p.add(source);
    const result = map.getOrNull(q);
    if (result === null) return true;
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
  const screen = blessed.screen();

  const {x: width, y: height} = state.map.size;
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
