var BabelAnimation = function() {
"use strict";

var kAttackFrames = 4;
var kTweenFrames = 4;
var kGridSize = 16;

function ASSERT(condition, message) {
  if (!condition) {
    console.log('ASSERTION FAILED:', message);
  }
}

function BabelAnimation(radius, bindings) {
  this.radius = radius;
  this.bindings = bindings;
  this.last = null;
  this.tween = null;
  this.steps = [];
}

BabelAnimation.prototype.BeforeAttack = function(source, dest) {
  this.PushStep({component: new AttackComponent(dest), view: this.Snapshot()});
}

BabelAnimation.prototype.Checkpoint = function() {
  this.PushStep({component: new CheckpointComponent(), view: this.Snapshot()});
}

BabelAnimation.prototype.Draw = function() {
  if (this.tween !== null) {
    this.tween.Draw(this.bindings.graphics);
  } else if (this.steps.length > 0) {
    this.steps[0].component.Draw(this.steps[0].view, this.bindings.graphics);
  } else {
    ASSERT(false, "Draw called while no animation was running!");
  }
}

BabelAnimation.prototype.Update = function() {
  while (this.steps.length > 0) {
    if (this.tween !== null && this.tween.Update()) {
      return true;
    }
    this.tween = null;
    if (this.steps[0].component.Update()) {
      return true;
    }
    PopStep();
  }
  return false;
}

// The remaining BabelAnimation methods are all private.

BabelAnimation.prototype.Snapshot = function() {
  var view = this.bindings.engine.GetView(this.radius);
  view.delete();
  ASSERT(false, "Snapshot is not implemented!");
}

BabelAnimation.prototype.PushStep = function(step) {
  if (this.tween === null) {
    this.tween = new Tween(this.last, step.view);
  }
  this.steps.push(step);
}

BabelAnimation.prototype.PopStep = function() {
  ASSERT(this.tween === null, "PopStep called while a tween was running!");
  ASSERT(this.steps.length > 0, "PopStep called while steps was empty!");
  this.last = this.steps[0].view;
  this.steps = this.steps.slice(1);
  if (this.steps.length > 0) {
    this.tween = new Tween(this.last, this.steps[0].view);
  } else {
    this.last.log.length = 0;
  }
}

// Implementations of the various Animation components.

function CheckpointComponent() {}

CheckpointComponent.prototype.Update = function() {
  return false;
}

CheckpointComponent.prototype.Draw = function(view, graphics) {
  ASSERT(false, "CheckpointComponent.Draw should never be called!");
}

function AttackComponent(target) {
  var key = '' + target.x + ',' + target.y;
  this.transform = new Transform();
  this.transform.shaded_squares[key] = {color: 0x00ff0000, alpha: 0.5};
  this.frames_left = kAttackFrames;
}

AttackComponent.prototype.Update = function() {
  this.frames_left -= 1;
  return this.frames_left >= 0;
}

AttackComponent.prototype.Draw = function(view, graphics) {
  graphics.Draw(view, transform);
}

// Implementation of tweening logic.

function Tween(start, end) {
  this.start = start;
  this.end = end;

  this.events = [];
  this.frames_left = kTweenFrames;
  this.transform = new Transform();

  if (this.end.offset.x !== this.start.offset.x ||
      this.end.offset.y !== this.start.offset.y) {
    this.events.push(new CameraEvent({
        x: this.end.offset.x - this.start.offset.x,
        y: this.end.offset.y - this.start.offset.y}));
  }
  for (var id in this.start.sprites) {
    if (this.start.sprites.hasOwnProperty(id)) {
      if (this.end.sprites.hasOwnProperty(id)) {
        var prev = {
            x: this.start.sprites[id].square.x + this.start.offset.x,
            y: this.start.sprites[id].square.y + this.start.offset.y};
        var next = {
            x: this.end.sprites[id].square.x + this.end.offset.x,
            y: this.end.sprites[id].square.y + this.end.offset.y};
        if (prev.x !== next.x || prev.y !== next.y) {
          this.events.push(new MoveEvent(
              id, {x: next.x - prev.x, y: next.y - prev.y}));
        }
      } else {
        this.hidden_sprites[id] = true;
      }
    }
  }
}

Tween.prototype.Update = function() {
  this.frames_left -= 1;
  if (this.frames_left < 0 || this.events.length === 0) {
    return false;
  }
  for (var i = 0; i < this.events.length; i++) {
    this.events[i].Update(kTweenFrames - this.frames_left, this.transform);
  }
  return true;
}

Tween.prototype.Draw(graphics) {
  ASSERT(this.frames_left >= 0, "Tween.Draw called without frames left!");
  if (this.frames_left > 0 && this.events.length > 0) {
    graphics.Draw(this.start, this.transform);
  } else {
    graphics.Draw(this.end, null);
  }
}

// Implementation of individual events for tweens.

function CameraEvent(move) {
  this.move = move;
}

CameraEvent.prototype.Update = function(frame, transform) {
  transform.camera_offset = {
      x: Math.floor(frame*kGridSize*this.move.x/kTweenFrames),
      y: Math.floor(frame*kGridSize*this.move.y/kTweenFrames)};
}

function MoveEvent(id, move) {
  this.id = id;
  this.move = move;
}

MoveEvent.prototype.Update = function(frame, transform) {
  transform.sprite_offsets[this.id] = {
      x: Math.floor(frame*kGridSize*this.move.x/kTweenFrames),
      y: Math.floor(frame*kGridSize*this.move.y/kTweenFrames)};
}

// Implementation of the Transform class for computing diffs on top of views.

function Transform() {
  // A pixel offset of the camera from the original view.
  this.camera_offset = {x: 0, y: 0};

  // Map from sprite ID -> pixel offsets for the sprite from the original view.
  this.sprites_offset = {};

  // Map from sprite ID -> true. If the ID is in the map, the sprite is hidden.
  this.hidden_sprites = {};

  // Map from serialized points (2, 3) -> '2,3' to a tint for that square.
  this.shaded_squares = {};
}

return BabelAnimation;
}();
