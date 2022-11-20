#include <string.h>

#include "menu.hpp"
#include "game-map.hpp"
#include "util.hpp"
#include "sprites.hpp"

uint8_t g_tile_to_spr[COUNTOF_TILES] = {
	[TILE_BLANK] = SPR_INVALID,
	[TILE_SOLID] = SPR_INVALID, 
	[TILE_GRASS] = SPR_GRASS, 
	[TILE_GROUND] = SPR_GROUND, 
	[TILE_CAPTAIN] = SPR_INVALID,
	[TILE_CRABBY] = SPR_INVALID 
};

const uint8_t g_tile_props[COUNTOF_TILES] = {
	[TILE_BLANK] = 0,
	[TILE_SOLID] = PROP_SOLID, 
	[TILE_GRASS] = PROP_SOLID, 
	[TILE_GROUND] = PROP_SOLID, 
	[TILE_CAPTAIN] = 0,
	[TILE_CRABBY] = 0 
};

const uint8_t g_em_to_tile[COUNTOF_EM] = {
	[EM_CAPTAIN] = TILE_CAPTAIN, 
	[EM_CRABBY] = TILE_CRABBY
};

const uint8_t g_idm_to_tile[] = {
	[IDM_BLANK - IDM_BLANK] = TILE_BLANK,
	[IDM_GRASS - IDM_BLANK] = TILE_GRASS,
	[IDM_GROUND - IDM_BLANK] = TILE_GROUND
};

const uint8_t g_idm_to_entity[] = {
	[IDM_PLAYER - IDM_PLAYER] = TILE_CAPTAIN,
	[IDM_CRABBY - IDM_PLAYER] = TILE_CRABBY
};
	
/*redudant table*/
uint8_t g_tile_to_em[COUNTOF_TILES];
uint8_t g_anim_to_tile[COUNTOF_ANIM];

game_map *g_gm;

static void init_tile_to_em(void)
{
	int i;

	memset(g_tile_to_em, EM_INVALID, sizeof(g_tile_to_em));

	for (i = 0; i < COUNTOF_EM; i++) {
		int tile;

		tile = g_em_to_tile[i];
		g_tile_to_em[tile] = i;
	}
}

static void init_anim_to_tile(void)
{
	int i;

	memset(g_anim_to_tile, TILE_INVALID, COUNTOF_ANIM);

	for (i = 0; i < COUNTOF_EM; i++) {
		int anim;

		anim = g_def_anims[i];
		g_anim_to_tile[anim] = g_em_to_tile[i];
	}
}

void init_tables(void)
{
	init_tile_to_em();
	init_anim_to_tile();
}

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

uint8_t get_tile(float x, float y)
{
	if (y < 0.0F) {
		return TILE_BLANK;
	}
	if (x < 0.0F || x >= g_gm->w || y >= g_gm->h) {
		return TILE_SOLID;
	}
	return g_gm->rows[(int) y][(int) x];
}
