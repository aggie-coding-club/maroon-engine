#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <stdio.h>

#include "render.hpp"
#include "util.hpp"

#define CHUNK_LEN 32 
#define CHUNK_MAP_LEN 8 
#define MAP_LEN (CHUNK_MAP_LEN * CHUNK_LEN)

struct chunk {
	int cx;
	int cy;
	uint8_t tiles[CHUNK_LEN][CHUNK_LEN];
};

typedef chunk *chunk_row[CHUNK_MAP_LEN];

struct chunk_map {
	int tw;
	int th;
	int cw;
	int ch;
	chunk_row chunks[CHUNK_MAP_LEN];
};

extern chunk_map *g_chunk_map;

/**
 * create_chunk_map() - Create new chunk map 
 * @tw: Initial tile width of chunk map
 * @th: Initial tile height of chunk map
 *
 * Return: New chunk map
 */
chunk_map *create_chunk_map(int tw, int th);

/**
 * destroy_chunk_map() - Destroy chunk map
 * @map: Map to destroy
 */
void destroy_chunk_map(chunk_map *map);

/**
 * clear_chunk_map() - Clear all chunks in chunk map
 * @map: Map to clear chunks from 
 */
void clear_chunk_map(chunk_map *map);

/**
 * set_chunk_map_size() - Set size of chunk map
 * @map: Map to set size of
 * @new_tw: New tile width
 * @new_th: New tile height
 */
void set_chunk_map_size(chunk_map *map, int new_tw, int new_th);

/**
 * create_chunk() - Create chunk
 * @cx: The chunk x
 * @cy: The chunk y
 *
 * NOTE: Chunk is not associated with a chunk map
 *
 * Return: Chunk pointer
 */
chunk *create_chunk(int cx, int cy);

/**
 * destroy_chunk() - Destroy chunk
 * @map: Map the chunk is part of
 * @c: Chunk
 */
void destroy_chunk(chunk_map *map, chunk *c);

/**
 * touch_chunk() - Returns (a possibly new) chunk at tile position
 * @tx: Tile x position
 * @ty: Tile y position
 *
 * NOTE: Operates on global chunk map 
 *
 * Return: Chunk at the position or a new chunk
 */
chunk *touch_chunk(int tx, int ty);

/**
 * touch_tile() - Returns (a possibly new) tile at position 
 * @tx: Tile x position
 * @ty: Tile y position
 *
 * NOTE: Operates on global chunk map 
 *
 * Return: Tile at the position or a tile in a new chunk
 */
uint8_t *touch_tile(int tx, int ty);

#endif
