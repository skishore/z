DIRECTIONS = {up: [0, -1], right: [1, 0], down: [0, 1], left: [-1, 0]}
FRAME = {x: 96, y: 112, w: 16, h: 16}
FRAMES = 4

DOORS = {frames: {}, meta: {image: 'tileset.png'}}

for direction, [x, y] of DIRECTIONS
  for index in [0...FRAMES]
    frame = _.clone FRAME
    frame.x -= (frame.w*x)*(FRAMES - index)/FRAMES
    frame.y -= (frame.w*y)*(FRAMES - index)/FRAMES
    DOORS.frames["door-#{direction}-#{index}"] = {frame: frame}

DOORS_JSON = "#{JSON.stringify DOORS}\n"


Meteor.startup ->
  Router.route '/doors.json', (-> @response.end DOORS_JSON), {where: 'server'}
  Router.route '/'
