import random
import struct
import sys


class Tileset(object):
  def __init__(self):
    self.default_tile = 4
    self.num_tiles = 5

  def blocked(self, tile):
    return tile == self.default_tile


class Map(object):
  def __init__(self, width, height, tileset):
    self.width = width
    self.height = height
    self.tileset = tileset
    tile = tileset.default_tile
    self.tiles = [[tile for h in xrange(height)] for w in xrange(width)]

  def get_header(self):
    lines = []
    lines.append('width: %d' % (self.width,))
    lines.append('height: %d' % (self.height,))
    lines.append('default_tile: %d' % (self.tileset.default_tile,))
    return '\n'.join(lines)

  def __str__(self):
    result = [self.get_header()]
    for h in xrange(self.height):
      result.append(''.join(
        ('X' if self.tileset.blocked(self.tiles[w][h]) else '.')
        for w in xrange(self.width)
      ))
    return '\n'.join(result)

  def print_to_file(self, filename):
    with open(filename, 'wb') as file:
      file.write(self.get_header())
      file.write('\ntiles:\n')
      for w in xrange(self.width):
        for h in xrange(self.height):
          file.write(struct.pack('B', self.tiles[w][h]))


def generate_random_map(width, height, tileset):
  map = Map(width, height, tileset)
  for w in xrange(width):
    for h in xrange(height):
      map.tiles[w][h] = random.randint(0, tileset.num_tiles - 1)
  return map


if __name__ == '__main__':
  random.seed()
  map = generate_random_map(16, 16, Tileset())
  print map
  map.print_to_file('world.dat')
