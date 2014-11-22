import math
import random
import scipy.sparse.csgraph as csgraph
import string
import struct
import sys


class Tileset(object):
  def __init__(self):
    self.default_tile = 5
    self.num_tiles = 6
    self.chars = ['.', '.', '.', '.', 'X', ' ']

  def blocked(self, tile):
    return tile > 3

  def get_free_tile(self):
    return random.randint(0, 3)


class Map(object):
  def __init__(self, width, height, tileset):
    self.width = width
    self.height = height
    self.tileset = tileset
    tile = tileset.default_tile
    self.tiles = [[tile for h in xrange(height)] for w in xrange(width)]
    self.rooms = []

  def add_room(self, room):
    for w in xrange(room.width):
      for h in xrange(room.height):
        self.tiles[room.x + w][room.y + h] = self.tileset.get_free_tile()
    self.rooms.append(room)

  def dig_corridor(self, index1, index2):
    source = self.rooms[index1].midpoint()
    target = self.rooms[index2].midpoint()
    frontier = {source: 0}
    parents = {source: None}
    visited = set()
    while target not in visited:
      (best_node, best_distance) = (None, float('Infinity'))
      for (node, distance) in frontier.iteritems():
        if distance < best_distance:
          (best_node, best_distance) = (node, distance)
      del frontier[best_node]
      visited.add(best_node)
      for step in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        child = (best_node[0] + step[0], best_node[1] + step[1])
        if not (0 <= child[0] < self.width and 0 <= child[1] < self.height):
          continue
        if child in visited:
          continue
        tile = self.tiles[child[0]][child[1]]
        step_length = (1.0 if self.tileset.blocked(tile) else 2.0)
        distance = best_distance + (1 + random.random())*step_length
        if distance < frontier.get(child, float('Infinity')):
          frontier[child] = distance
          parents[child] = best_node
    node = target
    while node != None:
      if self.tileset.blocked(self.tiles[node[0]][node[1]]):
        self.tiles[node[0]][node[1]] = self.tileset.get_free_tile()
      node = parents[node]

  def get_header(self):
    lines = []
    lines.append('width: %d' % (self.width,))
    lines.append('height: %d' % (self.height,))
    lines.append('default_tile: %d' % (self.tileset.default_tile,))
    return '\n'.join(lines)

  def __str__(self):
    room_labels = string.digits + string.letters
    result = [self.get_header()]
    tiles = [
      [self.tileset.chars[tile] for tile in column]
      for column in self.tiles
    ]
    for (i, room) in enumerate(self.rooms):
      for w in xrange(room.width):
        for h in xrange(room.height):
          tiles[room.x + w][room.y + h] = room_labels[i % len(room_labels)]
    for h in xrange(self.height):
      result.append(''.join(tiles[w][h] for w in xrange(self.width)))
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

  def midpoint(self):
    return (self.x + self.width/2, self.y + self.height/2)

  def place(self, map, tolerance):
    self.x = random.randint(0, map.width - self.width)
    self.y = random.randint(0, map.height - self.height)
    for room in map.rooms:
      if self.distance(room) <= tolerance:
        return False
    return True


def generate_random_map(width, height, tileset):
  map = Map(width, height, tileset)
  for w in xrange(width):
    for h in xrange(height):
      map.tiles[w][h] = random.randint(0, 4)
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
  (min_size, max_size) = (4, 8)
  tries = width*height/(min_size**2)

  while tries > 0:
    width = random.randint(min_size, max_size)
    height = random.randint(min_size, max_size)
    room = Room(width, height)
    if room.place(map, tolerance=min_size):
      map.add_room(room)
    else:
      tries -= 1

  room_graph = []
  for room in map.rooms:
    room_graph.append([])
    for other in map.rooms:
      room_graph[-1].append(room.distance(other))
  spanning_tree = csgraph.minimum_spanning_tree(room_graph)
  for (index1, index2) in zip(*spanning_tree.nonzero()):
    map.dig_corridor(index1, index2)

  return map


if __name__ == '__main__':
  random.seed()
  map = generate_rooms_map(64, 64, Tileset())
  #map.print_to_file('world.dat')
  print map
