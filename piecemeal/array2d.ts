import {Rect} from './rect';
import {IVec, Vec} from './vec';

type _NullaryGenerator<T> = () => T;

// TODO(skishore): Support other generator types. It's tricky because
// Typescript does not have good support for instanceof with functions.
//type _VecGenerator<T> = (pos: IVec) => T;
//type _CoordGenerator<T> = (x: number, y: number) => T;

// A two-dimensional fixed-size array of elements of type [T].
//
// This class doesn't follow matrix notation which tends to put the column
// index before the row. Instead, it mirrors graphics and games where x --
// the horizontal component -- comes before y.
//
// Internally, the elements are stored in a single contiguous list in row-major
// order.
class Array2D<T> {
  private _size: Vec;
  private _elements: Array<T>;

  // Creates a new array of the given [size] with its elements set to [value].
  constructor(size: IVec, value?: T) {
    this._size = new Vec(size.x, size.y);
    this._elements = new Array<T>(size.x * size.y).fill(value);
  }

  // Creates a new array of the given [size] with its elements initialized by
  // calling the [generator] function.
  static generated<U>(size: Vec, generator: _NullaryGenerator<U>) {
    const result = new Array2D<U>(size);
    result.generate(generator);
    return result;
  }

  // Gets the element at [pos].
  get(pos: IVec) {
    if (!this.bounds.contains(pos)) throw `${pos} is not in ${this.bounds}.`;
    return this._elements[pos.y * this._size.x + pos.x];
  }

  // Sets the element at [pos].
  set(pos: IVec, value: T) {
    if (!this.bounds.contains(pos)) throw `${pos} is not in ${this.bounds}.`;
    this._elements[pos.y * this._size.x + pos.x] = value;
  }

  // A [Rect] whose bounds cover the full range of valid element indexes.
  get bounds() { return new Rect(0, 0, this._size.x, this._size.y); }

  // The size of the array.
  get size() { return this._size; }

  // Evaluates [generator] on each position in the array and sets the element
  // at that position to the result.
  generate(generator: _NullaryGenerator<T>) {
    for (let pos of this.bounds) { this.set(pos, generator()); }
  }
}
