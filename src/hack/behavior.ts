import {Direction} from '../piecemeal/direction';
import {Nil, nil} from '../piecemeal/nil';
import {rng} from '../piecemeal/rng';
import {Vec} from '../piecemeal/vec';

import {Action, MovementAction} from './action';
import {Actor} from './actor';
import {assert} from './util';

export class Behavior {
  get satisfied() { return false; }

  nextAction(): Action|Nil {
    assert(false, 'nextAction was not implemented!');
    return new Action;
  }
}

export class BlockOnInputBehavior extends Behavior {
  action: Action|Nil = nil;

  nextAction() {
    const action = this.action;
    this.action = nil;
    return action;
  }
}

export class ExecuteOnceBehavior extends BlockOnInputBehavior {
  constructor(action: Action) {
    super();
    this.action = action;
  }

  get satisfied() { return this.action instanceof Nil; }
}

export class MoveRandomlyBehavior extends Behavior {
  nextAction() {
    return new MovementAction(rng.item(Direction.all));
  }
}

export class MoveToPositionBehavior extends Behavior {
  private _actor: Actor;
  private _target: Vec;
  private _index = 0;
  private _limit = 24;

  constructor(actor: Actor, target: Vec, limit: number) {
    super();
    this._actor = actor;
    this._target = target;
    this._limit = limit || this._limit;
  }

  get satisfied() {
    return this._index >= this._limit ||
           this._actor.position.equals(this._target);
  }

  nextAction() {
    this._index += 1;
    let distance = this._actor.position.distance(this._target);
    let step = Direction.none;
    Direction.all.map((direction) => {
      const position = this._actor.position.add(direction);
      if (this._actor.stage.actorAt(position) instanceof Nil) {
        const proposal = this._target.distance(position);
        if (proposal < distance) {
          distance = proposal;
          step = direction;
        }
      }
    });
    return new MovementAction(step);
  }
}

export class RunBehavior extends Behavior {
  private _index = 0;

  constructor(private _step: Direction) { super(); }

  get satisfied() { return this._index >= 4; }

  nextAction() {
    this._index += 1;
    return new MovementAction(this._step);
  }
}
