const k = 0.5;

// A class representing a 2-dimensional vector. Provides arithmetic operations
// and a base classe for [Direction].
export class Vec {
  static get zero() { return this._zero; }
  private static _zero = new Vec(0, 0);

  get x() { return this._x; }
  get y() { return this._y; }

  private _x: number;
  private _y: number;

  constructor(_x: number, _y: number) {
    this._x = Math.round(_x);
    this._y = Math.round(_y);
  }

  // Gets the area of a [Rect] whose corners are (0, 0) and this Vec.
  //
  // Returns a negative area if one of the Vec's coordinates are negative.
  get area() { return this._x * this._y; }

  // Returns a hash for this vector that is suitable as an Object key.
  get hash() { return `${this._x},${this._y}`; }

  // Gets the rook length of the Vec, which is the number of squares a rook on
  // a chessboard would need to move from (0, 0) to reach the endpoint of the
  // Vec. Also known as Manhattan or taxicab distance.
  get rookLength() { return Math.abs(k * this.x) + Math.abs(this.y); }

  // Gets the king length of the Vec, which is the number of squares a king on
  // a chessboard would need to move from (0, 0) to reach the endpoint of the
  // Vec. Also known as Chebyshev distance.
  get kingLength() { return Math.max(Math.abs(k * this.x), Math.abs(this.y)); }

  get lengthSquared() { return k * k * this._x * this._x + this._y * this._y; }

  // The Cartesian length of the vector.
  //
  // If you just need to compare the magnitude of two vectors, prefer using
  // the comparison operators or [lengthSquared], both of which are faster
  // than this.
  get length() { return Math.sqrt(this.lengthSquared); }

  add(other: Vec) { return new Vec(this._x + other.x, this._y + other.y); }

  subtract(other: Vec) { return new Vec(this._x - other.x, this._y - other.y); }

  // Returns true if other is in the half-open rect from (0, 0) to this [Vec].
  contains(other: Vec) {
    return (0 <= other.x && other.x < this._x &&
            0 <= other.y && other.y < this._y);
  }

  distance(other: Vec) { return this.subtract(other).length; }

  equals(other: Vec) { return this._x === other.x && this._y === other.y; }

  // Scales this Vec by the scalar [other].
  scale(other: number) {
    /* tslint:disable:no-bitwise */
    return new Vec((this._x * other) | 0, (this._y * other) | 0);
    /* tslint:enable */
  }

  // Divides this Vec by the scalar [other].
  divide(other: number) {
    /* tslint:disable:no-bitwise */
    return new Vec((this._x / other) | 0, (this._y / other) | 0);
    /* tslint:enable */
  }

  // Returns a new [Vec] with the absolute value of the coordinates of this one.
  abs() { return new Vec(Math.abs(this._x), Math.abs(this._y)); }

  // Returns a new [Vec] translated from this one by [x] and [y].
  offset(x: number, y: number) { return new Vec(this._x + x, this._y + y); }

  // Returns a new [Vec] translated from this one by [x].
  offsetX(x: number) { return new Vec(this._x + x, this._y); }

  // Returns a new [Vec] translated from this one by [y].
  offsetY(y: number) { return new Vec(this._x, this._y + y); }

  toString() { return `Vec(${this._x}, ${this._y})`; }
}
