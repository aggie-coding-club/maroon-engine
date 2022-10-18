#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>

#define TILE_LEN 32 
#define MAX_OBJS 1024 

struct v2 {
	float x;
	float y;
};

struct object {
	_Float16 x; 
	_Float16 y; 
	uint8_t tile; 
	uint8_t flags;
};

/**
 * Window Globals
 * @g_wnd: Main window    
 */
extern HWND g_wnd;

/**
 * Tile Map Globals 
 * @g_tile_map: Tile map shown on screen
 * @g_scroll: Wrapping scroll position, measured in tiles 
 * @g_grid_on: Whether or not to render grip over tile map
 */
extern uint8_t g_tile_map[32][32];
extern v2 g_scroll;
extern bool g_grid_on;

/**
 * Object Globals
 * @g_objects: Objects on screen
 * @g_object_count: Count of objects 
 */
extern object g_objects[MAX_OBJS];
extern size_t g_object_count;

/**
 * init_gl() - initialize OpenGL context and load necessary extensions 
 */
void init_gl(void);

/**
 * render() - Render tile map
 */
void render(void);

#endif
