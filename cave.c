#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>

/*
 * Cave-like level generator.
 * Includes implementation of a PRNG, simplex noise and
 * the algorithm itself.
 *
 */

/*
 * Pseudo random number generator, based on xorshift1024*.
 * Code from http://xorshift.di.unimi.it/xorshift1024star.c
 *
 * This is a fast, top-quality generator. If 1024 bits of state are too
 * much, try a xorshift128+ or generator.
 *
 * The state must be seeded so that it is not everywhere zero. If you have
 * a 64-bit seed,  we suggest to seed a xorshift64* generator and use its
 * output to fill s.
 *
 */

// State.
uint64_t s[16];
int p;

// Function to generate next pseudo random number.
uint64_t xrand(void) {
	uint64_t s0 = s[p];
	uint64_t s1 = s[p = (p + 1) & 15];
	s1 ^= s1 << 31; // a
	s1 ^= s1 >> 11; // b
	s0 ^= s0 >> 30; // c
	return (s[p] = s0 ^ s1) * 1181783497276652981LL;
}

/*
 *  Initializes state from 64 bit seed.
 * Code based on http://xorshift.di.unimi.it/xorshift64star.c
 *
 */
void xsrand(uint64_t x) {
	for (uint8_t i = 0; i < 16; i++) {
		x ^= x >> 12; // a
		x ^= x << 25; // b
		x ^= x >> 27; // c
		s[i] = x * 2685821657736338717LL;
	}
}

// Seed PRNG from current time.
void timexsrand(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	xsrand((uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000);
}

// Random double.
double drand(void) {
	return (double) xrand() / 18446744073709551616.0;
}

// Random integer in range [n, m).
int64_t randrange(int64_t n, int64_t m) {
	return (int64_t)(drand() * (m - n)) + n;
}

/*
 * Simplex noise and related functions.
 * Code based on http://staffwww.itn.liu.se/~stegu/aqsis/aqsis-newnoise/sdnoise1234.c
 *
 */

/*
 * Permutation table. This is just a random jumble of all numbers 0-255,
 * repeated twice to avoid wrapping the index at 255 for each lookup.
 */
unsigned char perm[512] = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53,
		194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
		190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
		35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171,
		168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83,
		111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40,
		244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187,
		208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109,
		198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118,
		126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28,
		42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153,
		101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113,
		224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238,
		210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50,
		45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243,
		141, 128, 195, 78, 66, 215, 61, 156, 180, 151, 160, 137, 91, 90, 15,
		131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
		8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197,
		62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56,
		87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27,
		166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105,
		92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80,
		73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188,
		159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124,
		123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47,
		16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44,
		154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19,
		98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145,
		235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84,
		204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222,
		114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };

/*
 * Gradient tables. These could be programmed the Ken Perlin way with
 * some clever bit-twiddling, but this is more clear, and not really slower.
 */
static float grad2lut[8][2] = { { -1.0f, -1.0f }, { 1.0f, 0.0f },
		{ -1.0f, 0.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f }, { 0.0f, -1.0f }, {
				0.0f, 1.0f }, { 1.0f, -1.0f } };

/*
 * Helper function to compute gradient
 * and gradients-dot-residualvectors in 2D.
 */

void grad2(int hash, float *gx, float *gy) {
	int h = hash & 7;
	*gx = grad2lut[h][0];
	*gy = grad2lut[h][1];
	return;
}

/* Skewing factors for 2D simplex grid:
 * F2 = 0.5*(sqrt(3.0)-1.0)
 * G2 = (3.0-sqrt(3.0))/6.0
 */
#define F2 0.366025403f
#define G2 0.211324865f

#define FASTFLOOR(x) ( ((x)>0) ? ((int)x) : (((int)x)-1) )

// 2D simplex noise.
float sdnoise2(float x, float y) {
	float n0, n1, n2; /* Noise contributions from the three simplex corners */
	float gx0, gy0, gx1, gy1, gx2, gy2; /* Gradients at simplex corners */

	/* Skew the input space to determine which simplex cell we're in */
	float s = (x + y) * F2; /* Hairy factor for 2D */
	float xs = x + s;
	float ys = y + s;
	int i = FASTFLOOR(xs);
	int j = FASTFLOOR(ys);

	float t = (float) (i + j) * G2;
	float X0 = i - t; /* Unskew the cell origin back to (x,y) space */
	float Y0 = j - t;
	float x0 = x - X0; /* The x,y distances from the cell origin */
	float y0 = y - Y0;

	/* For the 2D case, the simplex shape is an equilateral triangle.
	 * Determine which simplex we are in. */
	int i1, j1; /* Offsets for second (middle) corner of simplex in (i,j) coords */
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} /* lower triangle, XY order: (0,0)->(1,0)->(1,1) */
	else {
		i1 = 0;
		j1 = 1;
	} /* upper triangle, YX order: (0,0)->(0,1)->(1,1) */

	/* A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
	 * a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
	 * c = (3-sqrt(3))/6   */
	float x1 = x0 - i1 + G2; /* Offsets for middle corner in (x,y) unskewed coords */
	float y1 = y0 - j1 + G2;
	float x2 = x0 - 1.0f + 2.0f * G2; /* Offsets for last corner in (x,y) unskewed coords */
	float y2 = y0 - 1.0f + 2.0f * G2;

	/* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
	int ii = i & 0xff;
	int jj = j & 0xff;

	/* Calculate the contribution from the three corners */
	float t0 = 0.5f - x0 * x0 - y0 * y0;
	float t20, t40;
	if (t0 < 0.0f)
		t40 = t20 = t0 = n0 = gx0 = gy0 = 0.0f; /* No influence */
	else {
		grad2(perm[ii + perm[jj]], &gx0, &gy0);
		t20 = t0 * t0;
		t40 = t20 * t20;
		n0 = t40 * (gx0 * x0 + gy0 * y0);
	}

	float t1 = 0.5f - x1 * x1 - y1 * y1;
	float t21, t41;
	if (t1 < 0.0f)
		t21 = t41 = t1 = n1 = gx1 = gy1 = 0.0f; /* No influence */
	else {
		grad2(perm[ii + i1 + perm[jj + j1]], &gx1, &gy1);
		t21 = t1 * t1;
		t41 = t21 * t21;
		n1 = t41 * (gx1 * x1 + gy1 * y1);
	}

	float t2 = 0.5f - x2 * x2 - y2 * y2;
	float t22, t42;
	if (t2 < 0.0f)
		t42 = t22 = t2 = n2 = gx2 = gy2 = 0.0f; /* No influence */
	else {
		grad2(perm[ii + 1 + perm[jj + 1]], &gx2, &gy2);
		t22 = t2 * t2;
		t42 = t22 * t22;
		n2 = t42 * (gx2 * x2 + gy2 * y2);
	}

	/* Add contributions from each corner to get the final noise value.
	 * The result is scaled to return values in the interval [-1,1]. */
	float noise = 40.0f * (n0 + n1 + n2);

	return noise;
}

/*
 * Generates fractal noise by adding layers of simplex noise
 * with different amplitude and frequency.
 *
 * More about this algorithm: http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
 *
 */
float octave_noise_2d(float x, float y, uint8_t octaves, float persistence,
		float lacunarity) {

	float result = 0;
	float freq = 1;
	float amp = 1;

	for (uint8_t i = 0; i < octaves; i++) {
		result += sdnoise2(x * freq, y * freq) * amp;

		freq *= lacunarity;
		amp *= persistence;
	}

	return result;
}

// Renders level in PBM format: https://en.wikipedia.org/wiki/Netpbm_format
void drawLevel(uint32_t width, uint32_t height, uint8_t **level) {
	printf("P1\n%d %d\n", height, width);

	for (uint32_t i = 0; i < width; i++)
		for (uint32_t j = 0; j < height; j++)
			printf("%d ", level[i][j] == 0);

	return;
}

/*
 * Creates sparse maze that will be used as basis of system of caves.
 * From www.astrolog.org/labyrnth/algrithm.htm :
 *
 * "Sparse Mazes are produced by choosing to not grow the Maze in areas that
 * would violate the rule of sparseness. A consistent way to implement this
 * is to, whenever considering a new cell to carve into, to first check all
 * cells within a semicircle of chosen cell radius located forward in the
 * current direction. If any of those cells is already part of the Maze,
 * don't allow the cell being considered, since doing to would be too close
 * to an existing cell and hence make the Maze not sparse."
 *
 * My modification doesn't bother itself with counting neighbors in semicircle,
 * instead it counts each neighbor in a big rectangle.
 *
 * Maze algorithm inspired by: http://weblog.jamisbuck.org/2011/1/27/maze-generation-growing-tree-algorithm
 *
 */
uint32_t makeSkeleton(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
		uint8_t **level, uint32_t ox1, uint32_t oy1, uint32_t ox2,
		uint32_t oy2) {

	uint32_t **stack;
	uint32_t lastcell = 1;
	uint32_t index;
	uint32_t allocated = width + height;

	uint32_t size = 0;

	// Direct neighbors of a tile.
	const int8_t neigh[4][2] = { { 4, 0 }, { -4, 0 }, { 0, 4 }, { 0, -4 } };

	stack = malloc(2 * sizeof(uint32_t *));

	stack[0] = malloc((width + height) * sizeof(uint32_t));
	stack[1] = malloc((width + height) * sizeof(uint32_t));

	stack[0][lastcell] = x;
	stack[1][lastcell] = y;

	while (lastcell) {
		index = lastcell;

		x = stack[0][index];
		y = stack[1][index];

		// Randomize order in which neighbors are checked.
		uint8_t order[4] = { 0, 1, 2, 3 };

		for (uint8_t i = 0; i < 4; i++) {
			uint8_t j = randrange(i, 4);
			int32_t t = order[j];
			order[j] = order[i];
			order[i] = t;
		}

		for (uint8_t i = 0; i < 4; i++) {
			int32_t nx = neigh[order[i]][0] + x;
			int32_t ny = neigh[order[i]][1] + y;

			uint32_t ncount = 0;

			// Check for boundaries.
			if (nx < ox1 || ny < oy1)
				continue;
			if (nx >= ox2 || ny >= oy2)
				continue;
			if (level[nx][ny] == 0)
				continue;

			// Count cells in current rectangle.
			// Size of rectangle can be changed to produce different results.
			for (int32_t n = -30; n <= 30; n++)
				for (int32_t f = -30; f <= 30; f++) {
					if (nx + f < ox1 || ny + n < oy1)
						continue;
					if (nx + f >= ox2 || ny + n >= oy2)
						continue;

					ncount += !level[nx + f][ny + n];

					// 60 is a "magic" constant that seems to work well enough.
					if (ncount > 60) // Too many neighbors in this rectangle.
						goto retry;
				}

			stack[0][lastcell + 1] = nx;
			stack[1][lastcell + 1] = ny;

			// Draw maze.
			// Only walls are intentionally not connected, to speed up later stages
			// of cave gerenation.
			level[nx][ny] = 0;
			level[x + neigh[order[i]][0] / 2][y + neigh[order[i]][1] / 2] = 0;
			lastcell += 2;

			// Allocate more space for the stack.
			if (lastcell > allocated) {
				allocated *= allocated;

				stack[0] = realloc(stack[0], allocated * sizeof(uint32_t*));
				stack[1] = realloc(stack[1], allocated * sizeof(uint32_t*));
			}

			// Size of maze has to be recorded, so mazes that are too small can be
			// avoided.
			size++;

			break;
			retry: continue;
		}

		lastcell--;
	}

	free(stack[0]);
	free(stack[1]);
	free(stack);

	return size;
}

// Makes random irregular stains at every '1' tile.
void makeCave(uint32_t width, uint32_t height, uint8_t **level) {
	double **cache; // Used for caching noise values, to speed up calculation.

	// Random offset for noise function.
	// Ensures uniqueness of level.
	double offset_x = 65536 * drand();
	double offset_y = 65536 * drand();

	cache = malloc(width * sizeof(double*));

	for (uint32_t i = 0; i < width; i++) {
		cache[i] = malloc(height * sizeof(double));
		for (uint32_t j = 0; j < height; j++)
			cache[i][j] = 64; // 64 is just a random number
							  // that is never produced by
	}						  // the noise function.

	for (uint32_t i = 20; i < width - 20; i++)
		for (uint32_t j = 20; j < height - 20; j++) {
			if (level[i][j] != 0)
				continue;

			for (int32_t di = -20; di <= 20; di++)
				for (int32_t dj = -20; dj <= 20; dj++) {
					if (level[i + di][j + dj] != 1)
						continue;

					double c = cache[i + di][j + dj];

					if (c == 64) {
						// Generate noise.
						// Choosing different parameters will change layout of the cave.
						c = octave_noise_2d((i + di) * 0.01 + offset_x,
								(j + dj) * 0.01 + offset_y, 6, 0.5, 2);
						cache[i + di][j + dj] = c;
					}

					// Check if current point is inside the "stain".
					// It is based on circle equation but with
					// radius of "circle" deformed by noise function.

					// Changing it will also change layout of the level.
					if (dj * dj + di * di < 1000 * c * c + 5)
						level[i + di][j + dj] = 2;

				}
		}

	for (uint32_t i = 0; i < width; i++) {
		free(cache[i]);
	}

	free(cache);

}

// Uses scanline floodfill to remove disconnected regions.
// Code based on http://lodev.org/cgtutor/floodfill.html#Scanline_Floodfill_Algorithm_With_Stack
void removeDisconnected(int32_t x, int32_t y, uint32_t width, uint32_t height,
		uint8_t **level) {
	uint32_t **stack;
	uint32_t lastitem = 1;
	uint32_t allocated = width + height;
	int32_t y1;

	uint8_t right, left;

	stack = malloc(2 * sizeof(uint32_t *));

	stack[0] = malloc((width + height) * sizeof(uint32_t));
	stack[1] = malloc((width + height) * sizeof(uint32_t));

	stack[0][lastitem] = x;
	stack[1][lastitem] = y;

	while (lastitem) {
		x = stack[0][lastitem];
		y = stack[1][lastitem];

		lastitem--;

		left = right = 0;

		for (y1 = y; y1 >= 0 && level[x][y1] == 2; y1--)
			;

		y1++;

		while (y1 < height && level[x][y1] == 2) {
			level[x][y1] = 0;

			if (!left && x > 0 && level[x - 1][y1] == 2) {
				lastitem++;
				stack[0][lastitem] = x - 1;
				stack[1][lastitem] = y1;
				left = 1;

				if (lastitem >= allocated) {
					allocated *= allocated;

					stack[0] = realloc(stack[0], allocated * sizeof(uint32_t*));
					stack[1] = realloc(stack[1], allocated * sizeof(uint32_t*));
				}

			} else if (left && x > 0 && level[x - 1][y1] != 2)
				left = 0;

			if (!right && x < width - 1 && level[x + 1][y1] == 2) {
				lastitem++;
				stack[0][lastitem] = x + 1;
				stack[1][lastitem] = y1;
				right = 1;

				if (lastitem >= allocated) {
					allocated *= allocated;

					stack[0] = realloc(stack[0], allocated * sizeof(uint32_t*));
					stack[1] = realloc(stack[1], allocated * sizeof(uint32_t*));
				}

			} else if (right && x < width - 1 && level[x + 1][y1] != 2)
				right = 0;

			y1++;
		}
	}

	free(stack[0]);
	free(stack[1]);

	free(stack);
}

int main(void) {
	const uint32_t level_width = 300, level_height = 300;
	uint32_t x, y, size;
	uint8_t **level;

	timexsrand();

	level = malloc(level_width * sizeof(uint8_t*));

	for (uint32_t i = 0; i < level_width; i++)
		level[i] = malloc(level_height * sizeof(uint8_t));

	do { // Create random mazes until one is big enough.
		x = randrange(50, level_width - 50);
		y = randrange(50, level_height - 50);

		for (uint32_t i = 0; i < level_width; i++)
			for (uint32_t j = 0; j < level_height; j++)
				level[i][j] = 1;

		// Generate sparse maze, to be "skeleton" of the cave.
		size = makeSkeleton(x, y, level_width, level_height, level, 30, 30,
				level_width - 30, level_height - 30);

		// The condition bellow is purely empirical and probably
		// can be much improved.
	} while ((double) level_width * level_height / size
			> 100 + 5e6 / (level_width * level_height));

	// Generate cave layout.
	makeCave(level_width, level_height, level);
	removeDisconnected(x, y, level_width, level_height, level);

	drawLevel(level_width, level_height, level);

	for (uint32_t i = 0; i < level_width; i++) {
		free(level[i]);
	}

	free(level);

	return 0;
}