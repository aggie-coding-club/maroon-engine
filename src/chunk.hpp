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

chunk *g_chunk_map[CHUNK_NUM_DIM][CHUNK_NUM_DIM];
chunk *g_active_chunks[2][2];

chunk g_chunks[64];
chunk *g_chunk_freed[64];
size_t g_chunk_freed_count;
size_t g_chunk_next;

#endif
