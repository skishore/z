type int = number;
interface point {x: int, y: int};

const assert = (x: boolean): void => {
  if (!x) throw new Error();
};

const range = (n: int): int[] => {
  const result = [];
  for (let i = 0; i < n; i++) {
    result.push(i);
  }
  return result;
};

//////////////////////////////////////////////////////////////////////////////

const TranThong = (a: point, b: point): point[] => {
  const {x: xa, y: ya} = a;
  const {x: xb, y: yb} = b;
  const result: point[] = [{x: xa, y: ya}];

  const x_diff = Math.abs(xa - xb);
  const y_diff = Math.abs(ya - yb);
  const x_sign = xb < xa ? -1 : 1;
  const y_sign = yb < ya ? -1 : 1;

  let test = 0;
  let [x, y] = [xa, ya];

  if (x_diff >= y_diff) {
    test = Math.floor((x_diff + test) / 2);
    for (let i = 0; i < x_diff; i++) {
      x += x_sign;
      test -= y_diff;
      if (test < 0) {
        y += y_sign;
        test += x_diff;
      }
      result.push({x, y});
    }
  } else {
    test = Math.floor((y_diff + test) / 2);
    for (let i = 0; i < y_diff; i++) {
      y += y_sign;
      test -= x_diff;
      if (test < 0) {
        x += x_sign;
        test += y_diff;
      }
      result.push({x, y});
    }
  }

  return result;
};

interface Node {
  x: int,
  y: int,
  children: Node[],
};

class PrecomputedVisibilityTrie {
  #root: Node;

  constructor(radius: int) {
    this.#root = {x: 0, y: 0, children: []};
    for (let i = 0; i <= radius; i++) {
      for (let j = 0; j < 8; j++) {
        const [xa, ya] = (j & 1) ? [radius, i] : [i, radius];
        const [xb, yb] = [xa * ((j & 2) ? 1 : -1), ya * ((j & 4) ? 1 : -1)];
        const line = TranThong({x: 0, y: 0}, {x: xb, y: yb});
        this.trieUpdate(this.#root, line, 0);
      }
    }
  }

  fieldOfVision(blocked: (p: point) => boolean) {
    const nodes = [this.#root];
    for (let i = 0; i < nodes.length; i++) {
      const node = nodes[i]!;
      if (blocked(node)) continue;
      node.children.forEach(x => nodes.push(x));
    }
  }

  private trieUpdate(node: Node, line: point[], i: int) {
    const [prev, next] = [line[i]!, line[i + 1]];
    assert(node.x === prev.x);
    assert(node.y === prev.y);
    if (!next) return;

    const child = (() => {
      for (const child of node.children) {
        if (child.x === next.x && child.y == next.y) return child;
      }
      const result = {...next, children: []};
      node.children.push(result);
      return result;
    })();
    this.trieUpdate(child, line, i + 1);
  }
};

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  BLOCKED: 0.1,
  FRAME_RATE: 60,
  COLS: 79,
  ROWS: 23,
};

interface State {
  fov: PrecomputedVisibilityTrie,
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
  const fov = new PrecomputedVisibilityTrie(Math.max(COLS, ROWS));
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
    const line = TranThong(source, target)
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
  const shift = Math.floor(1000 * io.timing.length / Constants.FRAME_RATE);
  const delay = Math.max(io.timing[0]!.start + shift - Date.now(), 1);
  setTimeout(tick(io), delay);
};

const main = () => {
  const state = initializeState();
  const io = initializeIO(state);
  tick(io)();
};

main();
