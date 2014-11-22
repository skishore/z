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
    self.blocks = set()

  def add_room(self, room):
    for w in xrange(room.width):
      for h in xrange(room.height):
        self.tiles[room.x + w][room.y + h] = self.tileset.get_free_tile()
    self.rooms.append(room)
    self.add_block((room.x - 1, room.y - 1))
    self.add_block((room.x - 1, room.y + room.height))
    self.add_block((room.x + room.width, room.y - 1))
    self.add_block((room.x + room.width, room.y + room.height))

  def add_block(self, block):
    if not (0 <= block[0] < self.width and 0 <= block[1] < self.height):
      return
    self.blocks.add(block)
    self.tiles[block[0]][block[1]] = 4

  def dig_corridor(self, index1, index2):
    source = self.rooms[index1].random_square()
    target = self.rooms[index2].random_square()
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
        if not (1 <= child[0] < self.width - 1 and
                1 <= child[1] < self.height - 1):
          continue
        if child in visited or child in self.blocks:
          continue
        tile = self.tiles[child[0]][child[1]]
        step_length = (1.0 if self.tileset.blocked(tile) else 4.0)
        distance = best_distance + (1 + random.random())*step_length
        if distance < frontier.get(child, float('Infinity')):
          frontier[child] = distance
          parents[child] = best_node
    node = target
    while node != None:
      if self.tileset.blocked(self.tiles[node[0]][node[1]]):
        self.tiles[node[0]][node[1]] = self.tileset.get_free_tile()
      node = parents[node]

  def add_walls(self):
    for w in xrange(self.width):
      for h in xrange(self.height):
        if not self.tileset.blocked(self.tiles[w][h]):
          for dw in (-1, 0, 1):
            for dh in (-1, 0, 1):
              if (0 <= w + dw < self.width and 0 <= h + dh < self.height and
                  self.tiles[w + dw][h + dh] == self.tileset.default_tile):
                self.tiles[w + dw][h + dh] = 4

  def get_header(self):
    lines = []
    lines.append('width: %d' % (self.width,))
    lines.append('height: %d' % (self.height,))
    lines.append('default_tile: %d' % (self.tileset.default_tile,))
    starting_square = (0, 0)
    if self.rooms:
      starting_square = self.rooms[0].random_square()
    lines.append('starting_square: %d %d' % starting_square)
    return '\n'.join(lines)

  def __str__(self):
    room_labels = string.digits + string.letters
    result = [self.get_header()]
    tiles = [
      [self.tileset.chars[tile] for tile in column]
      for column in self.tiles
    ]
    #for (i, room) in enumerate(self.rooms):
    #  for w in xrange(room.width):
    #    for h in xrange(room.height):
    #      tiles[room.x + w][room.y + h] = room_labels[i % len(room_labels)]
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

  def random_square(self):
    return (
      self.x + random.randint(0, self.width - 1),
      self.y + random.randint(0, self.height - 1),
    )

  def place(self, map, tolerance):
    self.x = random.randint(1, map.width - self.width - 1)
    self.y = random.randint(1, map.height - self.height - 1)
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
  (min_size, max_size) = (8, 16)
  tries = width*height/(min_size**2)
  tries_left = tries

  print 'Placing rooms!'
  while tries_left > 0:
    width = random.randint(min_size, max_size)
    height = random.randint(min_size, max_size)
    room = Room(width, height)
    if room.place(map, tolerance=min_size):
      map.add_room(room)
    else:
      tries_left -= 1
  print 'Placed %d rooms and failed %d times.' % (len(map.rooms), tries)

  print 'Computing minimal spanning tree!'
  room_graph = []
  for room in map.rooms:
    room_graph.append([])
    for other in map.rooms:
      room_graph[-1].append(room.distance(other))
  spanning_tree = csgraph.minimum_spanning_tree(room_graph)
  print 'Digging corridors!'
  for (index1, index2) in zip(*spanning_tree.nonzero()):
    map.dig_corridor(index1, index2)

  print 'Adding extra edges!'
  tree_distances = csgraph.dijkstra(spanning_tree, directed=False)
  extra_edges_added = 0
  while True:
    edge_ratios = []
    for i in xrange(len(map.rooms)):
      for j in xrange(len(map.rooms)):
        if i == j:
          continue
        edge_ratio = tree_distances[i,j]/room_graph[i][j]
        if edge_ratio <= 1:
          continue
        edge_ratios.append((edge_ratio, (i, j)))
    (edge_ratio, edge) = max(edge_ratios)
    if edge_ratio < 3:
      break
    map.dig_corridor(edge[0], edge[1])
    # Update the tree distances based on the existence of the new edge.
    for i in xrange(len(map.rooms)):
      for j in xrange(len(map.rooms)):
        tree_distances[i,j] = min(
          tree_distances[i,j],
          tree_distances[i,edge[0]] + room_graph[edge[0]][edge[1]] + tree_distances[edge[1],j],
          tree_distances[i,edge[1]] + room_graph[edge[1]][edge[0]] + tree_distances[edge[0],j],
        )
    extra_edges_added += 1
  print 'Added %d extra edges.' % (extra_edges_added,)

  print 'Adding walls!'
  map.add_walls()

  return map


if __name__ == '__main__':
  random.seed()
  map = generate_rooms_map(256, 256, Tileset())
  #print map
  map.print_to_file('world.dat')
