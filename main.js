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
    const diff = util.subtract(point1, point2);
    return Math.sqrt(diff[0]*diff[0] + diff[1]*diff[1]);
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
    this.game = entity.stage.game;
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
  static success() {
    const result = new ActionResult;
    result.success = true;
    return result;
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
      if (this.entity instanceof Player) {
        this.game.log("You can't run from a trainer battle!");
      }
      return ActionResult.failed();
    }
    const other = this.stage.getEntityAt(position);
    if (other) {
      if (this.entity instanceof Player) {
        this.game.log(`You bump into ${other.description()}.`);
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
    return 'yourself';
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
    this.graphics.render(this.stage, this._log);
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
    let advanced = false;
    while (true) {
      // Consume existing actions, if any are available.
      if (this._action) {
        const entity = this._action.entity;
        const result = this._action.execute();
        if (result.alternate) {
          result.alternate.bind(entity);
          this._action = result.alternate;
          continue;
        }
        assert(result instanceof ActionResult, result);
        advanced = true;
        if (result.done) {
          delete this._action;
          if (result.success || !(entity instanceof Player)) {
            entity.timer.wait(result.turns);
            this.stage.advanceEntity();
          }
          if (entity instanceof Player) {
            return advanced;
          }
        }
      }
      // Construct new actions, if any are required.
      if (!this._action) {
        const entity = this.stage.getCurrentEntity();
        if (entity.timer.ready() || entity.timer.wait(-entity.speed)) {
          if (entity instanceof Player) {
            if (!this._next_player_action) {
              return advanced;
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
    if (this.tick()) {
      this.graphics.render(this.stage, this._log);
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
    this.spawn(player, [4, 7]);
    this.spawn(target, [35, 7]);
    this.spawn(new Pokemon(player, 'Bulbasaur'), [6, 5]);
    this.spawn(new Pokemon(player, 'Charmander'), [6, 7]);
    this.spawn(new Pokemon(player, 'Squirtle'), [6, 9]);
    this.spawn(new Pokemon(target, 'Poliwag'), [33, 5]);
    this.spawn(new Pokemon(target, 'Electabuzz'), [33, 7]);
    this.spawn(new Pokemon(target, 'Diglett'), [33, 9]);
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

class Graphics {
  constructor(stage, handler) {
    const blessed = require('blessed');
    this.screen = blessed.screen({smartCSR: true});
    this.stage = blessed.box({
      width: stage.size[0],
      height: stage.size[1],
      tags: true,
    });
    this.log = blessed.box({
      top: stage.size[1] + 1,
      tags: true,
    });

    this.screen.title = 'rogue';
    this.screen.append(this.stage);
    this.screen.append(this.log);

    const alphabet = 'abcdefghijklmnopqrstuvwxyz'.split('');
    this.screen.key(alphabet.concat(['.']), function(ch, key) {
      handler(ch);
    });
    this.screen.key(['C-c', 'escape'], function(ch, key) {
      return process.exit(0);
    });
  }
  render(stage, log) {
    const content = [];
    for (let y = 0; y < stage.size[1]; y++) {
      for (let x = 0; x < stage.size[0]; x++) {
        const entity = stage.getEntityAt([x, y]);
        if (entity) {
          content.push(entity.glyph);
        } else {
          content.push('.');
        }
      }
      content.push('\n');
    }
    this.stage.setContent(content.join(''));
    this.log.setContent(log.map((entry) => {
      return `${entry.message}${entry.count > 1 ? ` (x${entry.count})` : ''}`;
    }).join('\n'));
    this.screen.render();
  }
}

const game = new Game;
game.update();
