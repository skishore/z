import {Vec} from './vec';

class Direction extends Vec {
  static none = new Direction(0, 0);
  static n    = new Direction(0, -1);
  static ne   = new Direction(1, -1);
  static e    = new Direction(1, 0);
  static se   = new Direction(1, 1);
  static s    = new Direction(0, 1);
  static sw   = new Direction(-1, 1);
  static w    = new Direction(-1, 0);
  static nw   = new Direction(-1, -1);

  /// The eight cardinal and diagonal directions.
  static all = [Direction.n, Direction.ne, Direction.e, Direction.se,
                Direction.s, Direction.sw, Direction.w, Direction.nw];

  /// The four cardinal directions: north, south, east, and west.
  static cardinal = [Direction.n, Direction.e, Direction.s, Direction.w];

  /// The four directions between the cardinal ones: northwest, northeast,
  /// southwest and southeast.
  static diagonal = [Direction.ne, Direction.se, Direction.sw, Direction.nw];

  constructor(x: number, y: number) { super(x, y); }

  get rotateLeft45() {
    switch (this) {
      case Direction.none: return Direction.none;
      case Direction.n: return Direction.nw;
      case Direction.ne: return Direction.n;
      case Direction.e: return Direction.ne;
      case Direction.se: return Direction.e;
      case Direction.s: return Direction.se;
      case Direction.sw: return Direction.s;
      case Direction.w: return Direction.sw;
      case Direction.nw: return Direction.w;
    }
    throw 'unreachable';
  }

  get rotateRight45() {
    switch (this) {
      case Direction.none: return Direction.none;
      case Direction.n: return Direction.ne;
      case Direction.ne: return Direction.e;
      case Direction.e: return Direction.se;
      case Direction.se: return Direction.s;
      case Direction.s: return Direction.sw;
      case Direction.sw: return Direction.w;
      case Direction.w: return Direction.nw;
      case Direction.nw: return Direction.n;
    }
    throw 'unreachable';
  }

  get rotateLeft90() {
    switch (this) {
      case Direction.none: return Direction.none;
      case Direction.n: return Direction.w;
      case Direction.ne: return Direction.nw;
      case Direction.e: return Direction.n;
      case Direction.se: return Direction.ne;
      case Direction.s: return Direction.e;
      case Direction.sw: return Direction.se;
      case Direction.w: return Direction.s;
      case Direction.nw: return Direction.sw;
    }
    throw 'unreachable';
  }

  get rotateRight90() {
    switch (this) {
      case Direction.none: return Direction.none;
      case Direction.n: return Direction.e;
      case Direction.ne: return Direction.se;
      case Direction.e: return Direction.s;
      case Direction.se: return Direction.sw;
      case Direction.s: return Direction.w;
      case Direction.sw: return Direction.nw;
      case Direction.w: return Direction.n;
      case Direction.nw: return Direction.ne;
    }
    throw 'unreachable';
  }

  get rotate180() {
    switch (this) {
      case Direction.none: return Direction.none;
      case Direction.n: return Direction.s;
      case Direction.ne: return Direction.sw;
      case Direction.e: return Direction.w;
      case Direction.se: return Direction.nw;
      case Direction.s: return Direction.n;
      case Direction.sw: return Direction.ne;
      case Direction.w: return Direction.e;
      case Direction.nw: return Direction.se;
    }
    throw 'unreachable';
  }

  toString() {
    switch (this) {
      case Direction.none: return 'Direction.none';
      case Direction.n: return 'Direction.n';
      case Direction.ne: return 'Direction.ne';
      case Direction.e: return 'Direction.e';
      case Direction.se: return 'Direction.se';
      case Direction.s: return 'Direction.s';
      case Direction.sw: return 'Direction.sw';
      case Direction.w: return 'Direction.w';
      case Direction.nw: return 'Direction.nw';
    }
    throw 'unreachable';
  }
}
