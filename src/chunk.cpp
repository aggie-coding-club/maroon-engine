#include "chunk.hpp"

chunk_map *g_chunk_map;

chunk_map *create_chunk_map(int tw, int th) 
{
	chunk_map *map;

	map = (chunk_map *) xcalloc(1, sizeof(*map));
	map->tw = tw;
	map->th = th;
	map->cw = (tw + CHUNK_LEN - 1) / CHUNK_LEN; 
	map->ch = (th + CHUNK_LEN - 1) / CHUNK_LEN; 
	return map;
}

void destroy_chunk_map(chunk_map *map)
{
	clear_chunk_map(map);
	free(map);
}

void clear_chunk_map(chunk_map *map)
{
	chunk_row *cr;
	int ny;

	cr = map->chunks;
	ny = map->ch;
	while (ny-- > 0) {
		chunk **c;
		int nx;

		c = *cr;
		nx = map->cw; 
		while (nx-- > 0) {
			destroy_chunk(map, *c);
			c++;
		}
		cr++;
	}
}

void set_chunk_map_size(chunk_map *map, int new_tw, int new_th)
{
	int old_cw;
	int old_ch;

	int new_cw;
	int new_ch;

	int ch;

	old_cw = map->cw;
	old_ch = map->ch;

	new_cw = (new_tw + CHUNK_LEN - 1) / CHUNK_LEN;
	new_ch = (new_th + CHUNK_LEN - 1) / CHUNK_LEN; 

	/*set new dimensions*/
	map->tw = new_tw;
	map->th = new_th;
	map->cw = new_cw; 
	map->ch = new_ch; 

	/*get out of range chunks*/
	ch = old_ch;
	for (ch = new_ch; ch < old_ch; ch++) {
		int cw;
		for (cw = new_cw; cw < old_cw; cw++) {
			destroy_chunk(map, map->chunks[ch][cw]);
		}
	}
}

chunk *create_chunk(int cx, int cy)
{
	chunk *c;
	c = (chunk *) xcalloc(1, sizeof(chunk));
	c->cx = cx;
	c->cy = cy;
	return c;
}

void destroy_chunk(chunk_map *map, chunk *c)
{
	if (c) {
		map->chunks[c->cy][c->cx] = NULL;
		free(c);
	}
}

chunk *touch_chunk(int tx, int ty)
{
	int cx;
	int cy;
	chunk **c;

	cx = tx / CHUNK_LEN;
	cy = ty / CHUNK_LEN;

	c = &g_chunk_map->chunks[cy][cx];
	if (!*c) {
		*c = create_chunk(cx, cy);
	}
	return *c;
}

uint8_t *touch_tile(int tx, int ty)
{
	int ix;
	int iy;
	chunk *c;

	ix = tx % CHUNK_LEN; 
	iy = ty % CHUNK_LEN; 
	c = touch_chunk(tx, ty);

	return &c->tiles[iy][ix];
}
