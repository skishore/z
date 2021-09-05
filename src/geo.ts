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

class FOVNode extends Point {
  public readonly children: FOVNode[];
  constructor(x: int, y: int) { super(x, y); this.children = []; }
};

class FOV {
  #root: FOVNode;

  constructor(radius: int) {
    this.#root = new FOVNode(0, 0);
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

  private trieUpdate(node: FOVNode, line: Point[], i: int) {
    const [prev, next] = [line[i]!, line[i + 1]];
    assert(node.x === prev.x);
    assert(node.y === prev.y);
    if (!next) return;

    const child = (() => {
      for (const child of node.children) {
        if (child.x === next.x && child.y == next.y) return child;
      }
      const result = new FOVNode(next.x, next.y);
      node.children.push(result);
      return result;
    })();
    this.trieUpdate(child, line, i + 1);
  }
};

//////////////////////////////////////////////////////////////////////////////
// Pathfinding: breadth-first-search and A-star.

const AStarDiagonalCost = 17;
const AStarStraightCost = 16;

// Correctness below relies on the fact that this heuristic is consistent.
const AStarHeuristic = (a: Point, b: Point): int => {
  const x = Math.abs(a.x - b.x);
  const y = Math.abs(a.y - b.y);
  const diagonal = Math.min(x, y);
  const straight = Math.max(x, y) - diagonal;
  return diagonal * AStarDiagonalCost + straight * AStarStraightCost;
};

// A simple injection from Z x Z -> Z used as a key for each AStarNode.
const AStarHash = (p: Point): int => {
  const {x, y} = p;
  const s = Math.max(Math.abs(x), Math.abs(y));
  const k = 2 * s + 1;
  return (x + s + k * (y + s + k));
};

class AStarNode extends Point {
  constructor(x: int, y: int, public parent: AStarNode | null,
              public distance: int, public score: int) {
    super(x, y);
  }
};

const AStar = (source: Point, target: Point, blocked: (p: Point) => boolean): Point[] | null => {
  const map: Map<int, AStarNode> = new Map();
  const frontier: AStarNode[] = [];

  const score = AStarHeuristic(source, target);
  const node = new AStarNode(source.x, source.y, null, 0, score);
  frontier.push(node);
  map.set(AStarHash(node), node);

  while (frontier.length > 0) {
    frontier.sort((x, y) => y.score - x.score);
    const cur = frontier.pop()!;

    if (cur.equal(target)) {
      let current: AStarNode | null = cur;
      const result: Point[] = [];
      while (current) {
        result.push(current);
        current = current.parent;
      }
      return result.reverse();
    }

    for (const direction of Direction.all) {
      const next = cur.add(direction);
      if (blocked(next)) continue;

      const distance = cur.distance + 1;
      const score = distance * AStarDiagonalCost + AStarHeuristic(next, target);
      assert(score >= cur.score);

      const hash = AStarHash(next);
      const existing = map.get(hash);
      if (existing && existing.distance > distance) {
        existing.parent = cur;
        existing.distance = distance;
        existing.score = score;
      } else if (!existing) {
        const created = new AStarNode(next.x, next.y, cur, distance, score);
        frontier.push(created);
        map.set(hash, created);
      }
    }
  }

  return null;
};

//////////////////////////////////////////////////////////////////////////////

export {assert, int, Point, Direction, Matrix, LOS, FOV, AStar};
