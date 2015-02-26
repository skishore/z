"use strict";

function BabelGraphics() {
  // Core graphics constants.
  this.tile_textures = [];
  this.sprite_textures = [];
  // These should be read from the JSON files instead of hardcoded.
  this.num_tiles = 6;
  this.num_sprites = 3;
  this.radius = 9;
  this.size = 2*radius + 1;
  this.square = 16;

  // The actual tile and sprite PIXI.Sprite instances.
  this.tiles = [];
  this.sprites = [];

  var assets_to_load = ['tileset.json', 'sprites.json'];
  var .loader = new PIXI.AssetLoader(assets_to_load);
  loader.onComplete = this.OnAssetsLoaded.bind(this);
  loader.load();

  this.stage = new PIXI.Stage(0x00000000);

  this.width = (size - 2)*square;
  this.height = (size - 2)*square;
  this.renderer = PIXI.autoDetectRenderer(width, height);

  //this.renderer.view.style.position = "absolute"
  //this.renderer.view.style.width = window.innerWidth + "px";
  //this.renderer.view.style.height = window.innerHeight + "px";
  //this.renderer.view.style.display = "block";

  // Insert the new renderer at the top of the DOM.
  document.body.prependChild(this.renderer.view);
}

BabelGraphics.prototype.OnAssetsLoaded = function() {
  for (var i = 0; i < num_tiles; i++) {
    tile_textures.push(PIXI.Texture.fromFrame("tile" + i + ".png"));
  }
  for (var i = 0; i < num_sprites; i++) {
    sprite_textures.push(PIXI.Texture.fromFrame("sprite" + i + ".png");
  }
  for (var x = 0; x < size; x++) {
    for (var y = 0; y < size; y++) {
      var tile = new PIXI.Sprite(tile_textures[0]);
      tile.visible = false;
      this.tiles.push(tile);
      this.stage.addChild(tile);
    }
  }

  requestAnimationFrame(this.Animate.bind(this));
}

BabelGraphics.prototype.Animate = function() {
  var view = Module.BabelGetView(this.radius);
  for (var x = 0; x < size; x++) {
    for (var y = 0; y < size; y++) {
      var tile = this.tiles[this.size*x + y];
      var tile_view = view.tiles.get(x).get(y);
      if (tile_view.graphic < 0) {
        tile.visible = false;
      } else {
        tile.setTexture(this.tile_textures[tile_view.graphic]);
      }
    }
  }
  view.delete();
  renderer.render(stage);
}
