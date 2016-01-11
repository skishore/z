// An interface provided for convenience so that clients can pass an [Object]
// [x] and [y] properties to any [Vec] method.
export interface IVec {
  x: number;
  y: number;
}

// A class representing a 2-dimensional vector. Provides arithmetic operations
// and a base classe for [Direction].
export class Vec implements IVec {
  static get zero() { return this._zero; }
  private static _zero = new Vec(0, 0);

  get x() { return this._x; }
  get y() { return this._y; }

  private _x: number;
  private _y: number;

  constructor(x: number, y: number) {
    /* tslint:disable:no-bitwise */
    this._x = x | 0;
    this._y = y | 0;
    /* tslint:enable */
  }

  static coerce(pos: IVec) {
    return pos instanceof Vec ? pos : new Vec(pos.x, pos.y);
  }

  // Gets the area of a [Rect] whose corners are (0, 0) and this Vec.
  //
  // Returns a negative area if one of the Vec's coordinates are negative.
  get area() { return this._x * this._y; }

  // Gets the rook length of the Vec, which is the number of squares a rook on
  // a chessboard would need to move from (0, 0) to reach the endpoint of the
  // Vec. Also known as Manhattan or taxicab distance.
  get rookLength() { return Math.abs(this.x) + Math.abs(this.y); }

  // Gets the king length of the Vec, which is the number of squares a king on
  // a chessboard would need to move from (0, 0) to reach the endpoint of the
  // Vec. Also known as Chebyshev distance.
  get kingLength() { return Math.max(Math.abs(this.x), Math.abs(this.y)); }

  get lengthSquared() { return this._x * this._x + this._y * this._y; }

  // The Cartesian length of the vector.
  //
  // If you just need to compare the magnitude of two vectors, prefer using
  // the comparison operators or [lengthSquared], both of which are faster
  // than this.
  get length() { return Math.sqrt(this.lengthSquared); }

  // Scales this Vec by [other].
  scale(other: number) { return new Vec(this._x * other, this._y * other); }

  // Divides this Vec by [other].
  divide(other: number) { return new Vec(this._x / other, this._y / other); }

  // Adds [other] to this Vec.
  //
  //  *  If [other] is a [Vec] or [Direction], adds each pair of coordinates.
  //  *  If [other] is an [int], adds that value to both coordinates.
  //
  // Any other type is an error.
  add(other: IVec) {
    return new Vec(this._x + other.x, this._y + other.y);
  }

  // Substracts [other] from this Vec.
  //
  //  *  If [other] is a [Vec] or [Direction], subtracts each pair of
  //     coordinates.
  //  *  If [other] is an [int], subtracts that value from both coordinates.
  //
  // Any other type is an error.
  subtract(other: IVec) {
    return new Vec(this._x - other.x, this._y - other.y);
  }

  equals(other: IVec) {
    return this._x === other.x && this._y === other.y;
  }

  // Returns `true` if the magnitude of this vector is greater than [other].
  gt(other: IVec|Number) {
    if (other instanceof Number) {
      return this.lengthSquared > other;
    } else if (other instanceof Vec) {
      return this.lengthSquared > other.lengthSquared;
    } else {
      return this.lengthSquared > Vec.coerce(other).lengthSquared;
    }
  }

  // Returns `true` if the magnitude of this vector is greater than or equal
  // to [other].
  ge(other: IVec|Number) {
    if (other instanceof Number) {
      return this.lengthSquared >= other;
    } else if (other instanceof Vec) {
      return this.lengthSquared >= other.lengthSquared;
    } else {
      return this.lengthSquared >= Vec.coerce(other).lengthSquared;
    }
  }

  // Returns `true` if the magnitude of this vector is less than [other].
  lt(other: IVec|Number) {
    if (other instanceof Number) {
      return this.lengthSquared < other;
    } else if (other instanceof Vec) {
      return this.lengthSquared < other.lengthSquared;
    } else {
      return this.lengthSquared < Vec.coerce(other).lengthSquared;
    }
  }

  // Returns `true` if the magnitude of this vector is less than or equal to
  // [other].
  le(other: IVec|Number) {
    if (other instanceof Number) {
      return this.lengthSquared <= other;
    } else if (other instanceof Vec) {
      return this.lengthSquared <= other.lengthSquared;
    } else {
      return this.lengthSquared <= Vec.coerce(other).lengthSquared;
    }
  }

  // Returns true if the pos is contained in a half-inclusive rectangle between
  // this [Vec] and (0, 0)
  contains(pos: IVec) {
    if (pos.x < Math.min(0, this._x)) return false;
    if (pos.x >= Math.max(0, this._x)) return false;
    if (pos.y < Math.min(0, this._y)) return false;
    if (pos.y >= Math.max(0, this._y)) return false;
    return true;
  }

  /// Returns a new [Vec] with the absolute value of the coordinates of this
  /// one.
  abs() { return new Vec(Math.abs(this._x), Math.abs(this._y)); }

  /// Returns a new [Vec] whose coordinates are this one's translated by [x] and
  /// [y].
  offset(x: number, y: number) { return new Vec(this._x + x, this._y + y); }

  /// Returns a new [Vec] whose coordinates are this one's but with the X
  /// coordinate translated by [x].
  offsetX(x: number) { return new Vec(this._x + x, this._y); }

  /// Returns a new [Vec] whose coordinates are this one's but with the Y
  /// coordinate translated by [y].
  offsetY(y: number) { return new Vec(this._x, this._y + y); }

  get hash() { return `${this._x},${this._y}`; }

  toString() { return `Vec(${this._x}, ${this._y})`; }
}
