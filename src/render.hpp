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
	uint32_t tile;
};

/**
 * Window Globals
 * @g_wnd: Main window    
 */
extern HWND g_wnd;

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
