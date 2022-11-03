#include <string.h>

#include "menu.hpp"
#include "game_map.hpp"
#include "util.hpp"

game_map *g_gm;

uint16_t g_tile_to_idm[COUNTOF_TILES] = {
	[TILE_BLANK] = IDM_BLANK,
	[TILE_SOLID] = 0, 
	[TILE_GRASS] = IDM_GRASS, 
	[TILE_GROUND] = IDM_GROUND, 
};

uint8_t g_idm_to_tile[] = {
	[IDM_BLANK - IDM_BLANK] = TILE_BLANK,
	[IDM_GRASS - IDM_BLANK] = TILE_GRASS,
	[IDM_GROUND - IDM_BLANK] = TILE_GROUND
};

game_map *create_game_map(void)
{
	game_map *gm;

	gm = (game_map *) xmalloc(sizeof(*gm));
	gm->rows = NULL;
	gm->w = 0;
	gm->h = 0;
	return gm;
}

void size_game_map(game_map *gm, int w, int h)
{
	uint8_t **rows;
	int mw, mh;
	int dw, dh;
	uint8_t **row;
	int n;

	/*clean old rows*/
	n = gm->h - h;
	row = gm->rows + h;
	while (n-- > 0) {
		free(*row++);
	}

	/*get new row pointers*/
	rows = (uint8_t **) xrealloc(gm->rows, h * sizeof(*rows));

	/*calc zero padding*/
	mw = min(w, gm->w);
	mh = min(h, gm->h);
	
	dw = w - mw;
	dh = h - mh;

	/*resize old rows*/
	row = rows;
	n = mh;
	while (n-- > 0) {
		*row = (uint8_t *) xrealloc(*row, w); 
		memset(*row + gm->w, 0, dw);
		row++;
	}

	/*resize new rows*/
	row = rows + gm->h;
	n = dh;
	while (n-- > 0) {
		*row++ = (uint8_t *) xcalloc(w, 1);
	}

	/*copy new values*/
	gm->w = w;
	gm->h = h;
	gm->rows = rows;
}

void destroy_game_map(game_map *gm)
{
	uint8_t **row;
	int n;

	row = gm->rows;
	n = gm->h;
	while (n-- > 0) {
		free(*row);
		row++;	
	}
	free(gm->rows);
	free(gm);
}

static bool in_game_map(float x, float y)
{
	return x >= 0.0F && x < g_gm->w && y >= 0.0F && y < g_gm->h;
}

uint8_t get_tile(float x, float y)
{
	if (!in_game_map(x, y)) {
		return TILE_SOLID;
	}
	return g_gm->rows[(int) y][(int) x];
}
