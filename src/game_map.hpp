#ifndef GAME_MAP_HPP 
#define GAME_MAP_HPP 

#include <stdio.h>
#include <stdint.h>

#define MAX_MAP_LEN 999 

#define TILE_BLANK 0
#define TILE_SOLID 1
#define TILE_GRASS 2 
#define TILE_GROUND 3
#define TILE_CAPTAIN 4
#define TILE_CRABBY 5
#define COUNTOF_TILES 6

#include "entity.hpp"

struct game_map {
	uint8_t **rows;
	int w;
	int h;
};

extern uint8_t g_tile_to_spr[COUNTOF_TILES];

extern game_map *g_gm;
extern uint16_t g_tile_to_idm[COUNTOF_TILES];
extern uint8_t g_idm_to_tile[];

extern uint16_t g_entity_to_idm[COUNTOF_EM];
extern uint8_t g_idm_to_entity[];

/**
 * init_game_map() - Create game map
 * Return value: new game map 
 */
game_map *create_game_map(void);

/**
 * size_game_map() - Change size of game map
 * @game_map: Tile map to change size
 * @w: New width
 * @h: New height
 */
void size_game_map(game_map *gm, int w, int h);

/**
 * destroy_game_map() - Resets game map to default state
 * @game_map: Game map to destroy 
 */
void destroy_game_map(game_map *gm);

/**
 * get_tile() - get a tile at a given coordninate
 * @x: x coordinate in tiles
 * @y: y coordinate in tiles
*/
uint8_t get_tile(float x, float y);

#endif
