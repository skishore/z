import {assert, int, nonnull} from './lib';

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

  distanceL2(o: Point): number { return Math.sqrt(this.distanceSquared(o)); }

  distanceNethack(o: Point): int {
    const dx = Math.abs(this.x - o.x);
    const dy = Math.abs(this.y - o.y);
    const min = Math.min(dx, dy);
    const max = Math.max(dx, dy);
    return Math.floor((46 * min + 95 * max + 25) / 100);
  }

  distanceSquared(o: Point): int {
    const dx = this.x - o.x;
    const dy = this.y - o.y;
    return dx * dx + dy * dy;
  }

  distanceTaxicab(o: Point): int {
    const dx = this.x - o.x;
    const dy = this.y - o.y;
    return Math.abs(dx) + Math.abs(dy);
  }

  distanceWalking(o: Point): int {
    const dx = this.x - o.x;
    const dy = this.y - o.y;
    return Math.max(Math.abs(dx), Math.abs(dy));
  }

  equal(o: Point): boolean { return this.x === o.x && this.y === o.y; }

  // An injection from Z x Z -> Z suitable for use as a Map key.
  // We can also try better ones with less multiplication and branching.
  key(): int {
    const {x, y} = this;
    const s = Math.max(Math.abs(x), Math.abs(y));
    const k = 2 * s + 1;
    return (x + s + k * (y + s + s * k));
  }

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

  static assert(point: Point): Direction {
    if (point.equal(Direction.none)) return Direction.none;
    return nonnull(Direction.all.filter(x => x.equal(point))[0]);
  }
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

  fill(value: T): void {
    this.#data.fill(value);
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
  public readonly parent: FOVNode | null;
  public readonly children: FOVNode[];
  constructor(x: int, y: int, parent: FOVNode | null) {
    super(x, y);
    this.parent = parent;
    this.children = [];
  }
};

class FOV {
  #root: FOVNode;

  constructor(radius: int) {
    this.#root = new FOVNode(0, 0, null);
    for (let i = 0; i <= radius; i++) {
      for (let j = 0; j < 8; j++) {
        const [xa, ya] = (j & 1) ? [radius, i] : [i, radius];
        const [xb, yb] = [xa * ((j & 2) ? 1 : -1), ya * ((j & 4) ? 1 : -1)];
        const line = LOS(new Point(0, 0), new Point(xb, yb));
        this.trieUpdate(this.#root, line, 0);
      }
    }
  }

  fieldOfVision(blocked: (p: Point, parent: Point | null) => boolean) {
    const nodes = [this.#root];
    for (let i = 0; i < nodes.length; i++) {
      const node = nodes[i]!;
      if (blocked(node, node.parent)) continue;
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
      const result = new FOVNode(next.x, next.y, node);
      node.children.push(result);
      return result;
    })();
    this.trieUpdate(child, line, i + 1);
  }
};

//////////////////////////////////////////////////////////////////////////////
// Pathfinding: breadth-first-search and A-star.

const AStarUnitCost = 16;
const AStarDiagonalPenalty = 2;
const AStarLOSDeltaPenalty = 1;

// "delta" penalizes paths that travel far from the direct line-of-sight
// from the source to the target. In order to compute it, we figure out if
// this line is "more horizontal" or "more vertical", then compute the the
// distance from the point to this line orthogonal to this main direction.
//
// Adding this term to our heuristic means that it's no longer admissible,
// but it provides two benefits that are enough for us to use it anyway:
//
//   1. By breaking score ties, we expand the fronter towards T faster than
//      we would with a consistent heuristic. We complete the search sooner
//      at the cost of not always finding an optimal path.
//
//   2. By biasing towards line-of-sight, we select paths that are visually
//      more appealing than alternatives (e.g. that interleave cardinal and
//      diagonal steps, rather than doing all the diagonal steps first).
//
const AStarHeuristic = (p: Point, los: Point[]): int => {
  const {x: px, y: py} = p;
  const {x: sx, y: sy} = los[0]!;
  const {x: tx, y: ty} = los[los.length - 1]!;

  const delta = (() => {
    const dx = tx - sx;
    const dy = ty - sy;
    const l = los.length - 1;
    if (Math.abs(dx) > Math.abs(dy)) {
      const index = dx > 0 ? px - sx : sx - px;
      if (index < 0) return Math.abs(px - sx) + Math.abs(py - sy);
      if (index > l) return Math.abs(px - tx) + Math.abs(py - ty);
      return Math.abs(py - los[index]!.y);
    } else {
      const index = dy > 0 ? py - sy : sy - py;
      if (index < 0) return Math.abs(px - sx) + Math.abs(py - sy);
      if (index > l) return Math.abs(px - tx) + Math.abs(py - ty);
      return Math.abs(px - los[index]!.x);
    }
  })();

  const x = Math.abs(tx - px);
  const y = Math.abs(ty - py);
  const min = Math.min(x, y);
  const max = Math.max(x, y);
  return AStarUnitCost * max +
         AStarDiagonalPenalty * min +
         AStarLOSDeltaPenalty * delta;
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
  // Try line-of-sight - if that path is clear, then we don't need to search.
  // As with the full search below, we don't check if source is blocked here.
  const los = LOS(source, target);
  if (los.every((x, i) => i === 0 || !blocked(x))) return los.slice(1);

  const map: Map<int, AStarNode> = new Map();
  const heap: AStarHeap = [];

  const score = AStarHeuristic(source, los);
  const node = new AStarNode(source.x, source.y, null, 0, score);
  AStarHeapPush(heap, node);
  map.set(node.key(), node);

  while (heap.length > 0) {
    const cur = AStarHeapExtractMin(heap);
    if (record) record.push(cur);

    if (cur.equal(target)) {
      let current = cur;
      const result: Point[] = [];
      while (current.parent) {
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

      const key = next.key();
      const existing = map.get(key);

      // index !== null is a check to see if we've already popped this node
      // from the heap. We need it because our heuristic is not admissible.
      //
      // Using such a heuristic substantially speeds up search in easy cases,
      // with the downside that we don't always find an optimal path.
      if (existing && existing.index !== null && existing.distance > distance) {
        existing.score += distance - existing.distance;
        existing.distance = distance;
        existing.parent = cur;
        AStarHeapify(heap, existing, existing.index);
      } else if (!existing) {
        const score = distance + AStarHeuristic(next, los);
        const created = new AStarNode(next.x, next.y, cur, distance, score);
        AStarHeapPush(heap, created);
        map.set(key, created);
      }
    }
  }

  return null;
};

//////////////////////////////////////////////////////////////////////////////

export {assert, int, Point, Direction, Matrix, LOS, FOV, AStar};
