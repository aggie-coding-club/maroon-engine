#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>
#include <windows.h>

#include "util.hpp"

#define TILE_LEN 32 
#define MAX_OBJS 1024 

struct square {
	float x; 
	float y; 
	float z;
	int tile;
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
 * Square Globals
 * @g_squares: Squares on screen
 * @g_square_count: Count of squares 
 */
extern square g_squares[MAX_OBJS];
extern size_t g_square_count;

/**
 * init_gl() - initialize OpenGL context and load necessary extensions 
 */
void init_gl(void);

/**
 * render() - Render tile map
 */
void render(void);

#endif
