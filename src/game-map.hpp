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

#define TILE_INVALID 255

#include "entity.hpp"

#define PROP_SOLID 1

struct game_map {
	uint8_t **rows;
	int w;
	int h;
};

extern uint8_t g_tile_to_spr[COUNTOF_TILES];
extern const uint8_t g_tile_props[COUNTOF_TILES];

extern const uint8_t g_em_to_tile[COUNTOF_EM];
extern uint8_t g_tile_to_em[COUNTOF_TILES];
extern uint8_t g_anim_to_tile[COUNTOF_ANIM];

extern const uint8_t g_idm_to_tile[];
extern const uint8_t g_idm_to_entity[];

extern game_map *g_gm;

/**
 * init_tables() - Initalizes tables
 *
 * Initalize sparse/redudant tables.
 */
void init_tables(void);

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
