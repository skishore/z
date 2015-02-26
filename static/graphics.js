<!DOCTYPE HTML>
<html>
<head>
    <title>pixi.js example 2 loading a sprite sheet</title>
    <style>
        body {
            margin: 0;
            padding: 0;
            background-color: #000000;
        }
    </style>
    <script src="../../bin/pixi.dev.js"></script>
</head>
<body>
    <script>
    //PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST;

    // create an array of assets to load, in the form of json files generated from TexturePacker
    var assetsToLoader = ["tileset.json"];

    // create a new loader
    loader = new PIXI.AssetLoader(assetsToLoader);

    // use callback
    loader.onComplete = onAssetsLoaded;

    // begin load
    loader.load();

    // holder to store aliens
    var aliens = [];
    var alienFrames = [];
    var count = 0;

    // create an new instance of a pixi stage
    var stage = new PIXI.Stage(0xFFFFFF);

    // create a renderer instance.
    var width = window.innerWidth;
    var height = window.innerHeight;
    if (height > 1080) {
      width = Math.floor(width/2);
      height = Math.floor(height/2);
    }
    var renderer = PIXI.autoDetectRenderer(width, height);

    renderer.view.style.position = "absolute"
    renderer.view.style.width = window.innerWidth + "px";
    renderer.view.style.height = window.innerHeight + "px";
    renderer.view.style.display = "block";

    // add the renderer view element to the DOM
    document.body.appendChild(renderer.view);

    // create an empty container
    var alienContainer = new PIXI.DisplayObjectContainer();
    alienContainer.position.x = width/2;
    alienContainer.position.y = height/2;

    stage.addChild(alienContainer);

    var label = new PIXI.Text("Click to enable cacheAsBitmap", {fill:"black", stroke:"white", strokeThickness:5});
    label.position.x = 40;
    label.position.y = height - 40;
    stage.addChild(label)

    function onAssetsLoaded()
    {
        for (var i = 0; i < 6; i++) {
          alienFrames.push(PIXI.Texture.fromFrame("tile" + i + ".png"));
        }

        // add a bunch of aliens with textures from image paths
        for (var i = 0; i < 324; i++)
        {
            // Create a tile with a random texture.
            var texture = alienFrames[i % alienFrames.length];
            var alien = new PIXI.Sprite(texture);
            //alien.tint = Math.random() * 0xFFFFFF;

            /*
             * fun fact for the day :)
             * another way of doing the above would be
             * var alien = PIXI.Sprite.fromFrame(frameName);
             */
            alien.position.x = (Math.random() - 0.5)*width;
            alien.position.y = (Math.random() - 0.5)*height;
            alien.anchor.x = 0.5;
            alien.anchor.y = 0.5;
            aliens.push(alien);
            alienContainer.addChild(alien);
        }

        // start animating
        requestAnimFrame(animate);
    }

    stage.mousedown = stage.touchstart = function()
    {
        if (alienContainer.cacheAsBitmap) {
            label.setText("Click to enable cacheAsBitmap");
            alienContainer.cacheAsBitmap = false;
        } else {
            label.setText("Click to disable cacheAsBitmap");
            alienContainer.cacheAsBitmap = true;
        }
    }

    function animate() {
        // let's rotate the aliens a little bit
        for (var i = 0; i < aliens.length; i++)
        {
            var alien = aliens[i];
            alien.rotation += 0.1;
            if (Math.random() > 0.0) {
              var index = Math.floor(alienFrames.length*Math.random());
              alien.setTexture(alienFrames[index]);
            }
        }

        count += 0.01;
        alienContainer.scale.x = Math.sin(count);
        alienContainer.scale.y = Math.sin(count);

        alienContainer.rotation += 0.01;
        
        // render the stage
        renderer.render(stage);

        requestAnimFrame(animate);
    }
    </script>

    </body>
</html>
