#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>
#include <windows.h>

#define TILE_LEN 32 

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
 * @g_menu: Main menu
 */
extern HWND g_wnd;
extern HMENU g_menu; 

/**
 * Square Globals
 * @g_grid_on: Have a grid of tile map
 * @g_cam: Camera rect
 */
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
