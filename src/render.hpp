#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>
#include <windows.h>
#include "game-map.hpp"
#include "sprites.hpp"

#define TILE_LEN 32 

#define VIEW_TW 8
#define VIEW_TH 6

#define VIEW_WIDTH (VIEW_TW * TILE_LEN) 
#define VIEW_HEIGHT (VIEW_TH * TILE_LEN) 

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
extern float g_cloud_x;

/**
 * bound_coord() - Bound coordinate inside camera
 * @v: Camera value to bound
 * @gm: Dimension of game map
 * @cam: Dimension of camera
 *
 * Return: Bounded value
 */
inline float bound_coord(float v, float gm, float cam)
{
	return fclampf(v, 0.0F, fabsf(gm - cam)); 
}

/**
 * bound_cam() - Bounds camera to inside borders 
 */
inline void bound_cam(void) 
{
	g_cam.x = bound_coord(g_cam.x, g_gm->w, g_cam.w);
	g_cam.y = bound_coord(g_cam.y, g_gm->h, g_cam.h);
}

/**
 * err_wnd() - Show generic error message box
 * @parent: Parent window 
 * @err: Error message 
 */
inline void err_wnd(HWND parent, const wchar_t *err)
{
	MessageBoxW(parent, err, L"Error", MB_ICONERROR);
}

/**
 * init_gl() - initialize OpenGL context and load necessary extensions 
 */
void init_gl(void);

/**
 * render() - Render tile map
 */
void render(void);

#endif
