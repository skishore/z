window.BabelBindings = function() {
"use strict"

function BabelBindings(target) {
  this.size = {x: 48, y: 24};
  var onload = this.OnAssetsLoaded.bind(this);
  this.graphics = new BabelGraphics(target, this.size, onload);
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
  this.animation = new BabelAnimation(this.engine, this.graphics, this.size);
  this.engine.AddEventHandler(
      Module.BabelEventHandler.implement(this.animation));
  this.input = new BabelInput(this.engine, this.Reset.bind(this));
  this.graphics.Reset(this.animation);
}

return BabelBindings;
}();
