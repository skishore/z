type int = number;
interface point {x: int, y: int};

const assert = (x: boolean, fn?: () => string): void => {
  if (!x) throw new Error(fn && fn());
};

const flatten = <T>(xss: T[][]): T[] => {
  const result: T[] = [];
  xss.forEach(xs => xs.forEach(x => result.push(x)));
  return result;
};

const range = (n: int): int[] => {
  const result = [];
  for (let i = 0; i < n; i++) {
    result.push(i);
  }
  return result;
};

//////////////////////////////////////////////////////////////////////////////
// Tran-Thong symmetric line-of-sight calculation.

const LOS = (a: point, b: point): point[] => {
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
// Pre-computed visibility trie, for field-of-vision computation.

interface Node {
  x: int,
  y: int,
  children: Node[],
};

class FOV {
  #root: Node;

  constructor(radius: int) {
    this.#root = {x: 0, y: 0, children: []};
    for (let i = 0; i <= radius; i++) {
      for (let j = 0; j < 8; j++) {
        const [xa, ya] = (j & 1) ? [radius, i] : [i, radius];
        const [xb, yb] = [xa * ((j & 2) ? 1 : -1), ya * ((j & 4) ? 1 : -1)];
        const line = LOS({x: 0, y: 0}, {x: xb, y: yb});
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

export {assert, flatten, int, point, range, FOV, LOS};
