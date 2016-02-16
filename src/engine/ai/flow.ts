import {Array2D} from '../../piecemeal/array2d';
import {Direction} from '../../piecemeal/direction';
import {Nil, nil} from '../../piecemeal/nil';
import {Rect} from '../../piecemeal/rect';
import {rng} from '../../piecemeal/rng';
import {Vec} from '../../piecemeal/vec';

 import {IActor, Stage} from '../stage';

const _unknown = -2;
const _unreachable = -1;

// A lazy, generic pathfinder.
//
// It can be used to find the distance from a starting point to a goal, or
// find the directions to reach the nearest goals meeting some predicate.
//
// Internally, it lazily runs a breadth-first search. It only processes outward
// as far as needed to answer the query. In practice, this means it often does
// less than 10% of the iterations of a full eager search.
export class Flow {
  private _stage: Stage<IActor>;
  private _start: Vec;
  private _canOpenDoors: boolean;
  private _ignoreActors: boolean;

  private _distances: Array2D<number>;

  // The position of the array's top-level corner relative to the stage.
  private _offset: Vec;

  // The cells whose neighbors still remain to be processed.
  // TODO(skishore): Replace this method with a queue if it is not performant.
  private _open = new Array<Vec>();

  // The list of reachable cells that have been found so far, in order of
  // increasing distance from the [_start].
  //
  // Coordinates are relative to [_distances], not the [Stage].
  private _found = new Array<Vec>();

  // Returns the bounds of the [Flow] in stage coordinates.
  get bounds() { return Rect.posAndSize(this._offset, this._distances.size); }

  // Returns the starting position in stage coordinates.
  get start() { return this._start; }

  constructor(
      stage: Stage<IActor>, start: Vec,
      {maxDistance, canOpenDoors, ignoreActors}:
      {maxDistance?: number, canOpenDoors?: boolean, ignoreActors?: boolean}) {
    this._stage = stage;
    this._start = new Vec(start.x, start.y);
    this._canOpenDoors = canOpenDoors || false;
    this._ignoreActors = ignoreActors || false;

    this._offset = Vec.zero;
    let width = this._stage.size.x;
    let height = this._stage.size.y;

    if (maxDistance !== undefined) {
      const left = Math.max(0, this._start.x - maxDistance);
      const top = Math.max(0, this._start.y - maxDistance);
      const right = Math.min(
          this._stage.size.x, this._start.x + maxDistance + 1);
      const bottom = Math.min(
          this._stage.size.y, this._start.y + maxDistance + 1);
      this._offset = new Vec(left, top);
      width = right - left;
      height = bottom - top;
    }

    this._distances = new Array2D<number>(new Vec(width, height), _unknown);
    this._open.push(this._start.subtract(this._offset));
    this._distances.set(this._open[0], 0);
  }

  // Returns the nearest position to [_start] that meets [predicate].
  //
  // If there are multiple equidistant matching positions, this method chooses
  // one randomly. If there are none, it returns the starting position.
  nearestWhere(predicate: (pos: Vec) => boolean) {
    const results = this._findAllNearestWhere(predicate);
    if (results.length === 0) return this._start;
    return rng.item(results).add(this._offset);
  }

  // Returns the distance from [_start] to [pos], or [nil] if there is no path
  // between the two locations.
  getDistance(pos: Vec): number|Nil {
    const vec = pos.subtract(this._offset);
    if (!this._distances.bounds.contains(vec)) return nil;
    // Lazily search until we reach the tile in question or are blocked.
    while (this._open.length > 0 &&
           this._distances.get(pos) === _unknown) {
      this._processNext();
    }
    const distance = this._distances.get(pos);
    if (distance === _unknown || distance === _unreachable) return nil;
    return distance;
  }

  // Returns a random direction from [start] that gets closer to [pos], or
  // [Direction.none] if no such direction exists.
  directionTo(pos: Vec) {
    const vec = pos.subtract(this._offset);
    const directions = this._directionsTo([vec]);
    if (directions.length === 0) return Direction.none;
    return rng.item(directions);
  }

  // Chooses a random direction from [start] that gets closer to one of the
  // nearest positions matching [predicate].
  //
  // Returns [Direction.none] if no matching positions were found.
  directionToNearestWhere(predicate: (pos: Vec) => boolean) {
    const directions = this.directionsToNearestWhere(predicate);
    if (directions.length === 0) return Direction.none;
    return rng.item(directions);
  }

  // Find all directions from [start] that get closer to one of the nearest
  // positions matching [predicate].
  //
  // Returns an empty list if no matching positions were found.
  directionsToNearestWhere(predicate: (pos: Vec) => boolean) {
    const goals = this._findAllNearestWhere(predicate);
    if (goals.length === 0) return [];
    return this._directionsTo(goals);
  }

  // Find all directions from [_start] that get closer to a position in [goals].
  //
  // Returns an empty list if none of the goals can be reached.
  _directionsTo(goals: Array<Vec>) {
    const walked = {};
    const directions = new Array<Direction>();
    const target = this._start.subtract(this._offset);

    // Starting at [pos], recursively walk along all paths towards [_start].
    const walkBack = (pos: Vec) => {
      if (walked[pos.hash]) return;
      walked[pos.hash] = true;

      for (const dir of Direction.all) {
        const here = pos.add(dir);
        if (!this._distances.bounds.contains(here)) continue;

        if (here.equals(target)) {
          // If this step reached the target, mark the direction of the step.
          // We will never add a duplicate direction because of the walked set.
          directions.push(dir.rotate180);
        } else if (this._distances.get(here) >= 0 &&
                   this._distances.get(here) < this._distances.get(pos)) {
          walkBack(here);
        }
      }
    };

    // Trace all paths from the goals back to the target.
    goals.forEach(walkBack);
    return directions;
  }

  // Get the positions closest to [start] that meet [predicate].
  //
  // Only returns more than one position if there are multiple equidistance
  // positions meeting the criteria. Returns an empty list if no valid
  // positions are found. Returned positions are local to [_distances], not
  // the [Stage].
  _findAllNearestWhere(predicate: (pos: Vec) => boolean) {
    const goals = new Array<Vec>();
    let nearestDistance = Infinity;
    for (let i = 0; ; i++) {
      // Lazily find the next open tile.
      while (this._open.length > 0 && i >= this._found.length) {
        this._processNext();
      }
      // If we flowed everywhere and didn't find anything, give up.
      if (i >= this._found.length) return goals;

      const pos = this._found[i];
      if (!predicate(pos.add(this._offset))) continue;
      // Since pos was from _found, it should be reachable.
      const distance = this._distances.get(pos);
      if (distance < 0) {
        throw new Error(`${pos} was in _found but was unreachable.`);
      }

      if (goals.length === 0 || distance === nearestDistance) {
        // Consider all goals at the nearest distance.
        nearestDistance = distance;
        goals.push(pos);
      } else {
        // We hit a tile that's farther than some goal tile, so we can break.
        break;
      }
    }
    return goals;
  }

  // Runs one iteration of the lazy search.
  _processNext() {
    if (this._open.length === 0) throw '_processNext called with nothing open.';
    const start = this._open.shift();
    const distance = this._distances.get(start);

    // Update all neighbor's distances.
    for (const dir of Direction.all) {
      const here = start.add(dir);
      if (!this._distances.bounds.contains(here)) continue;
      if (this._distances.get(here) !== _unknown) continue;

      // Determine whether it is possible to enter the given cell.
      const pos = here.add(this._offset);
      const tile = this._stage.getTile(pos);
      const canEnter =
          (tile.passable || (tile.traversable && this._canOpenDoors)) &&
          (this._ignoreActors || (this._stage.actorAt(pos) instanceof Nil));

      if (!canEnter) {
        this._distances.set(here, _unreachable);
        continue;
      }

      this._distances.set(here, distance + 1);
      this._open.push(here);
      this._found.push(here);
    }
  }

  // Prints the [_distances] array for debugging.
  _dump() {
    const tokens = new Array<string>();
    for (let y = 0; y < this._distances.size.y; y++) {
      for (let x = 0; x < this._distances.size.x; x++) {
        const distance = this._distances.get(new Vec(x, y));
        if (distance === _unknown) {
          tokens.push('?');
        } else if (distance === _unreachable) {
          tokens.push('#');
        } else {
          tokens.push(`${distance % 10}`);
        }
      }
      tokens.push('\n');
    }
    return tokens.join('');
  }
}
