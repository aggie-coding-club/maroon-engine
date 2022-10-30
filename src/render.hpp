#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>
#include <windows.h>

#define TILE_LEN 32 

#define VIEW_TW 8
#define VIEW_TH 6

#define VIEW_WIDTH (VIEW_TW * TILE_LEN) 
#define VIEW_HEIGHT (VIEW_TH * TILE_LEN) 

enum sprite_ids {
	SPR_GRASS,
	SPR_GROUND,
	SPR_SKY,
	SPR_WATER,
	SPR_HORIZON_WATER,
	SPR_SKY_HORIZON,
	SPR_BIG_CLOUD_0_314,
	SPR_BIG_CLOUD_1_327,
	SPR_BIG_CLOUD_1_95,
	SPR_BIG_CLOUD_2_160,
	SPR_BIG_CLOUD_2_279,
	SPR_BIG_CLOUD_2_375,
	SPR_BIG_CLOUD_2_64,
	SPR_BIG_CLOUD_3_160,
	SPR_BIG_CLOUD_3_384,
	SPR_BIG_CLOUD_1_263,
	SPR_BIG_CLOUD_1_359,
	SPR_BIG_CLOUD_2_0,
	SPR_BIG_CLOUD_2_215,
	SPR_BIG_CLOUD_2_32,
	SPR_BIG_CLOUD_2_407,
	SPR_BIG_CLOUD_2_96,
	SPR_BIG_CLOUD_3_192,
	SPR_BIG_CLOUD_3_416,
	SPR_BIG_CLOUD_1_295,
	SPR_BIG_CLOUD_1_63,
	SPR_BIG_CLOUD_2_128,
	SPR_BIG_CLOUD_2_247,
	SPR_BIG_CLOUD_2_343,
	SPR_BIG_CLOUD_2_439,
	SPR_BIG_CLOUD_3_128,
	SPR_BIG_CLOUD_3_224,
	SPR_GRID,
};
	
#define COUNTOF_SPR 33

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
