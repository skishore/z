window.BabelGraphics = function() {
"use strict";

function BabelGraphics(target, radius, onload) {
  this.target = target;
  this.radius = radius;
  this.onload = onload;

  this.log = this.target.find('.log');
  this.status = $('<div>').addClass('line');
  this.target.find('.status').append(this.status);

  // Core graphics constants.
  this.tile_textures = [];
  this.sprite_textures = [];
  // These should be read from the JSON files instead of hardcoded.
  this.num_tiles = 6;
  this.num_sprites = 5;
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

  this.scale = 1.5;
  this.renderer.view.style.width = Math.floor(this.scale*this.width) + "px";
  this.renderer.view.style.height = Math.floor(this.scale*this.height) + "px";

  // Insert the new renderer at the top of the DOM.
  this.target.append($(this.renderer.view));

  this.stats = new PIXI.Stats();
  $('body').append($(this.stats.domElement));
  this.stats.domElement.style.position = "fixed";
  this.stats.domElement.style.top = "0px";
  this.stats.domElement.style.left = "0px";

  this.layout = new BabelLayout(this.scale, this.square, this.radius);
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
  this.onload();
}

BabelGraphics.prototype.Reset = function(animation) {
  this.animation = animation;
  this.Draw(this.animation.last, null);
}

BabelGraphics.prototype.Update = function() {
  this.stats.begin();
  if (this.animation.Update()) {
    this.animation.Draw();
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
    this.DrawTransformedTiles(view, transform);
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
        sprite._babel_graphic = graphic;
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
        sprite.tint = 0xffffff;

        var graphic = sprite_view.graphic;
        if (sprite._babel_graphic !== graphic) {
          sprite.setTexture(this.sprite_textures[graphic]);
          sprite._babel_graphic = graphic;
        }
      } else {
        this.stage.removeChild(sprite);
        delete this.sprites[id];
      }
    }
  }
  Session.set('labels', this.layout.place(
      view, this.sprites, transform === null));
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

BabelGraphics.prototype.DrawTransformedTiles = function(view, transform) {
  this.container.x = -transform.camera_offset.x;
  this.container.y = -transform.camera_offset.y;
  for (var key in transform.shaded_sprites) {
    if (transform.shaded_sprites.hasOwnProperty(key) &&
        this.sprites.hasOwnProperty(key)) {
      this.sprites[key].tint = transform.shaded_sprites[key];
    }
  }
}

BabelGraphics.prototype.DrawUI = function(view) {
  Session.set('log', view.log);
  Session.set('status.cur_health', view.status.cur_health);
  Session.set('status.max_health', view.status.max_health);
}

BabelGraphics.prototype.DrawLabel = function(sprite, text, orientation) {
  assert(this.kLabelOffset.hasOwnProperty(orientation),
         "Invalid orientation: " + orientation);
  var offset = this.kLabelOffset[orientation];
  return {
    left: Math.floor(this.scale*(sprite.x) + offset[0]),
    top: Math.floor(this.scale*(sprite.y) + offset[1]),
    orientation: orientation,
    text: text,
  };
}

return BabelGraphics;
}();
