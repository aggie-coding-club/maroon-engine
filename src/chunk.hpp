#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <stdio.h>

#include "render.hpp"
#include "util.hpp"

#define CHUNK_LEN 16
#define CHUNK_MAP_LEN 16 
#define MAP_LEN (CHUNK_MAP_LEN * CHUNK_LEN)

struct chunk {
	int cx;
	int cy;
	uint8_t tiles[CHUNK_LEN][CHUNK_LEN];
};

typedef chunk *chunk_row[CHUNK_LEN];

struct chunk_map {
	int tw;
	int th;
	int cw;
	int ch;
	chunk_row chunks[CHUNK_MAP_LEN];
};

#define endof(ary) (ary + _countof(ary))

chunk_map *g_chunk_map;

inline chunk *create_chunk(int x, int y)
{
	chunk *c;
	c = (chunk *) xcalloc(1, sizeof(chunk));
	c->cx = x;
	c->cy = y;
	return c;
}

inline void destroy_chunk(chunk_map *map, chunk *c)
{
	if (c) {
		map->chunks[c->cy][c->cx] = NULL;
		free(c);
	}
}

inline chunk_map *create_chunk_map(int tw, int th) 
{
	chunk_map *map;

	map = (chunk_map *) xcalloc(1, sizeof(*map));
	map->tw = tw;
	map->th = th;
	map->cw = (tw + CHUNK_LEN - 1) / CHUNK_LEN; 
	map->ch = (th + CHUNK_LEN - 1) / CHUNK_LEN; 
	return map;
}

inline void clear_chunk_map(chunk_map *map)
{
	chunk_row *cr;
	int ny;

	cr = g_chunk_map->chunks;
	ny = g_chunk_map->ch;
	while (ny-- > 0) {
		chunk **c;
		int nx;

		c = *cr;
		nx = g_chunk_map->cw; 
		while (nx-- > 0) {
			destroy_chunk(map, *c);
			c++;
		}
		cr++;
	}
}

inline void destroy_chunk_map(chunk_map *map)
{
	clear_chunk_map(map);
	free(map);
}

inline chunk *touch_chunk(int tx, int ty)
{
	int cx;
	int cy;
	chunk **c;

	cx = tx / CHUNK_LEN;
	cy = ty / CHUNK_LEN;

	c = &g_chunk_map->chunks[cy][cx];
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

	ix = tx % CHUNK_LEN; 
	iy = ty % CHUNK_LEN; 
	c = touch_chunk(tx, ty);

	return &c->tiles[iy][ix];
}

#endif
