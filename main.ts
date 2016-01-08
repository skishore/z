"use strict";

const _ = require('lodash');
const gaussian = require('gaussian');

const assert = (condition, message) => {
  if (!condition) {
    console.error(message);
    throw new Error;
  }
}

// Utility methods follow.

const util = {
  add(point1, point2) {
    return [point1[0] + point2[0], point1[1] + point2[1]];
  },
  distance(point1, point2) {
    return util.length(util.subtract(point1, point2));
  },
  equal(point1, point2) {
    return point1[0] === point2[0] && point1[1] === point2[1];
  },
  inBounds(point, size) {
    return 0 <= point[0] && point[0] < size[0] &&
           0 <= point[1] && point[1] < size[1];
  },
  key(point) {
    return point.join(',');
  },
  length(point) {
    return Math.sqrt(point[0]*point[0] + point[1]*point[1]);
  },
  subtract(point1, point2) {
    return [point1[0] - point2[0], point1[1] - point2[1]];
  },
}

// The various Actions follow.

class Action {
  constructor() {}
  bind(actor) {
    this.actor = actor;
    this.stage = actor.stage;
    this._game = actor.stage.game;
  }
  log(message) {
    this._game.log(message);
  }
  logForPlayer(message) {
    if (this.actor instanceof Player) {
      this._game.log(message);
    }
  }
}

class ActionResult {
  constructor() {
    this.done = true;
    this.success = false;
    this.alternate = null;
    this.turns = 1;
  }
  static alternate(alternate) {
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
  constructor(target) {
    super();
    this.target = target;
    this.index = 0;
  }
  bind(actor) {
    super.bind(actor);
    this.last = this.last || this.actor.position;
    this.diff = util.subtract(this.target, this.actor.position);
    this.length = util.length(this.diff);
  }
  execute(effects) {
    let result = null;
    for (let i of _.range(2)) {
      result = this._executeOnce(effects);
      if (result.success) {
        break;
      }
    }
    return result;
  }
  _executeOnce(effects) {
    let point = this.last;
    while (util.equal(point, this.last)) {
      this.index += 1;
      const factor = this.index/this.length;
      const step = this.diff.map((x) => Math.round(factor*x));
      point = util.add(this.actor.position, step);
    }
    if (!util.inBounds(point, this.stage.size)) {
      return ActionResult.success();
    }
    effects.push(new FireEffect(point));
    const other = this.stage.getActorAt(point);
    if (other) {
      this.log(`${this.actor.description()} attacks ${other.description()}.`);
      return ActionResult.success();
    }
    return ActionResult.incomplete();
  }
}

class IdleAction extends Action {
  constructor() {
    super();
  }
  execute() {
    return ActionResult.success();
  }
}

class MovementAction extends Action {
  constructor(step) {
    super();
    this.step = step;
  }
  execute() {
    if (this.step[0] === 0 && this.step[1] === 0) {
      return ActionResult.alternate(new IdleAction);
    }
    const position = util.add(this.step, this.actor.position);
    if (!util.inBounds(position, this.stage.size)) {
      this.logForPlayer("You can't run from a trainer battle!");
      return ActionResult.failed();
    }
    const other = this.stage.getActorAt(position);
    if (other) {
      if (other instanceof Pokemon && other.trainer === this.actor) {
        this.stage.swap(this.actor, position);
        this.logForPlayer(`You switch places with ${other.description()}.`);
      } else {
        this.logForPlayer(`You bump into ${other.description()}.`);
      }
      return ActionResult.success();
    }
    this.stage.move(this.actor, position);
    return ActionResult.success();
  }
}

// The various Actors follow.

class Actor {
  constructor() {
    // this.position and this.stage are set on spawning.
    this.speed = 0.1;
    this.timer = new Timer;
  }
  description() {
    return 'an unknown actor';
  }
  nextAction() {
    while (!this.behavior || this.behavior.satisfied()) {
      this.behavior = this._defaultBehavior();
    }
    return this.behavior.nextAction();
  }
  _defaultBehavior() {
    return new MoveRandomlyBehavior;
  }
}

class Pokemon extends Actor {
  constructor(trainer, breed) {
    super();
    this.breed = breed;
    this.speed = 0.2;
    this.glyph = breed[0].toLowerCase();
    this._allyWith(trainer);
  }
  description() {
    let allegiance = '';
    if (this.trainer && !(this.trainer instanceof Player)) {
      allegiance = `${this.trainer.description()}'s `;
    }
    return `${allegiance}${this.breed}`;
  }
  _allyWith(trainer) {
    if (!trainer) {
      return;
    }
    this.trainer = trainer;
    this.trainer.pokemon.push(this);
  }
  _defaultBehavior() {
    if (!this.trainer) {
      return new MoveRandomlyBehavior;
    }

    let distance = 8;
    let target = this.trainer;

    if (this.breed === 'Diglett' || this.breed === 'Squirtle') {
      const targets = this.stage.actors.filter(
          (x) => x instanceof Pokemon && x.trainer !== this.trainer);
      if (targets.length > 0) {
        distance = 12;
        target = targets[0];
        if (util.distance(this.position, target.position) < distance) {
          return new ExecuteOnceBehavior(new FireAction(target.position));
        }
      }
    }

    const distribution = gaussian(0, distance);
    for (let i of _.range(8)) {
      const offset = _.range(2).map(
          () => Math.round(distribution.ppf(Math.random())));
      const position = util.add(target.position, offset);
      if (this.stage.isSquareFree(position)) {
        return new MoveToPositionBehavior(this, position, 2);
      }
    }
    return new MoveToPositionBehavior(this, this.position, 2);
  }
}

class Trainer extends Actor {
  constructor() {
    super();
    this.glyph = '@';
    this.pokemon = [];
  }
  description() {
    return 'your rival';
  }
}

class Player extends Trainer {
  constructor() {
    super();
  }
  description() {
    return 'you';
  }
  disturb() {
    this.behavior = this._defaultBehavior();
  }
  _defaultBehavior() {
    return new BlockOnInputBehavior();
  }
}

// The various Behaviors follow.

class Behavior {
  constructor() {}
  nextAction() {
    assert(false, 'nextAction was not implemented!');
  }
  satisfied() {
    return false;
  }
}

class BlockOnInputBehavior extends Behavior {
  constructor() {
    super();
    this._action = null;
  }
  nextAction() {
    const action = this._action;
    delete this._action;
    return action;
  }
  ready() {
    return !!this._action;
  }
  setNextAction(action) {
    this._action = this._action || action;
  }
}

class ExecuteOnceBehavior extends BlockOnInputBehavior {
  constructor(action) {
    super();
    this._action = action;
  }
  satisfied() {
    return !this._action;
  }
}

class MoveRandomlyBehavior extends Behavior {
  constructor() {
    super();
  }
  nextAction() {
    return new MovementAction([_.random(-1, 1), _.random(-1, 1)]);
  }
}

class MoveToPositionBehavior extends Behavior {
  constructor(actor, position, limit) {
    super();
    this._actor = actor;
    this._position = position;
    this._index = 0;
    this._limit = limit || 24;
  }
  nextAction() {
    this._index += 1;
    let distance = util.distance(this._position, this._actor.position);
    let move = [0, 0];
    _.range(-1, 2).map((x) => _.range(-1, 2).map((y) => {
      const position = util.add(this._actor.position, [x, y]);
      if (this._actor.stage.getActorAt(position) &&
          !util.equal(position, this._position)) {
        return;
      }
      const proposal = util.distance(this._position, position);
      if (proposal < distance) {
        distance = proposal;
        move = [x, y];
      }
    }));
    return new MovementAction(move);
  }
  satisfied() {
    return this._index >= this._limit ||
           util.equal(this._actor.position, this._position);
  }
}

class RunBehavior extends Behavior {
  constructor(step) {
    super();
    this._step = step;
    this._index = 0;
  }
  nextAction() {
    this._index += 1;
    return new MovementAction(this._step);
  }
  satisfied() {
    return this._index >= 4;
  }
}

// Core game logic follows.

class Game {
  constructor() {
    this.stage = new Stage(this);
    this._log = [];
    this._action = null;

    const handler = this.handleInput.bind(this);
    this.graphics = new Graphics(this.stage, handler);
    this.graphics.render(this.stage, this._log, []);
  }
  handleInput(ch) {
    const moves = {
      h: [-1, 0],
      j: [0, 1],
      k: [0, -1],
      l: [1, 0],
      y: [-1, -1],
      u: [1, -1],
      b: [-1, 1],
      n: [1, 1],
      '.': [0, 0],
    }
    const behavior = this.stage.player.behavior;
    if (!(behavior instanceof BlockOnInputBehavior)) {
      return false;
    } else if (behavior.ready() || this._action) {
      return false;
    }
    const lower = ch === '>' ? '.' : ch.toLowerCase();
    if (moves[ch]) {
      behavior.setNextAction(new MovementAction(moves[ch]));
    } else if (moves[lower]) {
      this.stage.player.behavior = new RunBehavior(moves[lower]);
    }
  }
  log(message) {
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
    this.stage.player.disturb();
  }
  tick() {
    const update = {advanced: false, effects: []};
    while (true) {
      // Consume existing actions, if any are available.
      if (this._action) {
        const actor = this._action.actor;
        const result = this._action.execute(update.effects);
        assert(result instanceof ActionResult, result);
        if (result.alternate) {
          result.alternate.bind(actor);
          this._action = result.alternate;
          continue;
        }
        update.advanced = true;
        if (result.done) {
          delete this._action;
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
      if (!this._action) {
        const actor = this.stage.getCurrentActor();
        if (actor.timer.ready() || actor.timer.wait(-actor.speed)) {
          this._action = actor.nextAction();
          if (!this._action) {
            assert(actor instanceof Player);
            return update;
          }
          this._action.bind(actor);
        } else {
          this.stage.advanceActor();
        }
      }
    }
  }
  update() {
    const update = this.tick();
    if (update.advanced || this.graphics.hasEffects()) {
      this.graphics.render(this.stage, this._log, update.effects);
    }
    setTimeout(this.update.bind(this), 16);
  }
}

class Stage {
  constructor(game) {
    this.game = game;
    this.size = [40, 15];
    this.actors = [];
    this._current_actor = 0;
    this._position_actor_map = {};

    const player = new Player;
    const target = new Trainer;

    const left = 4;
    const right = this.size[0] - left - 1;
    const midpoint = Math.floor(this.size[1]/2);

    this.spawn(player, [left, midpoint]);
    this.spawn(target, [right, midpoint, 7]);
    this.spawn(new Pokemon(player, 'Bulbasaur'), [left + 2, midpoint - 2]);
    this.spawn(new Pokemon(player, 'Charmander'), [left + 2, midpoint]);
    this.spawn(new Pokemon(player, 'Squirtle'), [left + 2, midpoint + 2]);
    this.spawn(new Pokemon(target, 'Poliwag'), [right - 2, midpoint - 2]);
    this.spawn(new Pokemon(target, 'Electabuzz'), [right - 2, midpoint]);
    this.spawn(new Pokemon(target, 'Diglett'), [right - 2, midpoint + 2]);
    this.player = player;
  }
  advanceActor() {
    this._current_actor = (this._current_actor + 1) % this.actors.length;
  }
  getCurrentActor() {
    return this.actors[this._current_actor];
  }
  getActorAt(position) {
    return this._position_actor_map[util.key(position)];
  }
  isSquareFree(position) {
    return util.inBounds(position, this.size) && !this.getActorAt(position);
  }
  move(actor, position) {
    assert(this.isSquareFree(position));
    delete this._position_actor_map[util.key(actor.position)];
    this._position_actor_map[util.key(position)] = actor;
    actor.position = position;
  }
  swap(actor, position) {
    const other = this.getActorAt(position);
    delete this._position_actor_map[util.key(other.position)];
    other.position = actor.position;
    this.move(actor, position);
    this.move(other, other.position);
  }
  spawn(actor, position) {
    actor.stage = this;
    actor.position = position;
    this.actors.push(actor);
    this.move(actor, position);
  }
}

class Timer {
  constructor() {
    this._timeout = 1 << 8;
    this._time_left = _.random(0, this._timeout);
  }
  ready() {
    return this._time_left < 0;
  }
  wait(turns) {
    this._time_left += Math.ceil(this._timeout*turns);
    return this.ready();
  }
}

// Graphics code follows.

class Effect {
  constructor() {}
  update(buffer) {
    return true;
  }
  _update(buffer) {
    const completed = this._completed;
    this._completed = this._completed || !this.update(buffer);
    return !completed;
  }
}

class FireEffect extends Effect {
  constructor(point) {
    super();
    this.point = point;
    this.index = 0;
    this.cycle = '**++';
    this.offset = _.random(this.cycle.length - 1);
  }
  update(buffer) {
    this.index += 1;
    const index = (this.index + this.offset) % this.cycle.length;
    buffer[this.point[1]][this.point[0]] = this.cycle[index];
    return this.index < 4;
  }
}

class Graphics {
  constructor(stage, handler) {
    const blessed = require('blessed');
    this.screen = blessed.screen({smartCSR: true});
    this.stage = blessed.box({
      width: stage.size[0],
      height: stage.size[1],
      tags: true,
    });
    this.status = blessed.box({
      left: stage.size[0] + 1,
      tags: true,
    });
    this.log = blessed.box({
      top: stage.size[1] + 1,
      tags: true,
    });

    this.screen.title = 'rogue';
    this.screen.append(this.stage);
    this.screen.append(this.status);
    this.screen.append(this.log);

    const alphabet = 'abcdefghijklmnopqrstuvwxyz'.split('');
    const keys = alphabet.concat(alphabet.map((x) => `S-${x}`))
                         .concat(['.', '>']);
    this.screen.key(keys, function(ch, key) {
      handler(ch);
    });
    this.screen.key(['C-c', 'escape'], function(ch, key) {
      return process.exit(0);
    });

    this._buffer = _.range(stage.size[1]).map(
        () => _.range(stage.size[0]).map(() => '.'))
    this._effects = [];
  }
  hasEffects() {
    return this._effects.length > 0;
  }
  render(stage, log, effects) {
    // Render the stage.
    for (let y = 0; y < stage.size[1]; y++) {
      for (let x = 0; x < stage.size[0]; x++) {
        this._buffer[y][x] = '.';
      }
    }
    for (let actor of stage.actors) {
      this._buffer[actor.position[1]][actor.position[0]] = actor.glyph;
    }
    this._effects = this._effects.concat(effects);
    this._effects = this._effects.filter((x) => x._update(this._buffer));
    this.stage.setContent(this._buffer.map((x) => x.join('')).join('\n'));
    // Render the status.
    const status = [];
    for (let actor of stage.actors) {
      if (actor instanceof Pokemon && actor.trainer instanceof Player) {
        ['\n', actor.description(), '\n'].map((x) => status.push(x));
      }
    }
    this.status.setContent(status.join(''));
    // Render the log.
    this.log.setContent(log.map((entry) => {
      const follow_up = entry.count > 1 ? ` (x${entry.count})` : '';
      return `${entry.message}${follow_up}`;
    }).join('\n'));
    this.screen.render();
  }
}

const game = new Game;
game.update();
