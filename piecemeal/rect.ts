import {IVec, Vec} from './vec';

// A two-dimensional immutable rectangle with integer coordinates.
//
// Many operations treat a [Rect] as a collection of discrete points. In those
// cases, the boundaries of the rect are two half-open intervals when
// determining which points are inside the rect. For example, consider the
// rect whose coordinates are (-1, 1)-(3, 4):
//
//      -2 -1  0  1  2  3  4
//       |  |  |  |  |  |  |
//     0-
//     1-   +-----------+
//     2-   |           |
//     3-   |           |
//     4-   +-----------+
//     5-
//
// It contains all points within that region except for the ones that lie
// directly on the right and bottom edges. (It's always right and bottom,
// even if the rectangle has negative coordinates.) In the above examples,
// that's these points:
//
//      -2 -1  0  1  2  3  4
//       |  |  |  |  |  |  |
//     0-
//     1-   *--*--*--*--+
//     2-   *  *  *  *  |
//     3-   *  *  *  *  |
//     4-   +-----------+
//     5-
//
// This seems a bit odd, but does what you want in almost all respects. For
// example, the width of this rect, determined by subtracting the left
// coordinate (-1) from the right (3) is 4 and indeed it contains four columns
// of points.
export class Rect {
  // Gets the empty rectangle.
  static get empty() { return this._empty; }
  static _empty = new Rect(0, 0, 0, 0);

  private _pos: Vec;
  private _size: Vec;

  get width() { return this._size.x; }
  get height() { return this._size.y; }

  get left() { return this._pos.x; }
  get top() { return this._pos.y; }
  get right() { return this._pos.x + this._size.x; }
  get bottom() { return this._pos.y + this._size.y; }

  get topLeft() { return new Vec(this.left, this.top); }
  get topRight() { return new Vec(this.right, this.top); }
  get bottomLeft() { return new Vec(this.left, this.bottom); }
  get bottomRight() { return new Vec(this.right, this.bottom); }

  get center() { return new Vec((this.left + this.right) / 2,
                                (this.top + this.bottom) / 2); }

  get area() { return this._size.area; }

  constructor(x: number, y: number, width: number, height: number) {
    this._pos = new Vec(Math.min(x, x + width), Math.min(y, y + width));
    this._size = new Vec(Math.abs(width), Math.abs(height));
  }

  static posAndSize(pos: IVec, size: IVec) {
    return new Rect(pos.x, pos.y, size.x, size.y);
  }

  // Creates a new rectangle a single column in width, as tall as [size],
  // with its top left corner at [pos].
  static column(x: number, y: number, size: number) {
    return new Rect(x, y, 1, size);
  }

  // Creates a new rectangle a single row in height, as wide as [size],
  // with its top left corner at [pos].
  static row(x: number, y: number, size: number) {
    return new Rect(x, y, size, 1);
  }

  // Creates a new rectangle that is the intersection of [a] and [b].
  //
  //     .----------.
  //     | a        |
  //     | .--------+----.
  //     | | result |  b |
  //     | |        |    |
  //     '-+--------'    |
  //       |             |
  //       '-------------'
  static intersect(a: Rect, b: Rect) {
    const left = Math.max(a.left, b.left);
    const right = Math.min(a.right, b.right);
    const top = Math.max(a.top, b.top);
    const bottom = Math.min(a.bottom, b.bottom);

    const width = Math.max(0, right - left);
    const height = Math.max(0, bottom - top);
    return new Rect(left, top, width, height);
  }

  // Returns a new [Vec] that is as near to [point] as possible while still
  // being in bounds.
  clamp(point: IVec) {
    const x = Math.max(this.left, Math.min(point.x, this.right));
    const y = Math.max(this.top, Math.min(point.x, this.bottom));
    return new Vec(x, y);
  }

  contains(point: IVec) {
    if (point.x < this.left) return false;
    if (point.x >= this.right) return false;
    if (point.y < this.top) return false;
    if (point.y >= this.bottom) return false;
    return true;
  }

  containsRect(rect: Rect) {
    if (rect.left < this.left) return false;
    if (rect.right > this.right) return false;
    if (rect.top < this.top) return false;
    if (rect.bottom > this.bottom) return false;
    return true;
  }

  // Returns the distance between this Rect and [other]. This is minimum
  // length that a corridor would have to be to go from one Rect to the other.
  // If the two Rects are adjacent, returns zero. If they overlap, returns -1.
  distance(other: Rect) {
    let vertical = -1;
    if (this.top >= other.bottom) {
      vertical = this.top - other.bottom;
    } else if (this.bottom <= other.top) {
      vertical = other.top - this.bottom;
    }

    let horizontal = -1;
    if (this.left >= other.right) {
      horizontal = this.left - other.right;
    } else if (this.right <= other.left) {
      horizontal = other.left - this.right;
    }

    if ((vertical === -1) && (horizontal === -1)) return -1;
    if (vertical === -1) return horizontal;
    if (horizontal === -1) return vertical;
    return horizontal + vertical;
  }

  inflate(distance: number) {
    return new Rect(this.left - distance, this.top - distance,
                    this.width + (distance * 2), this.height + (distance * 2));
  }

  /// Iterates over the points along the edge of the Rect.
  *trace() {
    if (this.width > 1 && this.height > 1) {
      for (let x = this.left; x < this.right; x++) {
        yield new Vec(x, this.top);
        yield new Vec(x, this.bottom - 1);
      }
      for (let y = this.top + 1; y < this.bottom - 1; y++) {
        yield new Vec(this.left, y);
        yield new Vec(this.right, y);
      }
    } else {
      for (const x of this) yield x;
    }
  }

  *[Symbol.iterator]() {
    let x = this.left;
    let y = this.top;
    if (this.width === 0) return;
    while (y < this.bottom) {
      yield new Vec(x, y);
      x += 1;
      if (x >= this.right) {
        x = this.left;
        y += 1;
      }
    }
  }

  toString() { return `Rect(${this.left}, ${this.top}, ` +
                           `${this.width}, ${this.height})`; }
}
