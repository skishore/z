import {IActor, Stage, TileType} from './engine/stage';
import {Direction} from './piecemeal/direction';
import {Nil, nil} from './piecemeal/nil';
import {rng} from './piecemeal/rng';
import {Vec} from './piecemeal/vec';

/* tslint:disable */
declare function require(name: string): any;
const _ = require('lodash');
const gaussian = require('gaussian');

declare const process: any;
/* tslint:enable */

const assert = (condition: boolean, message?: any) => {
  if (!condition) {
    console.error(message);
    throw new Error(message);
  }
};

// The various Actions follow.

class Action {
  actor: Actor;
  stage: Stage<Actor>;
  private _game: Game;

  bind(game: Game, actor: Actor) {
    this.actor = actor;
    this.stage = actor.stage;
    this._game = game;
  }

  execute(effects: Array<Effect>): ActionResult {
    assert(false, 'Action.execute was not implemented!');
    return ActionResult.failed();
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

class ActionResult {
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

class FireAction extends Action {
  source: Vec;
  last: Vec;
  target: Vec;
  index = 0;

  constructor(target: Vec) {
    super();
    this.target = target;
  }

  execute(effects: Array<Effect>) {
    let result = ActionResult.failed();
    if (!this.source) {
      this.source = this.last = this.actor.position;
      if (this.source.equals(this.target)) {
        return result;
      }
    }
    for (let i of _.range(2)) {
      result = this._executeOnce(effects);
      if (result.success) {
        break;
      }
    }
    return result;
  }

  _executeOnce(effects: Array<Effect>) {
    let position = this.last;
    const diff = this.target.subtract(this.source);
    while (position.equals(this.last)) {
      this.index += 1;
      position = this.source.add(diff.scale(this.index / diff.length));
    }
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

class IdleAction extends Action {
  execute() {
    return ActionResult.success();
  }
}

class MovementAction extends Action {
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

// The various Actors follow.

class Actor implements IActor {
  behavior: Behavior;
  position: Vec;
  stage: Stage<Actor>;
  timer: Timer;

  constructor(position: Vec, stage: Stage<Actor>) {
    this.position = position;
    this.stage = stage;
    this.timer = new Timer;
    stage.addActor(this);
  }

  get description() { return 'an unknown actor'; }

  get glyph() { return '?'; }

  get speed() { return 0.1; }

  nextAction(): Action|Nil {
    while (!this.behavior || this.behavior.satisfied) {
      this.behavior = this._defaultBehavior();
    }
    return this.behavior.ready ? this.behavior.nextAction() : nil;
  }

  _defaultBehavior(): Behavior {
    return new MoveRandomlyBehavior;
  }
}

class Pokemon extends Actor {
  breed: string;
  trainer: Trainer|Nil;

  constructor(position: Vec, stage: Stage<Actor>,
              breed: string, trainer: Trainer|Nil) {
    super(position, stage);
    this.breed = breed;
    this._allyWith(trainer);
  }

  get description() {
    let allegiance = '';
    if (this.trainer instanceof Trainer && !(this.trainer instanceof Player)) {
      const trainer = <Trainer>this.trainer;
      allegiance = `${trainer.description}'s `;
    }
    return `${allegiance}${this.breed}`;
  }

  get glyph() { return this.breed[0].toLowerCase(); }

  get speed() { return 0.2; }

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
          (x: Actor) => x instanceof Pokemon && x.trainer !== this.trainer);
      if (targets.length > 0) {
        distance = 12;
        target = targets[0];
        if (this.position.distance(target.position) < distance) {
          return new ExecuteOnceBehavior(new FireAction(target.position));
        }
      }
    }

    const distribution = gaussian(0, distance);
    for (let i of _.range(8)) {
      const offset = new Vec(Math.round(distribution.ppf(Math.random())),
                             Math.round(distribution.ppf(Math.random())));
      const position = target.position.add(offset);
      if (this.stage.isSquareFree(position)) {
        return new MoveToPositionBehavior(this, position, 2);
      }
    }
    return new MoveToPositionBehavior(this, this.position, 2);
  }
}

class Trainer extends Actor {
  pokemon = new Array<Pokemon>();

  get description() { return 'your rival'; }

  get glyph() { return '@'; }
}

class Player extends Trainer {
  get description() { return 'you'; }

  disturb() { this.behavior = this._defaultBehavior(); }

  _defaultBehavior() { return new BlockOnInputBehavior(); }
}

// The various Behaviors follow.

class Behavior {
  get ready() { return true; }

  get satisfied() { return false; }

  nextAction() {
    assert(false, 'nextAction was not implemented!');
    return new Action;
  }
}

class BlockOnInputBehavior extends Behavior {
  action: Action|Nil = nil;

  get ready() { return this.action instanceof Action; }

  nextAction() {
    const action = <Action>this.action;
    this.action = nil;
    return action;
  }
}

class ExecuteOnceBehavior extends BlockOnInputBehavior {
  constructor(action: Action) {
    super();
    this.action = action;
  }

  get satisfied() { return !this.ready; }
}

class MoveRandomlyBehavior extends Behavior {
  nextAction() {
    return new MovementAction(rng.item(Direction.all));
  }
}

class MoveToPositionBehavior extends Behavior {
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
    Direction.all.map((direction: Direction) => {
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

class RunBehavior extends Behavior {
  private _index = 0;

  constructor(private _step: Direction) { super(); }

  get satisfied() { return this._index >= 4; }

  nextAction() {
    this._index += 1;
    return new MovementAction(this._step);
  }
}

// Core game logic follows.

interface IGameResult {
  advanced: boolean;
  effects: Array<Effect>;
}

interface ILogEntry {
  message: string;
  count: number;
}

class Game {
  stage: Stage<Actor>;
  player: Player;
  private _log = new Array<ILogEntry>();
  private _graphics: Graphics;
  private _action: Action|Nil = nil;

  constructor() {
    const ground = new TileType('ground', true /* passable */,
                                true /* transparent */, '.');
    this.stage = new Stage<Actor>(new Vec(40, 15), ground);
    this.player = this._spawnActors();

    const handler = this.handleInput.bind(this);
    this._graphics = new Graphics(this.stage, handler);
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
    if (!(behavior instanceof BlockOnInputBehavior)) {
      return false;
    } else if (behavior.ready || this._action instanceof Action) {
      return false;
    }
    const lower = ch === '>' ? '.' : ch.toLowerCase();
    if (moves[ch]) {
      (<BlockOnInputBehavior>behavior).action = new MovementAction(moves[ch]);
    } else if (moves[lower]) {
      this.player.behavior = new RunBehavior(moves[lower]);
    }
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
      if (this._action instanceof Action) {
        const action = <Action>this._action;
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
          this._action = actor.nextAction();
          if (this._action instanceof Nil) {
            assert(actor instanceof Player);
            return update;
          }
          (<Action>this._action).bind(this, actor);
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

class Timer {
  private _timeout = 256;
  private _timeLeft = rng.range(this._timeout);

  get ready() { return this._timeLeft < 0; }

  wait(turns: number) {
    this._timeLeft += Math.ceil(turns * this._timeout);
    return this.ready;
  }
}

// Graphics code follows.

type Buffer = Array<Array<string>>;

class Effect {
  private _completed = false;

  update(buffer: Buffer) {
    return true;
  }

  _update(buffer: Buffer) {
    const completed = this._completed;
    this._completed = this._completed || !this.update(buffer);
    return !completed;
  }
}

class FireEffect extends Effect {
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

class Graphics {
  private _screen: any;
  private _stage: any;
  private _status: any;
  private _log: any;
  private _buffer: Buffer;
  private _effects = new Array<Effect>();

  constructor(stage: Stage<Actor>, handler: any) {
    /* tslint:disable */
    const blessed = require('blessed');
    /* tslint:enable */
    this._screen = blessed.screen({smartCSR: true});
    this._stage = blessed.box({
      width: stage.size.x,
      height: stage.size.y,
      tags: true,
    });
    this._status = blessed.box({
      left: stage.size.x + 1,
      tags: true,
    });
    this._log = blessed.box({
      top: stage.size.y + 1,
      tags: true,
    });

    this._screen.title = 'pokehack';
    this._screen.append(this._stage);
    this._screen.append(this._status);
    this._screen.append(this._log);

    const alphabet = 'abcdefghijklmnopqrstuvwxyz'.split('');
    const keys = alphabet.concat(alphabet.map((x: string) => `S-${x}`))
                         .concat(['.', '>']);
    this._screen.key(keys, handler);
    this._screen.key(['C-c', 'escape'], () => {
      return process.exit(0);
    });

    this._buffer = _.range(stage.size.y).map((y: number) =>
        _.range(stage.size.x).map(() => '.'));
  }

  get animated() { return this._effects.length > 0; }

  render(stage: Stage<Actor>, log: Array<ILogEntry>, effects: Array<Effect>) {
    // Render the stage.
    for (let y = 0; y < stage.size.y; y++) {
      for (let x = 0; x < stage.size.x; x++) {
        this._buffer[y][x] = '.';
      }
    }
    for (let actor of stage.actors) {
      this._buffer[actor.position.y][actor.position.x] = actor.glyph;
    }
    this._effects = this._effects.concat(effects);
    this._effects = this._effects.filter(
        (x: Effect) => x._update(this._buffer));
    this._stage.setContent(this._buffer.map(
        (x: Array<string>) => x.join('')).join('\n'));
    // Render the status.
    const status = new Array<string>();
    for (let actor of stage.actors) {
      if (actor instanceof Pokemon && actor.trainer instanceof Player) {
        ['\n', actor.description, '\n'].map((x: string) => status.push(x));
      }
    }
    this._status.setContent(status.join(''));
    // Render the log.
    this._log.setContent(log.map((entry: ILogEntry) => {
      const suffix = entry.count > 1 ? ` (x${entry.count})` : '';
      return `${entry.message}${suffix}`;
    }).join('\n'));
    this._screen.render();
  }
}

const game = new Game;
game.update();
