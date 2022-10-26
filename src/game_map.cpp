#include <string.h>

#include "menu.hpp"
#include "game_map.hpp"
#include "util.hpp"

game_map g_game_map;

uint16_t g_tile_to_idm[COUNTOF_TILES] = {
	[TILE_BLANK] = IDM_BLANK,
	[TILE_SOLID] = 0, 
	[TILE_GRASS] = IDM_GRASS, 
};

uint8_t g_idm_to_tile[] = {
	[IDM_BLANK - IDM_BLANK] = TILE_BLANK,
	[IDM_GRASS - IDM_BLANK] = TILE_GRASS
};

static int min(int a, int b)
{
	return a < b ? a : b;
}

void init_game_map(game_map *game_map)
{
	game_map->rows = NULL;
	game_map->w = 0;
	game_map->h = 0;
}

void size_game_map(game_map *game_map, int w, int h)
{
	uint8_t **rows;
	int mw, mh;
	int dw, dh;
	uint8_t **row;
	int n;

	/*clean old rows*/
	n = game_map->h - h;
	row = game_map->rows + h;
	while (n-- > 0) {
		free(*row++);
	}

	/*get new row pointers*/
	rows = (uint8_t **) xrealloc(game_map->rows, h * sizeof(*rows));

	/*calc zero padding*/
	mw = min(w, game_map->w);
	mh = min(h, game_map->h);
	
	dw = w - mw;
	dh = h - mh;

	/*resize old rows*/
	row = rows;
	n = mh;
	while (n-- > 0) {
		*row = (uint8_t *) xrealloc(*row, w); 
		memset(*row + game_map->w, 0, dw);
		row++;
	}

	/*resize new rows*/
	row = rows + game_map->h;
	n = dh;
	while (n-- > 0) {
		*row++ = (uint8_t *) xcalloc(w, 1);
	}

	/*copy new values*/
	game_map->w = w;
	game_map->h = h;
	game_map->rows = rows;
}

void reset_game_map(game_map *game_map)
{
	uint8_t **row;
	int n;

	row = game_map->rows;
	n = game_map->h;
	while (n-- > 0) {
		free(*row);
		row++;	
	}
	free(game_map->rows);

	init_game_map(game_map);
}
