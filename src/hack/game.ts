import {Direction} from '../piecemeal/direction';
import {Nil, nil} from '../piecemeal/nil';
import {Vec} from '../piecemeal/vec';

import {Stage, TileType} from '../engine/stage';

import {Action, MovementAction} from './action';
import {Actor, Player, Pokemon, Trainer} from './actor';
import {BlockOnInputBehavior, RunBehavior} from './behavior';
import {Graphics} from './graphics';
import {Effect} from './effect';
import {assert} from './util';

interface IGameResult {
  advanced: boolean;
  effects: Array<Effect>;
}

export interface ILogEntry {
  message: string;
  count: number;
}

export class Game {
  stage: Stage<Actor>;
  player: Player;
  private _log = new Array<ILogEntry>();
  private _graphics: Graphics;
  private _action: Action|Nil = nil;

  constructor(size: Vec) {
    const ground = new TileType('ground', true /* passable */,
                                true /* transparent */, '.');
    this.stage = new Stage<Actor>(size, ground);
    this.player = this._spawnActors();

    const handler = this.handleInput.bind(this);
    this._graphics = new Graphics(size, handler);
    this._graphics.render(this.stage, this._log, []);
  }

  handleInput(ch: string) {
    const moves: {[key: string]: Direction} = {
      h: Direction.w,
      j: Direction.s,
      k: Direction.n,
      l: Direction.e,
      y: Direction.nw,
      u: Direction.ne,
      b: Direction.sw,
      n: Direction.se,
      '.': Direction.none,
    };
    const behavior = this.player.behavior;
    if (this._action instanceof Nil &&
        behavior instanceof BlockOnInputBehavior &&
        behavior.action instanceof Nil) {
      const lower = ch === '>' ? '.' : ch.toLowerCase();
      if (moves[ch]) {
        behavior.action = new MovementAction(moves[ch]);
      } else if (moves[lower]) {
        this.player.behavior = new RunBehavior(moves[lower]);
      }
    }
    return false;
  }

  log(message: string) {
    message = message[0].toUpperCase() + message.slice(1);
    if (this._log.length > 0 &&
        this._log[this._log.length - 1].message === message) {
      this._log[this._log.length - 1].count += 1;
    } else {
      this._log.push({message: message, count: 1});
      if (this._log.length > 6) {
        this._log.shift();
      }
    }
    this.player.disturb();
  }

  tick() {
    const update: IGameResult = {advanced: false, effects: []};
    while (true) {
      // Consume existing actions, if any are available.
      const action = this._action;
      if (action instanceof Action) {
        const actor = action.actor;
        const result = action.execute(update.effects);
        if (result.alternate instanceof Action) {
          const alternate = <Action>result.alternate;
          alternate.bind(this, actor);
          this._action = alternate;
          continue;
        }
        update.advanced = true;
        if (result.done) {
          this._action = nil;
          if (result.success || !(actor instanceof Player)) {
            actor.timer.wait(result.turns);
            this.stage.advanceActor();
          }
          if (actor instanceof Player) {
            return update;
          }
        }
        if (update.effects.length > 0) {
          return update;
        }
      }
      // Construct new actions, if any are required.
      if (this._action instanceof Nil) {
        const actor = this.stage.currentActor;
        if (actor.timer.ready || actor.timer.wait(-actor.speed)) {
          const action = actor.nextAction();
          if (action instanceof Action) {
            action.bind(this, actor);
            this._action = action;
          } else {
            assert(actor instanceof Player);
            return update;
          }
        } else {
          this.stage.advanceActor();
        }
      }
    }
  }

  update() {
    const update = this.tick();
    if (update.advanced || this._graphics.animated) {
      this._graphics.render(this.stage, this._log, update.effects);
    }
    setTimeout(this.update.bind(this), 16);
  }

  _spawnActors() {
    const midline = Math.floor(this.stage.size.y / 2);
    const left = new Vec(4, midline);
    const right = new Vec(this.stage.size.x - left.x - 1, midline);

    const player = new Player(left, this.stage);
    const rival = new Trainer(right, this.stage);

    new Pokemon(left.offset(2, -2), this.stage, 'Bulbasaur', player);
    new Pokemon(left.offset(2, 0), this.stage, 'Charmader', player);
    new Pokemon(left.offset(2, 2), this.stage, 'Squirtle', player);
    new Pokemon(right.offset(-2, -2), this.stage, 'Poliwag', rival);
    new Pokemon(right.offset(-2, 0), this.stage, 'Electabuzz', rival);
    new Pokemon(right.offset(-2, 2), this.stage, 'Diglett', rival);

    return player;
  }
}
