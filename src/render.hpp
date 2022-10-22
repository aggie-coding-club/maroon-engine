#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>
#include <windows.h>

#include "util.hpp"

#define TILE_LEN 32 
#define MAX_SQUARES 1024 

/**
 * square - Render square 
 * @x: x-pos relative to left
 * @y: y-pos relative to top
 * @z: depth, z goes from -1 to 0, lower means on top 
 * @tile: the tile to render
 */
struct square {
	float x; 
	float y; 
	float z;
	uint32_t tile;
};

/**
 * rect - Rectangle 
 * @x: left-most pos 
 * @y: top-most pos 
 * @w: width
 * @h: height
 */
struct rect {
	float x;
	float y;
	float w;
	float h;
};

/**
 * Window Globals
 * @g_wnd: Main window    
 */
extern HWND g_wnd;

/**
 * Square Globals
 * @g_squares: Squares on screen
 * @g_square_count: Count of squares 
 * @g_grid_on: Have a grid of tile map
 * @cam: Camera rect
 */
extern square g_squares[MAX_SQUARES];
extern size_t g_square_count;
extern bool g_grid_on;
extern rect g_cam;

/**
 * init_gl() - initialize OpenGL context and load necessary extensions 
 */
void init_gl(void);

/**
 * render() - Render tile map
 */
void render(void);

#endif
