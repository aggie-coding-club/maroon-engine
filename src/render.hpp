#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>
#include <windows.h>

#define TILE_LEN 32 

#define VIEW_TW 8
#define VIEW_TH 6

#define VIEW_WIDTH (VIEW_TW * TILE_LEN) 
#define VIEW_HEIGHT (VIEW_TH * TILE_LEN) 

#define SPR_GRASS 0
#define SPR_GROUND 1
#define SPR_SKY 2
#define SPR_WATER 3
#define SPR_HORIZON_WATER 4
#define SPR_SKY_HORIZON 5
#define SPR_BIG_CLOUD_0_9750 6
#define SPR_BIG_CLOUD_1_2875 7
#define SPR_BIG_CLOUD_1_3875 8
#define SPR_BIG_CLOUD_1_8750 9
#define SPR_GRID 10 
#define COUNTOF_SPR 11 

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
 * Sprite Globals
 * @g_grid_on: Have a grid of tile map
 * @g_cam: Camera rect
 */
extern bool g_grid_on;
extern rect g_cam;
extern bool g_running;

/**
 * init_gl() - initialize OpenGL context and load necessary extensions 
 */
void init_gl(void);

/**
 * render() - Render tile map
 */
void render(void);

#endif
