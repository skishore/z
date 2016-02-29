import {Vec} from '../piecemeal/vec';

import {Stage} from '../engine/stage';

import {Actor, Player, Pokemon} from './actor';
import {ILogEntry} from './game';
import {Buffer, Effect} from './effect';
import {_} from './util';

/* tslint:disable */
declare const process: any;
declare function require(name: string): any;
/* tslint:enable */

export class Graphics {
  private _screen: any;
  private _stage: any;
  private _status: any;
  private _log: any;
  private _buffer: Buffer;
  private _effects = new Array<Effect>();

  constructor(size: Vec, handler: any) {
    /* tslint:disable:no-require-imports */
    const blessed = require('blessed');
    /* tslint:enable */
    this._screen = blessed.screen({smartCSR: true});
    this._stage = blessed.box({
      width: size.x,
      height: size.y,
      tags: true,
    });
    this._status = blessed.box({
      left: size.x + 1,
      tags: true,
    });
    this._log = blessed.box({
      top: size.y + 1,
      tags: true,
    });

    this._screen.title = 'pokehack';
    this._screen.append(this._stage);
    this._screen.append(this._status);
    this._screen.append(this._log);

    const alphabet = 'abcdefghijklmnopqrstuvwxyz'.split('');
    const keys = alphabet.concat(alphabet.map((x) => `S-${x}`))
                         .concat(['.', '>']);
    this._screen.key(keys, handler);
    this._screen.key(['C-c', 'escape'], () => {
      return process.exit(0);
    });

    this._buffer = _.range(size.y).map(() => _.range(size.x).map(() => ' '));
  }

  get animated() { return this._effects.length > 0; }

  render(stage: Stage<Actor>, log: Array<ILogEntry>, effects: Array<Effect>) {
    // Render the stage, with its tiles, actors, and
    for (let y = 0; y < stage.size.y; y++) {
      for (let x = 0; x < stage.size.x; x++) {
        this._buffer[y][x] = stage.getTile(new Vec(x, y)).type.appearance;
      }
    }
    for (let actor of stage.actors) {
      this._buffer[actor.position.y][actor.position.x] = actor.glyph;
    }
    this._effects = this._effects.concat(effects);
    this._effects = this._effects.filter((x) => x.wrap(this._buffer));
    this._stage.setContent(this._buffer.map((x) => x.join('')).join('\n'));
    // Render the status.
    const status = new Array<string>();
    for (let actor of stage.actors) {
      if (actor instanceof Pokemon && actor.trainer instanceof Player) {
        ['\n', actor.description, '\n'].map((x) => status.push(x));
      }
    }
    this._status.setContent(status.join(''));
    // Render the log.
    this._log.setContent(log.map((entry) => {
      const suffix = entry.count > 1 ? ` (x${entry.count})` : '';
      return `${entry.message}${suffix}`;
    }).join('\n'));
    this._screen.render();
  }
}
