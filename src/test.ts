import {assert, int} from './lib';
import {Point} from './geo';

const testPointDistanceNethack = () => {
  const cases: [Point, int][] = [
    [new Point(0, 0), 0],
    [new Point(1, 0), 1],
    [new Point(1, 1), 1],
    [new Point(2, 0), 2],
    [new Point(2, 1), 2],
    [new Point(2, 2), 3],
    [new Point(3, 0), 3],
    [new Point(3, 1), 3],
    [new Point(3, 2), 4],
    [new Point(3, 3), 4],
    [new Point(4, 0), 4],
    [new Point(4, 1), 4],
    [new Point(4, 2), 4],
    [new Point(4, 3), 5],
    [new Point(4, 4), 5],
    [new Point(5, 0), 5],
    [new Point(5, 1), 5],
    [new Point(5, 2), 5],
    [new Point(5, 3), 6],
    [new Point(5, 4), 6],
    [new Point(5, 5), 7],
  ];
  const origin = new Point(0, 0);
  for (const [point, expected] of cases) {
    const got = point.distanceNethack(origin);
    assert(got === expected, () => {
      return `distanceNethack for ${point}: expected: ${expected}, got: ${got}`;
    });

    // Unaffected by rotation, reflection, or translation.
    const n = 10;
    for (const sx of [-1, 1]) {
      for (const sy of [-1, 1]) {
        for (let dx = -n; dx <= n; dx++) {
          for (let dy = -n; dy <= n; dy++) {
            const mo = new Point(origin.x * sx + dx, origin.y * sy + dy);
            const mp = new Point(point.x * sx + dx, point.y * sy + dy);
            assert(mo.distanceNethack(mp) === expected);
            const no = new Point(mo.y, mo.x);
            const np = new Point(mp.y, mp.x);
            assert(no.distanceNethack(np) === expected);
          }
        }
      }
    }
  }
};

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
  testPointDistanceNethack();
  testPointKeyIsAnInjection();
};

main();

export {}
