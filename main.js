"use strict";

const _ = require('lodash');

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
  bind(entity) {
    this.entity = entity;
    this.stage = entity.stage;
    this._game = entity.stage.game;
  }
  log(message) {
    this._game.log(message);
  }
  logForPlayer(message) {
    if (this.entity instanceof Player) {
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
  bind(entity) {
    super.bind(entity);
    this.last = this.last || this.entity.position;
    this.diff = util.subtract(this.target, this.entity.position);
    this.length = util.length(this.diff);
  }
  execute(effects) {
    let result = null;
    for (let i of _.range(4)) {
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
      point = util.add(this.entity.position, step);
    }
    if (!util.inBounds(point, this.stage.size)) {
      return ActionResult.success();
    }
    effects.push(new FireEffect(point));
    const other = this.stage.getEntityAt(point);
    if (other) {
      this.log(`${this.entity.description()} attacks ${other.description()}.`);
      return ActionResult.success();
    }
    return ActionResult.incomplete();
  }
}

class HoverAroundPositionAction extends Action {
  constructor(target) {
    super();
    this.target = target;
  }
  execute() {
    const diff = util.subtract(this.target, this.entity.position);
    if (Math.max(Math.abs(diff.x), Math.abs(diff.y)) < 4) {
      const step = [_.random(-1, 1), _.random(-1, 1)];
      return ActionResult.alternate(new MovementAction(step));
    }
    const samples = [];
    _.range(-1, 2).map((x) => _.range(-1, 2).map((y) => {
      _.range(1).map(() => samples.push([x, y]));
      _.range(x*Math.sign(diff[0]) + 1).map(() => samples.push([x, y]));
      _.range(y*Math.sign(diff[1]) + 1).map(() => samples.push([x, y]));
    }));
    return ActionResult.alternate(new MovementAction(_.sample(samples)));
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
    const position = util.add(this.step, this.entity.position);
    if (!util.inBounds(position, this.stage.size)) {
      this.logForPlayer("You can't run from a trainer battle!");
      return ActionResult.failed();
    }
    const other = this.stage.getEntityAt(position);
    if (other) {
      if (other instanceof Pokemon && other.trainer === this.entity) {
        this.stage.swap(this.entity, position);
        this.logForPlayer(`You switch places with ${other.description()}.`);
      } else {
        this.logForPlayer(`You bump into ${other.description()}.`);
      }
      return ActionResult.success();
    }
    this.stage.move(this.entity, position);
    return ActionResult.success();
  }
}

class MoveToPositionAction extends Action {
  constructor(target) {
    super();
    this.target = target;
  }
  execute() {
    let distance = util.distance(this.target, this.entity.position);
    let move = [0, 0];
    _.range(-1, 2).map((x) => _.range(-1, 2).map((y) => {
      const position = util.add(this.entity.position, [x, y]);
      if (this.stage.getEntityAt(position) &&
          !util.equal(position, this.target)) {
        return;
      }
      const proposal = util.distance(this.target, position);
      if (proposal < distance) {
        distance = proposal;
        move = [x, y];
      }
    }));
    return ActionResult.alternate(new MovementAction(move));
  }
}

// The various Entities follow.

class Entity {
  constructor() {
    // this.position and this.stage are set on spawning.
    this.speed = 0.1;
    this.timer = new Timer;
  }
  description() {
    return 'an unknown entity';
  }
  getNextAction() {
    return new MovementAction([_.random(-1, 1), _.random(-1, 1)]);
  }
}

class Pokemon extends Entity {
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
  getNextAction() {
    if (this.trainer) {
      if (Math.random() < 0.0) {
        const targets = this.stage.entities.filter(
            (x) => x instanceof Pokemon && x.trainer !== this.trainer);
        if (targets.length > 0) {
          return new FireAction(_.sample(targets).position);
        }
      }
      return new HoverAroundPositionAction(this.trainer.position);
    }
    return super.getNextAction();
  }
  _allyWith(trainer) {
    if (!trainer) {
      return;
    }
    this.trainer = trainer;
    this.trainer.pokemon.push(this);
  }
}

class Trainer extends Entity {
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
  getNextAction() {
    assert(false, 'Player.getNextAction was called!');
  }
}

// Core game logic follows.

class Game {
  constructor() {
    this.stage = new Stage(this);
    this._log = [];
    this._action = null;
    this._next_player_action = null;

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
    if (this._action || this._next_player_action) {
      return false;
    }
    if (moves[ch]) {
      this._next_player_action = new MovementAction(moves[ch]);
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
  }
  tick() {
    const update = {advanced: false, effects: []};
    while (true) {
      // Consume existing actions, if any are available.
      if (this._action) {
        const entity = this._action.entity;
        const result = this._action.execute(update.effects);
        assert(result instanceof ActionResult, result);
        if (result.alternate) {
          result.alternate.bind(entity);
          this._action = result.alternate;
          continue;
        }
        update.advanced = true;
        if (result.done) {
          delete this._action;
          if (result.success || !(entity instanceof Player)) {
            entity.timer.wait(result.turns);
            this.stage.advanceEntity();
          }
          if (entity instanceof Player) {
            return update;
          }
        }
        if (update.effects.length > 0) {
          return update;
        }
      }
      // Construct new actions, if any are required.
      if (!this._action) {
        const entity = this.stage.getCurrentEntity();
        if (entity.timer.ready() || entity.timer.wait(-entity.speed)) {
          if (entity instanceof Player) {
            if (!this._next_player_action) {
              return update;
            }
            this._action = this._next_player_action;
            delete this._next_player_action;
          } else {
            this._action = entity.getNextAction();
          }
          this._action.bind(entity);
        } else {
          this.stage.advanceEntity();
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
    this.entities = [];
    this._current_entity = 0;
    this._position_entity_map = {};

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
  advanceEntity() {
    this._current_entity = (this._current_entity + 1) % this.entities.length;
  }
  getCurrentEntity() {
    return this.entities[this._current_entity];
  }
  getEntityAt(position) {
    return this._position_entity_map[util.key(position)];
  }
  move(entity, position) {
    assert(util.inBounds(position, this.size));
    assert(!this.getEntityAt(position));
    delete this._position_entity_map[util.key(entity.position)];
    this._position_entity_map[util.key(position)] = entity;
    entity.position = position;
  }
  swap(entity, position) {
    const other = this.getEntityAt(position);
    delete this._position_entity_map[util.key(other.position)];
    other.position = entity.position;
    this.move(entity, position);
    this.move(other, other.position);
  }
  spawn(entity, position) {
    entity.stage = this;
    entity.position = position;
    this.entities.push(entity);
    this.move(entity, position);
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
    this.screen.key(alphabet.concat(['.']), function(ch, key) {
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
    for (let entity of stage.entities) {
      this._buffer[entity.position[1]][entity.position[0]] = entity.glyph;
    }
    this._effects = this._effects.concat(effects);
    this._effects = this._effects.filter((x) => x._update(this._buffer));
    this.stage.setContent(this._buffer.map((x) => x.join('')).join('\n'));
    // Render the status.
    const status = [];
    for (let entity of stage.entities) {
      if (entity instanceof Pokemon && entity.trainer instanceof Player) {
        ['\n', entity.description(), '\n'].map((x) => status.push(x));
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
