var BabelInput = function() {
"use strict";

function BabelInput(target, engine) {
  this.engine = engine;

  this.move_map = {
    'h': {x: -1, y: 0},
    'j': {x: 0, y: 1},
    'k': {x: 0, y: -1},
    'l': {x: 1, y: 0},
    'y': {x: -1, y: -1},
    'u': {x: 1, y: -1},
    'b': {x: -1, y: 1},
    'n': {x: 1, y: 1},
    '.': {x: 0, y: 0},
  };

  window.onkeypress = this.OnKeyPress.bind(this);
}

BabelInput.prototype.OnKeyPress = function(e) {
  var key = String.fromCharCode(e.keyCode);
  if (this.move_map.hasOwnProperty(key)) {
    var move = this.move_map[key];
    this.engine.AddInput(Module.MakeMoveAction(move));
  }
  e.stopPropagation();
}

return BabelInput;
}();
