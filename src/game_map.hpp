#ifndef GAME_MAP_HPP 
#define GAME_MAP_HPP 

#include <stdio.h>
#include <stdint.h>

#define MAX_MAP_LEN 999 

#define TILE_BLANK 0
#define TILE_SOLID 1
#define TILE_GRASS 2 
#define TILE_GROUND 3
#define COUNTOF_TILES 4

struct game_map {
	uint8_t **rows;
	int w;
	int h;
};

extern game_map g_game_map;
extern uint16_t g_tile_to_idm[COUNTOF_TILES];
extern uint8_t g_idm_to_tile[];

/**
 * init_game_map() - Initialize game map
 * @game_map: Tile map to initialize 
 *
 * NOTE: If no operations are done
 * to the game map, no memory is
 * needed to clean the game map 
 */
void init_game_map(game_map *game_map);

/**
 * size_game_map() - Change size of game map
 * @game_map: Tile map to change size
 * @w: New width
 * @h: New height
 */
void size_game_map(game_map *game_map, int w, int h);

/**
 * reset_game_map() - Resets game map to default state
 * @game_map: Tile map to reset
 */
void reset_game_map(game_map *game_map);

#endif
