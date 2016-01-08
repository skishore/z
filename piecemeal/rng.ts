declare function require(name: string);
const random_seed = require('random-seed');

import {Rect} from './rect';
import {IVec, Vec} from './vec';

// The Random Number God: deliverer of good and ill fortune alike.
export class Rng {
  private _random: any;

  constructor(seed: string|number) {
    this._random = random_seed.create(seed);
  }

  // Gets a random int within a given range. If [max] is given, then it is
  // in the range `[minOrMax, max)`. Otherwise, it is `[0, minOrMax)`. In
  // other words, `range(3)` returns a `0`, `1`, or `2`, and `range(2, 5)`
  // returns `2`, `3`, or `4`.
  range(minOrMax: number, max?: number) {
    if (max == null) {
      max = minOrMax;
      minOrMax = 0;
    }
    return <number>this._random.intBetween(minOrMax, max - 1);
  }

  // Gets a random int within a given range. If [max] is given, then it is
  // in the range `[minOrMax, max]`. Otherwise, it is `[0, minOrMax]`. In
  // other words, `inclusive(2)` returns a `0`, `1`, or `2`, and
  // `inclusive(2, 4)` returns `2`, `3`, or `4`.
  inclusive(minOrMax: number, max?: number) {
    if (max == null) {
      max = minOrMax;
      minOrMax = 0;
    }
    return <number>this._random.intBetween(minOrMax, max);
  }

  // Returns `true` if a random int chosen between 1 and chance was 1.
  oneIn(chance: number) { return this.range(chance) === 0; }

  // Gets a random item from the given list.
  item<T>(items: Array<T>) { return items[this.range(items.length)]; }

  // Removes a random item from the given list.
  take<T>(items: Array<T>) {
    return items.splice(this.range(items.length), 1)[0];
  }

  /// Gets a random [Vec] within the given [Rect] (half-inclusive).
  vecInRect(rect: Rect) {
    return new Vec(this.range(rect.left, rect.right),
                   this.range(rect.top, rect.bottom));
  }

  // Gets a random number centered around [center] with [range] (inclusive)
  // using a triangular distribution. For example `triangleInt(8, 4)` will
  // return values between 4 and 12 (inclusive) with greater distribution
  // towards 8.
  //
  // This means output values will range from `(center - range)` to
  // `(center + range)` inclusive, with most values near the center, but not
  // with a normal distribution. Think of it as a poor man's bell curve.
  //
  // The algorithm works essentially by choosing a random point inside the
  // triangle, and then calculating the x coordinate of that point. It works
  // like this:
  //
  // Consider Center 4, Range 3:
  //
  //             *
  //           * | *
  //         * | | | *
  //       * | | | | | *
  //     --+-----+-----+--
  //     0 1 2 3 4 5 6 7 8
  //      -r     c     r
  //
  // Now flip the left half of the triangle (from 1 to 3) vertically and move
  // it over to the right so that we have a square.
  //
  //         .-------.
  //         |       V
  //         |
  //         |   R L L L
  //         | . R R L L
  //         . . R R R L
  //       . . . R R R R
  //     --+-----+-----+--
  //     0 1 2 3 4 5 6 7 8
  //
  // Choose a point in that square. Figure out which half of the triangle the
  // point is in, and then remap the point back out to the original triangle.
  // The result is the *x* coordinate of the point in the original triangle.
  triangle(center: number, range: number) {
    if (range < 0) throw 'range must be non-negative.';
    const x = this.inclusive(range);
    const y = this.inclusive(range);
    return center + x - (x > y ? range + 1 : 0);
  }

  taper(start: number, chanceOfIncrement: number) {
    while (this.oneIn(chanceOfIncrement)) start++;
    return start;
  }
}

export const rng = new Rng(new Date().getTime());
