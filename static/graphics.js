var BabelGraphics = function() {
"use strict";

function BabelGraphics(target, bindings) {
  this.bindings = bindings;

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
  this.sprite_map = {};

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

  var scale = 1.25;
  this.renderer.view.style.width = Math.floor(scale*this.width) + "px";
  this.renderer.view.style.height = Math.floor(scale*this.height) + "px";

  // Insert the new renderer at the top of the DOM.
  target.append($(this.renderer.view));

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

  this.Redraw();
  requestAnimationFrame(this.Animate.bind(this));
}

BabelGraphics.prototype.Animate = function() {
  this.stats.begin();
  if (this.bindings.engine.Update()) {
    this.Redraw();
  }
  requestAnimationFrame(this.Animate.bind(this));
  this.stats.end();
}

BabelGraphics.prototype.Redraw = function() {
  var view = this.bindings.engine.GetView(this.radius);

  var tiles = view.tiles;
  for (var x = 0; x < this.size; x++) {
    var column = tiles.get(x);
    for (var y = 0; y < this.size; y++) {
      var tile = this.tiles[this.size*x + y];
      var tile_view = column.get(y);
      if (tile_view.graphic < 0) {
        tile.visible = false;
      } else {
        tile.setTexture(this.tile_textures[tile_view.graphic]);
        tile.tint = (tile_view.visible ? 0xffffff : 0x888888);
        tile.visible = true;
      }
    }
    column.delete();
  }
  tiles.delete();

  var sprites = view.sprites;
  var sprite_map = {}
  for (var i = 0; i < sprites.size(); i++) {
    var sprite = sprites.get(i);
    sprite_map[sprite.id] = sprite;
  }
  sprites.delete();

  for (var id in sprite_map) {
    if (sprite_map.hasOwnProperty(id)) {
      if (!this.sprite_map.hasOwnProperty(id)) {
        var graphic = sprite_map[id].graphic;
        var sprite = new PIXI.Sprite(this.sprite_textures[graphic]);
        this.sprite_map[id] = sprite;
        this.stage.addChild(sprite);
      }
    }
  }
  for (var id in this.sprite_map) {
    if (this.sprite_map.hasOwnProperty(id)) {
      var sprite = this.sprite_map[id];
      if (sprite_map.hasOwnProperty(id)) {
        var sprite_view = sprite_map[id];
        sprite.x = this.square*(sprite_view.square.x - 1);
        sprite.y = this.square*(sprite_view.square.y - 1);
      } else {
        this.stage.removeChild(sprite);
        delete this.sprite_map[id];
      }
    }
  }

  view.delete();
  this.renderer.render(this.stage);
}

return BabelGraphics;
}();
