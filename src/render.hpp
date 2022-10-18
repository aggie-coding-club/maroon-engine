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

extern HWND g_wnd;

extern uint8_t g_tile_map[32][32];
extern v2 g_scroll;
extern bool g_grid_on;

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
