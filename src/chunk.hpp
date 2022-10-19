#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <stdio.h>

#include "render.hpp"
#include "util.hpp"

#define CHUNK_NUM_DIM 16 

struct chunk {
	int x, y;
	uint8_t tiles[16][16];
};

typedef chunk *chunk_map[CHUNK_NUM_DIM][CHUNK_NUM_DIM];

#define endof(ary) (ary + _countof(ary))

v2 g_cam; 
chunk_map g_chunk_map;

inline chunk *create_chunk(int x, int y)
{
	chunk *c;
	c = (chunk *) xcalloc(1, sizeof(chunk));
	c->x = x;
	c->y = y;
	return c;
}

inline void destroy_chunk(chunk *c)
{
	if (c) {
		g_chunk_map[c->y][c->x] = NULL;
		free(c);
	}
}

inline void clear_chunk_map(chunk_map map)
{
	int cy;

	for (cy = 0; cy < CHUNK_NUM_DIM; cy++) {
		int cx;

		for (cx = 0; cx < CHUNK_NUM_DIM; cx++) {
			destroy_chunk(map[cy][cx]);
		}
	}
}

inline chunk *touch_chunk(int tx, int ty)
{
	int cx;
	int cy;
	chunk **c;

	cx = tx / 16;
	cy = ty / 16;

	c = &g_chunk_map[cy][cx];
	if (!*c) {
		*c = create_chunk(cx, cy);
	}
	return *c;
}

inline uint8_t *touch_tile(int tx, int ty)
{
	int ix;
	int iy;
	chunk *c;

	ix = tx % 16; 
	iy = ty % 16; 
	c = touch_chunk(tx, ty);

	return &c->tiles[iy][ix];
}

#endif
