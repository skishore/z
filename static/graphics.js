var BabelGraphics = function() {
"use strict";

function BabelGraphics(target, bindings) {
  this.target = target;
  this.bindings = bindings;

  this.log = this.target.find('.log');
  this.status = $('<div>').addClass('line');
  this.target.find('.status').append(this.status);

  // Core graphics constants.
  this.tile_textures = [];
  this.sprite_textures = [];
  // These should be read from the JSON files instead of hardcoded.
  this.num_tiles = 6;
  this.num_sprites = 3;
  this.radius = 9;
  this.size = 2*this.radius + 1;
  this.square = 16;

  // The actual tile and sprite PIXI.Sprite instances.
  this.tiles = [];
  this.sprites = {};

  var assets_to_load = ['tileset.json', 'sprites.json'];
  var loader = new PIXI.AssetLoader(assets_to_load);
  loader.onComplete = this.OnAssetsLoaded.bind(this);
  loader.load();

  this.stage = new PIXI.Stage(0x00000000);
  this.container = new PIXI.DisplayObjectContainer();
  this.stage.addChild(this.container);

  this.width = (this.size - 2)*this.square;
  this.height = (this.size - 2)*this.square;
  this.renderer = PIXI.autoDetectRenderer(this.width, this.height);

  var scale = 1.5;
  this.renderer.view.style.width = Math.floor(scale*this.width) + "px";
  this.renderer.view.style.height = Math.floor(scale*this.height) + "px";

  // Insert the new renderer at the top of the DOM.
  this.target.append($(this.renderer.view));

  this.stats = new PIXI.Stats();
  $('body').append($(this.stats.domElement));
  this.stats.domElement.style.position = "fixed";
  this.stats.domElement.style.top = "0px";
  this.stats.domElement.style.left = "0px";
}

BabelGraphics.prototype.OnAssetsLoaded = function() {
  for (var i = 0; i < this.num_tiles; i++) {
    this.tile_textures.push(PIXI.Texture.fromFrame("tile" + i + ".png"));
  }
  for (var i = 0; i < this.num_sprites; i++) {
    this.sprite_textures.push(PIXI.Texture.fromFrame("sprite" + i + ".png"));
  }
  for (var x = 0; x < this.size; x++) {
    for (var y = 0; y < this.size; y++) {
      var tile = new PIXI.Sprite(this.tile_textures[0]);
      tile.x = this.square*(x - 1);
      tile.y = this.square*(y - 1);
      tile.visible = false;
      this.tiles.push(tile);
      this.container.addChild(tile);
    }
  }

  this.Reset();
  requestAnimationFrame(this.Update.bind(this));
}

BabelGraphics.prototype.Reset = function() {
  this.bindings.engine = new Module.BabelEngine();
  this.bindings.animation = new BabelAnimation(this.radius, this.bindings);
  this.Draw(this.bindings.animation.last, null);
}

BabelGraphics.prototype.Update = function() {
  this.stats.begin();
  if (this.bindings.animation.Update()) {
    this.bindings.animation.Draw();
  }
  requestAnimationFrame(this.Update.bind(this));
  this.stats.end();
}

BabelGraphics.prototype.Draw = function(view, transform) {
  this.DrawSprites(view, transform);
  if (transform === null) {
    this.DrawTiles(view);
    this.DrawUI(view);
    this.container.x = 0;
    this.container.y = 0;
  } else {
    this.container.x = -transform.camera_offset.x;
    this.container.y = -transform.camera_offset.y;
  }
  this.renderer.render(this.stage);
}

BabelGraphics.prototype.DrawSprites = function(view, transform) {
  for (var id in view.sprites) {
    if (view.sprites.hasOwnProperty(id) &&
        (transform === null || !transform.hidden_sprites[id])) {
      if (!this.sprites.hasOwnProperty(id)) {
        var graphic = view.sprites[id].graphic;
        var sprite = new PIXI.Sprite(this.sprite_textures[graphic]);
        this.sprites[id] = sprite;
        this.stage.addChild(sprite);
      }
    }
  }
  for (var id in this.sprites) {
    if (this.sprites.hasOwnProperty(id)) {
      var sprite = this.sprites[id];
      if (view.sprites.hasOwnProperty(id) &&
          (transform === null || !transform.hidden_sprites[id])) {
        var sprite_view = view.sprites[id];
        var offset = {x: 0, y: 0};
        if (transform !== null) {
          offset.x = -transform.camera_offset.x;
          offset.y = -transform.camera_offset.y;
          if (transform.sprite_offsets.hasOwnProperty(id)) {
            offset.x += transform.sprite_offsets[id].x;
            offset.y += transform.sprite_offsets[id].y;
          }
        }
        sprite.x = this.square*(sprite_view.square.x - 1) + offset.x;
        sprite.y = this.square*(sprite_view.square.y - 1) + offset.y;
      } else {
        this.stage.removeChild(sprite);
        delete this.sprites[id];
      }
    }
  }
}

BabelGraphics.prototype.DrawTiles = function(view) {
  for (var x = 0; x < this.size; x++) {
    for (var y = 0; y < this.size; y++) {
      var tile = this.tiles[this.size*x + y];
      var tile_view = view.tiles[x][y];
      if (tile_view.graphic < 0) {
        tile.visible = false;
      } else {
        tile.setTexture(this.tile_textures[tile_view.graphic]);
        tile.tint = (tile_view.visible ? 0xffffff : 0x888888);
        tile.visible = true;
      }
    }
  }
}

BabelGraphics.prototype.DrawUI = function(view) {
  if (view.log.length > 0) {
    this.log.children().remove();
    for (var i = 0; i < view.log.length; i++) {
      this.log.append($('<div>').addClass('line').text(view.log[i]));
    }
    this.log.show();
  } else {
    this.log.hide();
  }

  this.status.text(
    'Health: ' + view.status.cur_health + '/' + view.status.max_health);
}

return BabelGraphics;
}();
