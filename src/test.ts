import {assert, int} from './lib';
import {Point} from './geo';

const testPointKeyIsAnInjection = () => {
  const n = 100;
  const map: Map<int, Point> = new Map();

  for (let x = -n; x <= n; x++) {
    for (let y = -n; y <= n; y++) {
      const point = new Point(x, y);
      const match = map.get(point.key());
      assert(!match, () => `Point.key collision: ${match}, ${point}`);
      map.set(point.key(), point);
    }
  }

  const side = 2 * n + 1;
  assert(map.size === side * side);
};

const main = () => {
  testPointKeyIsAnInjection();
};

main();

export {}
