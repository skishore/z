import {Vec} from '../piecemeal/vec';

// Line-of-sight object for tracing a straight line from a [start] to [end]
// and determining which intermediate tiles are touched.
export class LineOfSight {
  private _start: Vec;
  private _end: Vec;
  private _current: Vec;
  private _error: number;
  private _primary: number;
  private _secondary: number;
  private _primaryIncrement: Vec;
  private _secondaryIncrement: Vec;

  constructor(start: Vec, end: Vec) {
    this._start = start;
    this._end = end;
    this._current = start;
    this._error = 0;

    // Figure which quadrant the line is in and increment appropriately.
    let delta = end.subtract(start);
    this._primaryIncrement = new Vec(Math.sign(delta.x), 0);
    this._secondaryIncrement = new Vec(0, Math.sign(delta.y));

    // Assume that we have to move horizontally on each step.
    delta = delta.abs();
    this._primary = delta.x;
    this._secondary = delta.y;

    // Swap the order if the y magnitude is greater.
    if (delta.y > delta.x) {
      const temp = this._primary;
      this._primary = this._secondary;
      this._secondary = temp;

      const tempIncrement = this._primaryIncrement;
      this._primaryIncrement = this._secondaryIncrement;
      this._secondaryIncrement = tempIncrement;
    }
  }

  // Always returns `true` to allow a line to overshoot the end point. Make
  // sure you terminate iteration yourself.
  *[Symbol.iterator]() {
    while (true) {
      this._current = this._current.add(this._primaryIncrement);
      this._error += this._secondary;
      if (this._error * 2 >= this._primary) {
        this._current = this._current.add(this._secondaryIncrement);
        this._error -= this._primary;
      }
      yield this._current;
    }
  }
}
