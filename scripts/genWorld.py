import random
from struct import pack

def genRandomWorld(filename, width, height, default_tile, num_tiles):
  random.seed()
  with open(filename, 'wb') as map_file:
    map_file.write('width: %d\n' % (width,))
    map_file.write('height: %d\n' % (height,))
    map_file.write('default_tile: %d\n' % (default_tile,))
    map_file.write('tiles:\n')
    for i in range(width*height):
      map_file.write(pack('B', random.randint(0, num_tiles - 1)))

if __name__ == '__main__':
  # Generate a world that is 1024x1024 squares large.
  genRandomWorld('world.dat', 1<<10, 1<<10, 4, 5)
