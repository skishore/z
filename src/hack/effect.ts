import {rng} from '../piecemeal/rng';
import {Vec} from '../piecemeal/vec';

export type Buffer = Array<Array<string>>;

export class Effect {
  private _completed = false;

  update(buffer: Buffer) {
    return true;
  }

  wrap(buffer: Buffer) {
    const completed = this._completed;
    this._completed = this._completed || !this.update(buffer);
    return !completed;
  }
}

export class FireEffect extends Effect {
  private _point: Vec;
  private _index = 0;
  private _cycle = '**++';
  private _offset = rng.range(this._cycle.length);

  constructor(point: Vec) {
    super();
    this._point = point;
  }

  update(buffer: Buffer) {
    this._index += 1;
    const index = (this._index + this._offset) % this._cycle.length;
    buffer[this._point.y][this._point.x] = this._cycle[index];
    return this._index < 4;
  }
}
