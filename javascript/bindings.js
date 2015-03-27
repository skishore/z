window.BabelBindings = function() {
"use strict"

function BabelBindings(target) {
  this.radius = {x: 12, y: 9};
  var onload = this.OnAssetsLoaded.bind(this);
  this.graphics = new BabelGraphics(target, this.radius, onload);
}

BabelBindings.prototype.OnAssetsLoaded = function() {
  this.Reset();
  requestAnimationFrame(this.graphics.Update.bind(this.graphics));
}

BabelBindings.prototype.Reset = function() {
  if (this.engine) {
    this.engine.delete();
  }
  this.engine = new Module.BabelEngine();
  this.animation = new BabelAnimation(this.engine, this.graphics, this.radius);
  this.engine.AddEventHandler(
      Module.BabelEventHandler.implement(this.animation));
  this.input = new BabelInput(this.engine, this.Reset.bind(this));
  this.graphics.Reset(this.animation);
}

return BabelBindings;
}();
