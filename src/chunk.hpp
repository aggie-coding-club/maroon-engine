#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <stdio.h>

#include "render.hpp"

#define CHUNK_NUM_DIM 16 
#define CHUNK_NUM

struct chunk {
	int x, y;
	uint8_t tiles[16][16];
};

#define endof(ary) (ary + _countof(ary))

chunk *g_chunk_map[CHUNK_NUM_DIM][CHUNK_NUM_DIM];
chunk *g_active_chunks[2][2];

chunk g_chunks[64];
chunk *g_chunk_freed[64];
chunk *g_chunk_freed_next;
chunk *g_chunk_next;

inline chunk *alloc_chunk(void)
{
	if (g_chunk_freed < g_chunk_freed_next) {
		return --g_chunk_freed_next;
	}

	if (g_chunk_next < endof(g_chunks)) {
		return g_chunk_next++;
	}

	abort();
}

inline void free_chunk(chunk *c)
{
	*g_chunk_freed_next++ = c;
}

inline void init_active_chunks(void)
{
	g_active_chunks[0][0] = alloc_chunk();
	g_active_chunks[0][1] = alloc_chunk();
	g_active_chunks[1][0] = alloc_chunk();
	g_active_chunks[1][1] = alloc_chunk();
}

inline chunk *get_chunk(int tx, int ty)
{
	int chunk_x;
	int chunk_y;

	chunk_x = tx / 16;
	chunk_y = ty / 16;

	return g_active_chunks[chunk_y] + chunk_x;
}

inline uint8_t *get_tile(int scr_tx, int scr_ty)
{
	int tx;
	int ty;

	int rel_x;
	int rel_y;

	chunk_x += 1;

	tx = g_scroll.x + x;
	ty = g_scroll.x + y;

	rel_x = tx % 16; 
	rel_y = ty % 16; 

	return g_active_chunks[chunk_y][chunk_x].tiles[rel_y] + rel_x;	
}

#endif
