import math
import random
import struct
import sys


class Tileset(object):
  def __init__(self):
    self.default_tile = 4
    self.num_tiles = 5

  def blocked(self, tile):
    return tile == self.default_tile

  def get_free_tile(self):
    return random.randint(0, self.num_tiles - 2)


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


class Room(object):
  def __init__(self, width, height):
    self.width = width
    self.height = height

  def distance(self, room):
    x_distance = max(
      (room.x - self.x - self.width),
      (self.x - room.x - room.width), 0)
    y_distance = max(
      (room.y - self.y - self.height),
      (self.y - room.y - room.height), 0)
    return math.sqrt(x_distance**2 + y_distance**2)

  def place(self, map, rooms, tolerance):
    self.x = random.randint(0, map.width - self.width)
    self.y = random.randint(0, map.height - self.height)
    for room in rooms:
      if self.distance(room) <= tolerance:
        return False
    for w in xrange(self.width):
      for h in xrange(self.height):
        map.tiles[self.x + w][self.y + h] = map.tileset.get_free_tile()
    return True


def generate_random_map(width, height, tileset):
  map = Map(width, height, tileset)
  for w in xrange(width):
    for h in xrange(height):
      map.tiles[w][h] = random.randint(0, tileset.num_tiles - 1)
  return map


def sample(distribution):
  total = sum(distribution.itervalues())
  target = random.random()*total
  for (value, relative_probability) in distribution.iteritems():
    target -= relative_probability
    if target < 0:
      return value
  print 'WARNING: sample failed to find value!'
  return distribution.keys()[0]


def generate_mostly_linear_tree(n, bias=2):
  # Returns a list of edges [(0, 1), (1, 2), ...] that make up the tree.
  result = []
  depths = {0: 0}
  while len(result) < n:
    child = len(result) + 1
    distribution = \
        dict((node, bias**depth) for (node, depth) in depths.iteritems())
    parent = sample(distribution)
    result.append((parent, child))
    depths[child] = depths[parent] + 1
  return result


def generate_rooms_map(width, height, tileset):
  map = Map(width, height, tileset)
  (min_size, max_size) = (2, 4)
  tries = width*height/(min_size**2)

  rooms = []
  while tries > 0:
    width = random.randint(min_size, max_size)
    height = random.randint(min_size, max_size)
    room = Room(width, height)
    if room.place(map, rooms, tolerance=min_size):
      rooms.append(room)
    else:
      tries -= 1

  room_graph = []
  for room in rooms:
    room_graph.append([])
    for other in rooms:
      room_graph[-1].append(room.distance(other))
  print room_graph

  return map


if __name__ == '__main__':
  random.seed()
  map = generate_rooms_map(32, 32, Tileset())
  print map
  #map.print_to_file('world.dat')
