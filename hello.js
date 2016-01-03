"use strict";

const blessed = require('blessed');

const size = [80, 32];
const screen = blessed.screen({smartCSR: true});
const game = blessed.box({width: size[0], height: size[1], tags: true});
const debug = blessed.box({left: size[0]});

screen.title = 'rogue';
screen.append(game);
screen.append(debug);
screen.key(['C-c', 'escape'], function(ch, key) {
  return process.exit(0);
});

const pad = (color) => `000000${color}`.slice(-6);

const randomColor = () => {
  return pad(Math.floor(Math.random()*0x1000000).toString(16));
}

const render = () => {
  let start = (new Date).getTime();

  const content = [];
  let last_color = randomColor();
  for (let y = 0; y < size[1]; y++) {
    for (let x = 0; x < size[0]; x++) {
      const color = Math.random() < 0.9 ? last_color : randomColor();
      if (color !== last_color) {
        content.push(`{/}${color ? `{#${color}-fg}` : ''}`);
        last_color = color;
      }
      content.push('.');
    }
    content.push('\n');
  }
  game.content = content.join('');
  screen.render();
  debug.content = '' + ((new Date).getTime() - start);
  setTimeout(render, 4);
}

render();
