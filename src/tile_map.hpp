#ifndef TILE_MAP_HPP 
#define TILE_MAP_HPP 

#include <stdio.h>
#include <stdint.h>

#define MAX_MAP_LEN 999 

#define TILE_BLANK 0
#define TILE_SOLID 1
#define TILE_GRASS 2 
#define COUNTOF_TILES 3

struct tile_map {
	uint8_t **rows;
	int w;
	int h;
};

extern tile_map g_tm;
extern uint16_t g_tile_to_idm[COUNTOF_TILES];
extern uint8_t g_idm_to_tile[];

/**
 * init_tm() - Initialize tile map
 * @tm: Tile map to initialize 
 *
 * NOTE: If no operations are done
 * to the tile map, no memory is
 * needed to clean the tile map 
 */
void init_tm(tile_map *tm);

/**
 * size_tm() - Change size of tile map
 * @tm: Tile map to change size
 * @w: New width
 * @h: New height
 */
void size_tm(tile_map *tm, int w, int h);

/**
 * reset_tm() - Resets tile map to default state
 * @tm: Tile map to reset
 */
void reset_tm(tile_map *tm);

#endif
