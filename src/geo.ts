import {assert, int} from './lib';

//////////////////////////////////////////////////////////////////////////////
// Simple 2D geometry helpers.

class Point {
  readonly x: int;
  readonly y: int;

  constructor(x: int, y: int) {
    this.x = x | 0;
    this.y = y | 0;
  }

  add(o: Point): Point { return new Point(this.x + o.x, this.y + o.y); }
  sub(o: Point): Point { return new Point(this.x - o.x, this.y - o.y); }

  equal(o: Point): boolean { return this.x === o.x && this.y === o.y; }

  toString(): string { return `Point(${this.x}, ${this.y})`; }
};

class Direction extends Point {
  static none = new Direction(0, 0);
  static n    = new Direction(0, -1);
  static ne   = new Direction(1, -1);
  static e    = new Direction(1, 0);
  static se   = new Direction(1, 1);
  static s    = new Direction(0, 1);
  static sw   = new Direction(-1, 1);
  static w    = new Direction(-1, 0);
  static nw   = new Direction(-1, -1);

  static all = [Direction.n, Direction.ne, Direction.e, Direction.se,
                Direction.s, Direction.sw, Direction.w, Direction.nw];

  static cardinal = [Direction.n, Direction.e, Direction.s, Direction.w];

  static diagonal = [Direction.ne, Direction.se, Direction.sw, Direction.nw];

  private constructor(x: int, y: int) { super(x, y); }
};

class Matrix<T> {
  readonly size: Point;
  #data: T[];

  constructor(size: Point, value: T) {
    this.size = size;
    this.#data = Array(size.x * size.y).fill(value);
  }

  get(point: Point): T {
    if (!this.contains(point)) throw new Error(`${point} not in ${this.size}`);
    return this.#data[point.x + this.size.x * point.y]!;
  }

  set(point: Point, value: T): void {
    if (!this.contains(point)) throw new Error(`${point} not in ${this.size}`);
    this.#data[point.x + this.size.x * point.y] = value;
  }

  getXY(x: int, y: int): T {
    x = x | 0;
    y = y | 0;
    const {x: sx, y: sy} = this.size;
    if (!(0 <= x && x < sx && 0 <= y && y < sy)) {
      throw new Error(`${new Point(x, y)} not in ${this.size}`);
    }
    return this.#data[x + this.size.x * y]!;
  }

  getOrNull(point: Point): T | null {
    if (!this.contains(point)) return null;
    return this.#data[point.x + this.size.x * point.y]!;
  }

  contains(point: Point): boolean {
    const {x: px, y: py} = point;
    const {x: sx, y: sy} = this.size;
    return 0 <= px && px < sx && 0 <= py && py < sy;
  }
};

//////////////////////////////////////////////////////////////////////////////
// Tran-Thong symmetric line-of-sight calculation.

const LOS = (a: Point, b: Point): Point[] => {
  const {x: xa, y: ya} = a;
  const {x: xb, y: yb} = b;
  const result: Point[] = [a];

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
      result.push(new Point(x, y));
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
      result.push(new Point(x, y));
    }
  }

  return result;
};

//////////////////////////////////////////////////////////////////////////////
// Pre-computed visibility trie, for field-of-vision computation.

class Node extends Point {
  public children: Node[];
  constructor(x: int, y: int) { super(x, y); this.children = []; }
};

class FOV {
  #root: Node;

  constructor(radius: int) {
    this.#root = new Node(0, 0);
    for (let i = 0; i <= radius; i++) {
      for (let j = 0; j < 8; j++) {
        const [xa, ya] = (j & 1) ? [radius, i] : [i, radius];
        const [xb, yb] = [xa * ((j & 2) ? 1 : -1), ya * ((j & 4) ? 1 : -1)];
        const line = LOS(new Point(0, 0), new Point(xb, yb));
        this.trieUpdate(this.#root, line, 0);
      }
    }
  }

  fieldOfVision(blocked: (p: Point) => boolean) {
    const nodes = [this.#root];
    for (let i = 0; i < nodes.length; i++) {
      const node = nodes[i]!;
      if (blocked(node)) continue;
      node.children.forEach(x => nodes.push(x));
    }
  }

  private trieUpdate(node: Node, line: Point[], i: int) {
    const [prev, next] = [line[i]!, line[i + 1]];
    assert(node.x === prev.x);
    assert(node.y === prev.y);
    if (!next) return;

    const child = (() => {
      for (const child of node.children) {
        if (child.x === next.x && child.y == next.y) return child;
      }
      const result = new Node(next.x, next.y);
      node.children.push(result);
      return result;
    })();
    this.trieUpdate(child, line, i + 1);
  }
};

//////////////////////////////////////////////////////////////////////////////

export {assert, int, Point, Direction, Matrix, LOS, FOV};
