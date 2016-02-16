import {Direction} from '../../piecemeal/direction';
import {Nil, nil} from '../../piecemeal/nil';
import {Vec} from '../../piecemeal/vec';

import {kAStarDoorCost, kAStarFloorCost,
        kAStarOccupiedCost, kAStarStraightCost, } from '../option';
import {IActor, Stage} from '../stage';

class PathResult {
  // The [Direction] to move on the first step of the path.
  get direction() { return this._direction; }

  // The total number of steps on the path.
  get length() { return this._length; }

  constructor(private _direction: Direction, private _length: number) {}
}

// A* pathfinding algorithm.
export class AStar {
  // Tries to find a path from [start] to [end], searching up to [maxLength]
  // steps from [start]. Returns the [Direction] of the first step from [start]
  // along that path (or [Direction.none] if it determines there is no path
  // possible.
  static findDirection(stage: Stage<IActor>, start: Vec, end: Vec,
                       maxLength: number, canOpenDoors: boolean) {
    return this.findPath(stage, start, end, maxLength, canOpenDoors).direction;
  }

  static findPath(stage: Stage<IActor>, start: Vec, end: Vec,
                  maxLength: number, canOpenDoors: boolean) {
    let path = AStar._findPath(stage, start, end, maxLength, canOpenDoors);
    if (path instanceof PathNode) {
      let length = 1;
      while (path.parent instanceof PathNode &&
             (<PathNode>path.parent).parent instanceof PathNode) {
        [path, length] = [<PathNode>path.parent, length + 1];
      }
      return new PathResult(path.direction, length);
    }
    return new PathResult(Direction.none, 0);
  }

  // Internal helper used to implement pathing. Returns a PathNode or void.
  static _findPath(stage: Stage<IActor>, istart: Vec, iend: Vec,
                   maxLength: number, canOpenDoors: boolean): PathNode|Nil {
    // TODO(skishore): Use a heap data structure here.
    const start: Vec = new Vec(istart.x, istart.y);
    const end: Vec = new Vec(iend.x, iend.y);
    const startPath = new PathNode(nil /* parent */, Direction.none,
                                   start, 0, this._heuristic(start, end));
    const open = [startPath];
    const closed = {};

    while (open.length > 0) {
      // Pull out the best potential candidate.
      const current = open.pop();
      if (current.pos.equals(end) ||
          current.cost > kAStarFloorCost * maxLength) {
        // Found the path or reached the end of the search radius.
        return current;
      }
      closed[current.pos.hash] = true;

      for (const dir of Direction.all) {
        const neighbor = current.pos.add(dir);

        // Skip impassable tiles and tiles that we already have a path to.
        if (!stage.getTile(neighbor).traversable) continue;
        if (closed[neighbor.hash]) continue;

        // Given how far the current tile is, how far is the neighbor?
        let stepCost = kAStarFloorCost;
        if (stage.getTile(neighbor).type.opens) {
          if (canOpenDoors) {
            // One to open the door and one to enter the tile.
            stepCost = kAStarFloorCost * 2;
          } else {
            // Even though the monster can't open doors, we don't consider it
            // totally impassable because there's a chance the door will be
            // opened by someone else.
            stepCost = kAStarDoorCost;
          }
        } else if (!(stage.actorAt(neighbor) instanceof Nil)) {
          stepCost = kAStarOccupiedCost;
        }

        const cost = current.cost + stepCost;

        // See if we just found a better path to a tile we're already
        // considering. If so, remove the old one and replace it (below) with
        // this new better path.
        let inOpen = false;
        for (let i = 0; i < open.length; i++) {
          const alreadyOpen = open[i];
          if (alreadyOpen.pos.equals(neighbor)) {
            if (alreadyOpen.cost > cost) {
              open.splice(i, 1);
              i--;
            } else {
              inOpen = true;
            }
            break;
          }
        }

        // If we have a new path, add it.
        if (!inOpen) {
          const guess = cost + this._heuristic(neighbor, end);
          const path = new PathNode(current, dir, neighbor, cost, guess);

          // Insert it in sorted order (such that the best node is at the *end*
          // of the list for easy removal).
          let inserted = false;
          for (let i = open.length - 1; i >= 0; i--) {
            if (open[i].guess > guess) {
              open.splice(i + 1, 0, path);
              inserted = true;
              break;
            }
          }

          // If we didn't find a node to put it after, put it at the front.
          if (!inserted) open.unshift(path);
        }
      }
    }
    // The path is blocked. (If there was a path but it was too long, then we
    // would exit out in the first check in the A* loop.)
    return nil;
  }

  // The estimated cost from [pos] to [end].
  static _heuristic(pos: Vec, end: Vec) {
    // A simple heuristic would just be the kingLength. The problem is that
    // diagonal moves are as "fast" as straight ones, which means many
    // zig-zagging paths are as good as one that looks "straight" to the player.
    // But they look wrong. To avoid this, we will estimate straight steps to
    // be a little cheaper than diagonal ones. This avoids paths like:
    //
    // ...*...
    // s.*.*.g
    // .*...*.
    const offset = end.subtract(pos).abs();
    const numDiagonal = Math.min(offset.x, offset.y);
    const numStraight = Math.max(offset.x, offset.y) - numDiagonal;
    return numDiagonal * kAStarFloorCost + numStraight * kAStarStraightCost;
  }
}

class PathNode {
  get parent() { return this._parent; };
  get direction() { return this._direction; };
  get pos() { return this._pos; };

  /// The cost to get to this node from the starting point. This is roughly the
  /// distance, but may be a little different if we start weighting tiles in
  /// interesting ways (i.e. make it more expensive for light-abhorring
  /// monsters to walk through lit tiles).
  get cost() { return this._cost; };

  /// The guess as to the total cost from the start node to the end node going
  /// along this path. In other words, this is [cost] plus the heuristic.
  get guess() { return this._guess; };

  constructor(private _parent: PathNode|Nil, private _direction: Direction,
              private _pos: Vec, private _cost: number,
              private _guess: number) {}
}
