import {Direction} from '../piecemeal/direction';
import {Nil, nil} from '../piecemeal/nil';
import {rng} from '../piecemeal/rng';
import {Vec} from '../piecemeal/vec';

import {AStar} from '../engine/ai/a_star';
import {Flow} from '../engine/ai/flow';
import {IActor, Stage} from '../engine/stage';

import {Action, FireAction, MovementAction} from './action';
import {Behavior, BlockOnInputBehavior, ExecuteOnceBehavior,
        MoveRandomlyBehavior, MoveToPositionBehavior, } from './behavior';
import {_} from './util';

/* tslint:disable */
declare function require(name: string): any;
const gaussian = require('gaussian');
/* tslint:enable */

// The various Actors follow.

export class Actor implements IActor {
  behavior: Behavior;
  position: Vec;
  stage: Stage<Actor>;
  private _timer = new Timer;

  constructor(position: Vec, stage: Stage<Actor>) {
    this.position = position;
    this.stage = stage;
    stage.addActor(this);
  }

  get description() { return 'an unknown actor'; }

  get glyph() { return '?'; }

  get speed() { return 0.1; }

  nextAction(): Action|Nil {
    while (!this.behavior || this.behavior.satisfied) {
      this.behavior = this._defaultBehavior();
    }
    return this.behavior.nextAction();
  }

  ready() {
    if (!this._timer.ready) this._timer.wait(-this.speed);
    return this._timer.ready;
  }

  wait(turns: number) {
    this._timer.wait(turns);
  }

  _defaultBehavior(): Behavior {
    return new MoveRandomlyBehavior;
  }
}

export class Pokemon extends Actor {
  breed: string;
  trainer: Trainer|Nil;
  private _cooldown = new Timer;

  constructor(position: Vec, stage: Stage<Actor>,
              breed: string, trainer: Trainer|Nil) {
    super(position, stage);
    this.breed = breed;
    this._allyWith(trainer);
  }

  get description() {
    let allegiance = '';
    const trainer = this.trainer;
    if (trainer instanceof Trainer && !(trainer instanceof Player)) {
      allegiance = `${trainer.description}'s `;
    }
    return `${allegiance}${this.breed}`;
  }

  get glyph() { return this.breed[0].toLowerCase(); }

  get speed() { return 0.2; }

  ready() {
    // TODO(skishore): Move the cooldown into a Move object.
    if (!this._cooldown.ready) this._cooldown.wait(-this.speed / 10);
    return super.ready();
  }

  _allyWith(trainer: Trainer|Nil) {
    this.trainer = trainer;
    if (trainer instanceof Trainer) {
      trainer.pokemon.push(this);
    }
  }

  _defaultBehavior(): Behavior {
    if (this.trainer instanceof Nil) {
      return new MoveRandomlyBehavior;
    }
    let distance = 8;
    let target = <Actor>this.trainer;

    if (this.breed === 'Diglett' || this.breed === 'Squirtle') {
      const targets = this.stage.actors.filter(
          (x) => x instanceof Pokemon && x.trainer !== this.trainer);
      if (targets.length > 0) {
        distance = 12;
        target = targets[0];
      }
    }

    if (target !== this.trainer) {
      if (this._cooldown.ready && this._isValidRangedPosition(
              this.position, target.position, distance)) {
        this._cooldown.wait(1 /* turns */);
        return new ExecuteOnceBehavior(new FireAction(target.position));
      }
      const direction = this._findRangedPath(target.position, distance);
      if (direction instanceof Direction) {
        return new ExecuteOnceBehavior(new MovementAction(direction));
      }
    }

    const distribution = gaussian(0, distance);
    /* tslint:disable:no-unused-variable */
    for (let i of _.range(8)) {
    /* tslint:enable */
      const offset = new Vec(Math.round(distribution.ppf(Math.random())),
                             Math.round(distribution.ppf(Math.random())));
      const position = target.position.add(offset);
      if (this.stage.isSquareFree(position)) {
        return new MoveToPositionBehavior(this, position, 2);
      }
    }
    return new MoveToPositionBehavior(this, this.position, 2);
  }

  // Tries to find a path a desirable position for using a ranged [Move].
  //
  // Returns the [Direction] to take along the path. Returns [Direction.NONE]
  // if the monster's current position is a good ranged spot. Returns `null`
  // if no good ranged position could be found.
  private _findRangedPath(target: Vec, range: number): Direction|Nil {
    let best: Direction;
    let bestDistance = 0;

    if (this._isValidRangedPosition(this.position, target, range)) {
      best = Direction.none;
      bestDistance = this.position.distance(target);
    }
    for (const dir of Direction.all) {
      const pos = this.position.add(dir);
      if (!this.stage.isSquareFree(pos)) continue;
      if (!this._isValidRangedPosition(pos, target, range)) continue;
      const distance = pos.distance(target);
      if (distance > bestDistance) {
        best = dir;
        bestDistance = distance;
      }
    }
    if (best) return best;

    // We'll need to actually pathfind to reach a good vantage point.
    const flow = new Flow(this.stage, this.position, {maxDistance: range});
    let result = flow.directionToNearestWhere(
        (pos) => this._isValidRangedPosition(pos, target, range));
    if (result.equals(Direction.none)) {
      result = AStar.findDirection(this.stage, this.position, target,
                                   false /* canOpenDoors */);
    }
    return result.equals(Direction.none) ? nil : result;
  }

  private _isValidRangedPosition(
      pos: Vec, target: Vec, range: number): boolean {
    if (pos.distance(target) >= range) return false;
    const actor = this.stage.actorAt(pos);
    if (actor instanceof Actor && actor !== this) return false;

    // TODO(skishore): De-deduplicate this line-of-sight computation with the
    // one in FireAction.
    const diff = target.subtract(pos);
    for (let i = 0; i <= diff.length; i++) {
      const step = pos.add(diff.scale(i / diff.length));
      if (step.equals(pos)) continue;
      if (step.equals(target)) return true;
      if (this.stage.actorAt(step) instanceof Actor) return false;
    }
    return true;
  };
}

export class Trainer extends Actor {
  pokemon = new Array<Pokemon>();

  get description() { return 'your rival'; }

  get glyph() { return '@'; }
}

export class Player extends Trainer {
  get description() { return 'you'; }

  disturb() { this.behavior = this._defaultBehavior(); }

  _defaultBehavior() { return new BlockOnInputBehavior(); }
}

// The Timer helper class, an implementation detail of Actor, follows.

class Timer {
  private _timeout = 256;
  private _timeLeft = rng.range(this._timeout);

  get ready() { return this._timeLeft < 0; }

  wait(turns: number) {
    this._timeLeft += Math.ceil(turns * this._timeout);
  }
}
