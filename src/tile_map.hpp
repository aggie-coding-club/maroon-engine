#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <stdio.h>
#include <stdint.h>

struct tile_map {
	uint8_t **rows;
	int w;
	int h;
};

extern tile_map g_tm;

void init_tm(tile_map *tm);
void size_tm(tile_map *tm, int w, int h);
void reset_tm(tile_map *tm);

#endif
