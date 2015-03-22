window.BabelAnimation = function() {
"use strict";

// The number of frames taken for each animation type.
var kAttackFrames = 4;

// The number of frames taken to tween between animation checkpoints.
var kTweenFrames = 4;

// The side length of each grid square, in pixels.
var kGridSize = 16;

function ASSERT(condition, message) {
  if (!condition) {
    console.log('ASSERTION FAILED:', message);
  }
}

function BabelAnimation(engine, graphics, radius) {
  this.engine = engine;
  this.graphics = graphics;
  this.radius = radius;
  this.last = this.Snapshot();
  this.tween = null;
  this.steps = [];
}

BabelAnimation.prototype.OnAttack = function(source, target) {
  var view = this.Snapshot();
  var component = new AttackComponent(view, source, target);
  this.PushStep({component: component, view: view});
}

BabelAnimation.prototype.Checkpoint = function() {
  this.PushStep({component: new CheckpointComponent(), view: this.Snapshot()});
}

BabelAnimation.prototype.Draw = function() {
  if (this.tween !== null) {
    this.tween.Draw(this.graphics);
  } else if (this.steps.length > 0) {
    this.steps[0].component.Draw(this.steps[0].view, this.graphics);
  } else {
    ASSERT(false, "Draw called while no animation was running!");
  }
}

BabelAnimation.prototype.Update = function() {
  while (true) {
    while (this.steps.length > 0) {
      if (this.tween !== null && this.tween.Update()) {
        return true;
      }
      this.tween = null;
      if (this.steps[0].component.Update()) {
        return true;
      }
      this.PopStep();
    }
    if (this.engine.Update()) {
      this.Checkpoint();
    } else {
      return false;
    }
  }
}

// The remaining BabelAnimation methods are all private.

BabelAnimation.prototype.Snapshot = function() {
  var result = {};
  var view = this.engine.GetView(this.radius);

  result.offset = view.offset;

  result.tiles = [];
  var tiles = view.tiles;
  for (var x = 0; x < tiles.size(); x++) {
    result.tiles.push([]);
    var column = tiles.get(x);
    for (var y = 0; y < tiles.size(); y++) {
      result.tiles[x].push(column.get(y));
    }
    column.delete();
  }
  tiles.delete();

  result.sprites = {};
  var sprites = view.sprites;
  for (var i = 0; i < sprites.size(); i++) {
    var sprite = sprites.get(i);
    result.sprites[sprite.id] = sprite;
  }
  sprites.delete();

  result.log = []
  var log = view.log;
  for (var j = 0; j < log.size(); j++) {
    result.log.push(log.get(j));
  }
  log.delete();

  result.status = view.status;

  view.delete();
  return result;
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

function AttackComponent(view, source, target) {
  this.source = source;
  var move = {x: 0, y: 0};
  if (view.sprites.hasOwnProperty(source) &&
      view.sprites.hasOwnProperty(target)) {
    var start = view.sprites[source].square;
    var end = view.sprites[target].square;
    move.x = kGridSize*(end.x - start.x)/4;
    move.y = kGridSize*(end.y - start.y)/4;
  }

  this.transform = new Transform();
  this.transform.sprite_offsets[source] = move;
  this.transform.shaded_sprites[target] = 0xff0000;
  this.frames_left = kAttackFrames;
}

AttackComponent.prototype.Update = function() {
  if (this.frames_left < kAttackFrames) {
    delete this.transform.sprite_offsets[this.source];
  }
  this.frames_left -= 1;
  return this.frames_left >= 0;
}

AttackComponent.prototype.Draw = function(view, graphics) {
  graphics.Draw(view, this.transform);
}

// Implementation of tweening logic.

function Tween(start, end) {
  this.start = start;
  this.end = end;

  this.events = [];
  this.frame = 0;
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
        this.transform.hidden_sprites[id] = true;
      }
    }
  }
}

Tween.prototype.Update = function() {
  this.frame += 1;
  if (this.frame > kTweenFrames ||
      (this.events.length === 0 && this.frame > 1)) {
    return false;
  }
  for (var i = 0; i < this.events.length; i++) {
    this.events[i].Update(this.frame, this.transform);
  }
  return true;
}

Tween.prototype.Draw = function(graphics) {
  ASSERT(this.frame <= kTweenFrames, "Tween.Draw called without frames left!");
  if (this.frame < kTweenFrames && this.events.length > 0) {
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
  this.sprite_offsets = {};

  // Map from sprite ID -> true. If the ID is in the map, the sprite is hidden.
  this.hidden_sprites = {};

  // Map from serialized points (2, 3) -> '2,3' to a tint for that square.
  this.shaded_sprites = {};
}

return BabelAnimation;
}();
