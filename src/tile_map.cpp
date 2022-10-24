#include <string.h>

#include "menu.hpp"
#include "tile_map.hpp"
#include "util.hpp"

tile_map g_tm;

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

void init_tm(tile_map *tm)
{
	tm->rows = NULL;
	tm->w = 0;
	tm->h = 0;
}

void size_tm(tile_map *tm, int w, int h)
{
	uint8_t **rows;
	int mw, mh;
	int dw, dh;
	uint8_t **row;
	int n;

	/*clean old rows*/
	n = tm->h - h;
	row = tm->rows + h;
	while (n-- > 0) {
		free(*row++);
	}

	/*get new row pointers*/
	rows = (uint8_t **) xrealloc(tm->rows, h * sizeof(*rows));

	/*calc zero padding*/
	mw = min(w, tm->w);
	mh = min(h, tm->h);
	
	dw = w - mw;
	dh = h - mh;

	/*resize old rows*/
	row = rows;
	n = mh;
	while (n-- > 0) {
		*row = (uint8_t *) xrealloc(*row, w); 
		memset(*row + tm->w, 0, dw);
		row++;
	}

	/*resize new rows*/
	row = rows + tm->h;
	n = dh;
	while (n-- > 0) {
		*row++ = (uint8_t *) xcalloc(w, 1);
	}

	/*copy new values*/
	tm->w = w;
	tm->h = h;
	tm->rows = rows;
}

void reset_tm(tile_map *tm)
{
	uint8_t **row;
	int n;

	row = tm->rows;
	n = tm->h;
	while (n-- > 0) {
		free(*row);
		row++;	
	}
	free(tm->rows);

	init_tm(tm);
}
