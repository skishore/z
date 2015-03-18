window.BabelBindings = function() {
"use strict"

function BabelBindings(target) {
  this.radius = 9;
  var onload = this.OnAssetsLoaded.bind(this);
  this.graphics = new BabelGraphics(target, this.radius, onload);
}

BabelBindings.prototype.OnAssetsLoaded = function() {
  this.Reset();
  requestAnimationFrame(this.graphics.Update.bind(this.graphics));
}

BabelBindings.prototype.Reset = function() {
  this.engine = new Module.BabelEngine();
  this.engine.AddEventHandler(Module.BabelEventHandler.implement(this));
  this.animation = new BabelAnimation(this.engine, this.graphics, this.radius);
  this.input = new BabelInput(this.engine, this.Reset.bind(this));
  this.graphics.Reset(this.animation);
}

// Implement the engine's EventHandler callback.s

BabelBindings.prototype.BeforeAttack = function(source, target) {
  this.animation.BeforeAttack(source, target);
}

BabelBindings.prototype.LaunchDialog = function(source) {
  DialogManager.instantiate_random_dialog();
  this.input.RegisterDialog(DialogManager);
}

return BabelBindings;
}();
