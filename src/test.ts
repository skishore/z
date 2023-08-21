import {assert, int, nonnull, range} from './lib';
import {Point, Matrix, AStar, BFS, Status} from './geo';

//////////////////////////////////////////////////////////////////////////////

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
            const mo = new Point(int(origin.x * sx + dx),
                                 int(origin.y * sy + dy));
            const mp = new Point(int(point.x * sx + dx),
                                 int(point.y * sy + dy));
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

  for (let x = int(-n); x <= n; x++) {
    for (let y = int(-n); y <= n; y++) {
      const point = new Point(x, y);
      const match = map.get(point.key());
      assert(!match, () => `Point.key collision: ${match}, ${point}`);
      map.set(point.key(), point);
    }
  }

  const side = 2 * n + 1;
  assert(map.size === side * side);
};

//////////////////////////////////////////////////////////////////////////////

type SearchTestCase = {
  source: Point,
  targets: Point[],
  check: (p: Point) => Status,
  raw: string[][],
};

const parseSearchTestCase = (input: string): SearchTestCase => {
  const lines = input.trim().split('\n');
  assert(lines.length > 0);
  const size = new Point(int(nonnull(lines[0]).length), int(lines.length));
  const map = new Matrix(size, Status.FREE);
  const raw = range(size.y).map(_ => range(size.x).map(_ => '.'));
  const sources: Point[] = [];
  const targets: Point[] = [];

  lines.forEach((line, yy) => {
    const y = int(yy);
    assert(line.length === size.x);
    for (let x = int(0); x < size.x; x++) {
      const ch = nonnull(line[x]);
      const point = new Point(x, y);
      if (ch === '.' || ch === '?' || ch === '*') continue;
      switch (ch) {
        case '@': sources.push(point); break;
        case 'T': targets.push(point); break;
        case 'X': map.set(point, Status.OCCUPIED); break;
        case '#': map.set(point, Status.BLOCKED); break;
        default: assert(false, () => `Unknown character: ${ch}`);
      }
      raw[y]![x] = ch;
    }
  });

  const check = (p: Point) => {
    const result = map.getOrNull(p);
    return result === null ? Status.BLOCKED : result;
  };

  assert(sources.length === 1);
  return {source: nonnull(sources[0]), targets, check, raw};
};

const runAStarTestCase = (input: string) => {
  const expected = input.trim();
  const test = parseSearchTestCase(input);
  assert(test.targets.length === 1);
  const target = nonnull(test.targets[0]);

  const set = (point: Point, ch: string) => {
    const {x, y} = point;
    const old = nonnull(nonnull(test.raw[y])[x]);
    if (old === '@' || old === 'T' || old === 'X' || old === '#') return;
    test.raw[y]![x] = ch;
  };

  const seen: Point[] = [];
  const path = nonnull(AStar(test.source, target, test.check, seen));
  seen.forEach(x => set(x, '?'));
  path.forEach(x => set(x, '*'));

  const actual = test.raw.map(x => x.join('')).join('\n');
  assert(actual === expected,
         () => `Actual:\n\n${actual}\n\nExpected:\n\n${expected}`);
};

const runBFSTestCase = (input: string) => {
  const expected = input.trim();
  const lines = expected.split('\n');
  const limit = int(parseInt(nonnull(lines[0]).substr('Limit: '.length), 10));
  input = lines.slice(1).join('\n');

  const test = parseSearchTestCase(input);
  const targets: Set<int> = new Set();
  test.targets.forEach(x => targets.add(x.key()));
  const target = (x: Point) => targets.has(x.key());

  const set = (point: Point, ch: string) => {
    const {x, y} = point;
    const old = nonnull(nonnull(test.raw[y])[x]);
    if (old === '@' || old === 'T' || old === 'X' || old === '#') return;
    test.raw[y]![x] = ch;
  };

  const seen: Point[] = [];
  const result = BFS(test.source, target, limit, test.check, seen);
  const dirs = result?.directions ?? [];
  seen.forEach(x => set(x, '?'));
  dirs.forEach(x => set(x.add(test.source), '*'));

  const prefix = `Limit: ${limit}\n`;
  const actual = prefix + test.raw.map(x => x.join('')).join('\n');
  assert(actual === expected,
         () => `Actual:\n\n${actual}\n\nExpected:\n\n${expected}`);
};

const testAStar = () => {
  const cases = `
........
.....*T.
...**...
.@*.....
........

........
..??X...
.@*XX*T.
..?**...
........

...???....
..?####...
.@??X...T.
..*####*..
...****...

...****...
.?*####*..
??*####.*.
?@??X...T.
???####...
.??####...
...???....

..????....
.??####...
???####...
???####...
?@**X***T.
???####...
???####...
.??####...
..????....

..............
......**......
....?*##**T...
...@*?##......
....??##......
..............

.......................
..............##.......
..............##..**T..
.....##.??********.....
...??##***##.....##....
..@****???###....##....
...........##..........
.......................

...............#..............#.....
.T**?.................##.......##...
...#*??..#............##.......##...
....?***?..#......#.................
......?#*?......................#...
.......??**#..##....................
...........***##...............##...
..............*##?...###....#.......
..............?***??.######.#.......
...##...........##**********..#....#
...##.#.......######..######*.......
...............?????######???*......
.................?????????????*.....
...#.............###???????????*....
.................##..?????????#?*...
.........................???????#*#.
..........................????????@.
....................................

.##.##..#...###.###.#...#####.....#.##.
...###..#*?..####...#..##..#.##.#.....#
######***#***?##..#.....###.#...##.....
#..T**#??##?#***?????..####.#...###...#
#.#...?#???.#?##*?##??####.####.####..#
.#....#.##..###?#*??#???##?##..##...#..
#...##.#.#.##.##??***?##??#?#####..##..
#....#..#.#.#..###?##***?##????#.##...#
#...#.#..#....#.##.#?#?#****##??.......
##..#.##...#...#...#.#.#####*##??..##.#
####...##...#.####.###.######*****...#.
..#..##.......##..##...##...#????#*####
.#..####..##..#.#####.########?####*.##
........#..#..#..????.#######????#?@.##
.#.###.......#.#...##?#??###??###?#?#.#
##....#.#....##.#..##??.#??#?#..##?.#.#
#..###..####.#.....##..#.#??#.##..####.
  `;
  cases.split('\n\n').forEach(runAStarTestCase);
};

const testBFS = () => {
  const cases = `
Limit: 2
.........
...T.....
..?????..
..?????..
..??@??..
..?????..
..?????..
.T.......
.........

Limit: 3
.........
.??T????.
.???????.
.??***??.
.???@???.
.??*????.
.???????.
.T??????.
.........

Limit: 3
.........
.?T?????.
.???????.
.??**???.
.???@???.
.???????.
.???????.
.???????.
.........

Limit: 3
.........
...?????.
..##????.
.?##????.
.???@???.
.??###??.
.???.???.
.........
.........

Limit: 3
.........
.???T....
.???###?.
.??**???.
.???@???.
.???????.
.???????.
.???????.
.........
  `;
  cases.split('\n\n').forEach(runBFSTestCase);
};

//////////////////////////////////////////////////////////////////////////////

const main = () => {
  testPointDistanceNethack();
  testPointKeyIsAnInjection();
  testAStar();
  testBFS();
};

main();

export {}
