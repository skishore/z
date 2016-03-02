import {Direction} from '../piecemeal/direction';
import {Nil, nil} from '../piecemeal/nil';
import {Vec} from '../piecemeal/vec';

import {LineOfSight} from '../engine/line_of_sight';
import {Stage} from '../engine/stage';

import {Actor, Player, Pokemon} from './actor';
import {Effect, FireEffect} from './effect';
import {Game} from './game';
import {_} from './util';

export class Action {
  actor: Actor;
  stage: Stage<Actor>;
  private _game: Game;

  bind(game: Game, actor: Actor) {
    this.actor = actor;
    this.stage = actor.stage;
    this._game = game;
  }

  execute(effects: Array<Effect>): ActionResult {
    throw new Error('Action.execute was not implemented!');
  }

  log(message: string) {
    this._game.log(message);
  }

  logForPlayer(message: string) {
    if (this.actor instanceof Player) {
      this._game.log(message);
    }
  }
}

export class ActionResult {
  done = true;
  success = false;
  alternate: Action|Nil = nil;
  turns = 1;

  static alternate(alternate: Action) {
    const result = new ActionResult;
    result.alternate = alternate;
    return result;
  }

  static failed() {
    return new ActionResult;
  }

  static incomplete() {
    const result = new ActionResult;
    result.done = false;
    return result;
  }

  static success() {
    const result = new ActionResult;
    result.success = true;
    return result;
  }
}

export class FireAction extends Action {
  private _line: LineOfSight;
  private _iterator: IterableIterator<Vec>;

  constructor(private _target: Vec) { super(); }

  execute(effects: Array<Effect>) {
    if (!this._line) {
      if (this.actor.position.equals(this._target)) {
        return ActionResult.failed();
      }
      this._line = new LineOfSight(this.actor.position, this._target);
      this._iterator = this._line[Symbol.iterator]();
    }
    let result = ActionResult.incomplete();
    _.range(2).forEach(() => {
        if (!result.done) result = this._executeOnce(effects); });
    return result;
  }

  _executeOnce(effects: Array<Effect>) {
    const position = this._iterator.next().value;
    if (!this.stage.size.contains(position)) {
      return ActionResult.success();
    }
    effects.push(new FireEffect(position));
    const other = this.stage.actorAt(position);
    if (other instanceof Actor) {
      this.log(`${this.actor.description} attacks ${other.description}.`);
      return ActionResult.success();
    }
    return ActionResult.incomplete();
  }
}

export class IdleAction extends Action {
  execute() {
    return ActionResult.success();
  }
}

export class MovementAction extends Action {
  constructor(private _step: Direction) { super(); }

  execute() {
    if (this._step.equals(Direction.none)) {
      return ActionResult.alternate(new IdleAction);
    }
    const position = this.actor.position.add(this._step);
    if (!this.stage.size.contains(position)) {
      this.logForPlayer("You can't run from a trainer battle!");
      return ActionResult.failed();
    }
    const other = this.stage.actorAt(position);
    if (other instanceof Actor) {
      if (other instanceof Pokemon && other.trainer === this.actor) {
        this.stage.swapActors(this.actor.position, position);
        this.logForPlayer(`You switch places with ${other.description}.`);
      } else {
        this.logForPlayer(`You bump into ${other.description}.`);
      }
      return ActionResult.success();
    }
    this.stage.moveActor(this.actor.position, position);
    return ActionResult.success();
  }
}
