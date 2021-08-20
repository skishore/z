const assert = (x: boolean) => { if (!x) { throw new Error(); } };

//////////////////////////////////////////////////////////////////////////////

type int = number;
interface point {x: int, y: int};

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

//////////////////////////////////////////////////////////////////////////////

const Constants = {
  FRAME_RATE: 60,
  COLS: 79,
  ROWS: 23,
};

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
  fps: Element;
  map: Element;
  screen: Element;
  timing: Timing[];
};

const renderFrameRate = (cpu: number, fps: number) => {
  return `CPU: ${cpu.toFixed(2)}%; FPS: ${fps.toFixed(2)}  `;
};

const initializeIO = (): IO => {
  const blessed = require('../extern/blessed');
  const screen = blessed.screen({smartCSR: true});

  const {COLS: width, ROWS: height} = Constants;
  const [border, left, top] = ['line', 'center', 'center'];
  const map = blessed.box({border, height, left, top, width});
  const fps = blessed.box({align: 'right', top: '100%-1'});
  [fps, map].map(x => screen.append(x));

  screen.key(['C-c', 'escape'], () => process.exit(0));
  return {fps, map, screen, timing: []};
};

const updateState = (io: IO) => {
  const start = Date.now();
  io.timing.push({start, end: start});
  if (io.timing.length > Constants.FRAME_RATE) io.timing.shift();
  assert(io.timing.length <= Constants.FRAME_RATE);
};

const renderState = (io: IO) => {
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
    updateState(io);
    renderState(io);
  } catch (error) {
    console.error(error);
  }
  io.screen.render();
  const shift = Math.floor(1000 * io.timing.length / Constants.FRAME_RATE);
  const delay = Math.max(io.timing[0]!.start + shift - Date.now(), 1);
  setTimeout(tick(io), delay);
};

const main = () => {
  const io = initializeIO();
  tick(io)();
};

main();
