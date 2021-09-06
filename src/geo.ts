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

const AStarUnitCost = 16;
const AStarDiagonalPenalty = 1;

// Intentionally not admissible to speed up search.
const AStarHeuristic = (a: Point, b: Point): int => {
  const x = Math.abs(a.x - b.x);
  const y = Math.abs(a.y - b.y);
  const min = Math.min(x, y);
  const max = Math.max(x, y);
  return AStarUnitCost * max + 2 * AStarDiagonalPenalty * min;
};

// A simple injection from Z x Z -> Z used as a key for each AStarNode.
const AStarHash = (p: Point): int => {
  const {x, y} = p;
  const s = Math.max(Math.abs(x), Math.abs(y));
  const k = 2 * s + 1;
  return (x + s + k * (y + s + k));
};

class AStarNode extends Point {
  public index: int | null = null;
  constructor(x: int, y: int, public parent: AStarNode | null,
              public distance: int, public score: int) {
    super(x, y);
  }
};

// Min-heap implementation on lists of A* nodes. Nodes track indices as well.
type AStarHeap = AStarNode[];

const AStarHeapCheckInvariants = (heap: AStarHeap): void => {
  return; // Comment this line out to enable debug checks.
  heap.map(x => `(${x.index}, ${x.score})`).join('; ');
  heap.forEach((node, index) => {
    const debug = (label: string) => {
      const contents = heap.map(x => `(${x.index}, ${x.score})`).join('; ');
      return `Violated ${label} at ${index}: ${contents}`;
    };
    assert(node.index === index, () => debug('index'));
    if (index === 0) return;
    const parent_index = Math.floor((index - 1) / 2);
    assert(heap[parent_index]!.score <= node.score, () => debug('ordering'));
  });
};

const AStarHeapPush = (heap: AStarHeap, node: AStarNode): void => {
  assert(node.index === null);
  heap.push(node);
  AStarHeapify(heap, node, heap.length - 1);
}

const AStarHeapify = (heap: AStarHeap, node: AStarNode, index: int): void => {
  assert(0 <= index && index < heap.length);
  const score = node.score;

  while (index > 0) {
    const parent_index = Math.floor((index - 1) / 2);
    const parent = heap[parent_index]!;
    if (parent.score <= score) break;

    heap[index] = parent;
    parent.index = index;
    index = parent_index;
  }

  heap[index] = node;
  node.index = index;
  AStarHeapCheckInvariants(heap);
};

const AStarHeapExtractMin = (heap: AStarHeap): AStarNode => {
  assert(heap.length > 0);
  const result = heap[0]!;
  const node = heap.pop()!;
  result.index = null;

  if (!heap.length) return result;

  let index = 0;
  while (2 * index + 1 < heap.length) {
    const c1 = heap[2 * index + 1]!;
    const c2 = heap[2 * index + 2] || c1;
    if (node.score <= Math.min(c1.score, c2.score)) break;

    const child_index = 2 * index + (c1.score > c2.score ? 2 : 1);
    const child = (c1.score > c2.score ? c2 : c1);
    heap[index] = child;
    child.index = index;
    index = child_index;
  }

  heap[index] = node;
  node.index = index;
  AStarHeapCheckInvariants(heap);
  return result;
};

const AStar = (source: Point, target: Point, blocked: (p: Point) => boolean,
               record?: Point[]): Point[] | null => {
  const map: Map<int, AStarNode> = new Map();
  const heap: AStarHeap = [];

  const score = AStarHeuristic(source, target);
  const node = new AStarNode(source.x, source.y, null, 0, score);
  AStarHeapPush(heap, node);
  map.set(AStarHash(node), node);

  while (heap.length > 0) {
    const cur = AStarHeapExtractMin(heap);
    if (record) record.push(cur);

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

      const diagonal = direction.x !== 0 && direction.y !== 0;
      const addition = AStarUnitCost + (diagonal ? AStarDiagonalPenalty : 0);
      const distance = cur.distance + addition;
      const score = distance + AStarHeuristic(next, target);

      const hash = AStarHash(next);
      const existing = map.get(hash);

      // index !== null is a check to see if we've already popped this node
      // from the heap. We need it because our heuristic is not admissible.
      //
      // Using such a heuristic substantially speeds up search in easy cases,
      // with the downside that we don't always find an optimal path.
      if (existing && existing.index !== null && existing.distance > distance) {
        existing.parent = cur;
        existing.distance = distance;
        existing.score = score;
        AStarHeapify(heap, existing, existing.index);
      } else if (!existing) {
        const created = new AStarNode(next.x, next.y, cur, distance, score);
        AStarHeapPush(heap, created);
        map.set(hash, created);
      }
    }
  }

  return null;
};

//////////////////////////////////////////////////////////////////////////////

export {assert, int, Point, Direction, Matrix, LOS, FOV, AStar};
