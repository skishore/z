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
  inBounds(point, size) {
    return 0 <= point[0] && point[0] < size[0] &&
           0 <= point[1] && point[1] < size[1];
  },
  key(point) {
    return point.join(',');
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
}

class IdleAction extends Action {
  constructor() {
    super();
  }
  execute() {
    const result = new ActionResult;
    result.success = true;
    return result;
  }
}

class MovementAction extends Action {
  constructor(step) {
    super();
    this.step = step;
  }
  execute() {
    const result = new ActionResult;
    if (this.step[0] === 0 && this.step[1] === 0) {
      result.alternate = new IdleAction;
      return result;
    }
    const position = util.add(this.step, this.entity.position);
    if (!util.inBounds(position, this.stage.size)) {
      if (this.entity instanceof Player) {
        this.game.log("You can't run from a trainer battle!");
      }
      return result;
    }
    const other = this.stage.getEntityAt(position);
    if (other) {
      if (this.entity instanceof Player) {
        this.game.log(`You bump into ${other.getDescription()}.`);
      }
      result.success = true;
      return result;
    }
    this.stage.move(this.entity, position);
    result.success = true;
    return result;
  }
}

// The various Entities follow.

class Entity {
  constructor() {
    // this.position and this.stage are set on spawning.
    this.speed = 0.1;
    this.timer = new Timer;
  }
  getDescription() {
    return 'an unknown entity';
  }
  getNextAction() {
    return new MovementAction([_.random(-1, 1), _.random(-1, 1)]);
  }
}

class Pokemon extends Entity {
  constructor(allegiance, breed) {
    super();
    this.allegiance = allegiance || this;
    this.breed = breed;
    this.glyph = breed[0];
  }
  getDescription() {
    return `${this.allegiance instanceof Player ? 'your ' : ''}${this.breed}`;
  }
}

class Trainer extends Entity {
  constructor() {
    super();
    this.allegiance = this;
    this.glyph = '@';
  }
  getDescription() {
    return 'the opposing trainer';
  }
}

class Player extends Trainer {
  constructor() {
    super();
  }
  getDescription() {
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
    this.spawn(new Pokemon(player, 'bulbasaur'), [6, 5]);
    this.spawn(new Pokemon(player, 'charmander'), [6, 7]);
    this.spawn(new Pokemon(player, 'squirtle'), [6, 9]);
    this.spawn(new Pokemon(target, 'poliwag'), [33, 5]);
    this.spawn(new Pokemon(target, 'electabuzz'), [33, 7]);
    this.spawn(new Pokemon(target, 'diglett'), [33, 9]);
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
