import random
from struct import pack

def genRandomWorld(datafile, size, numTiles):
    random.seed()
    world = open(datafile, 'wb')
    for i in range(size):
        world.write(pack('B', random.randint(0, numTiles - 1)))
    world.close()

if __name__ == '__main__':
    # generate a world of 64x64 scenes, each scene containing 16x16 squares
    genRandomWorld('world.dat', 1<<20, 5)
