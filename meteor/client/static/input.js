window.BabelInput = function() {
"use strict";

function BabelInput(engine, reset) {
  this.engine = engine;
  this.reset = reset;

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

  Session.set('dialog.active', false);
  this.dialog = null;

  window.onkeypress = this.OnKeyPress.bind(this);
}

BabelInput.prototype.OnKeyPress = function(e) {
  var key = String.fromCharCode(e.keyCode);
  if (this.dialog !== null) {
    this.dialog.OnTextInput(key);
  } else if (this.move_map.hasOwnProperty(key)) {
    var move = this.move_map[key];
    this.engine.AddInput(Module.MakeMoveAction(move));
  } else if (key === 'r') {
    this.reset();
  }
  e.preventDefault();
  e.stopPropagation();
}

BabelInput.prototype.PressRandomKey = function(e) {
  var key = 'r';
  while (key === 'r') {
    key = String.fromCharCode(Math.floor(26*Math.random() + 'a'.charCodeAt(0)));
  }
  this.OnKeyPress({keyCode: key.charCodeAt(0), stopPropagation: function() {}});
}

BabelInput.prototype.RegisterDialog = function(dialog) {
  Session.set('dialog.active', true);
  this.dialog = dialog;
}

return BabelInput;
}();