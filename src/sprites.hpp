#ifndef SPRITES_HPP
#define SPRITES_HPP

#include <stdint.h>

#define SPR_GRASS 0
#define SPR_GROUND 1
#define SPR_SKY 2
#define SPR_WATER 3
#define SPR_HORIZON_WATER 4
#define SPR_SKY_HORIZON 5
#define SPR_BIG_CLOUDS 6
#define SPR_GRID 7

/**
 * Indicates all predefined sprites
 */
#define COUNTOF_SPR 8

/**
 * Indicates the maximum number of sprites
 */
#define COUNTOF_SPR_ALL 64 

/**
 * Used like NULL is for pointers in g_tile_to_spr.
 */
#define SPR_INVALID 0xFF

#define ANIM_CAPTAIN_IDLE 0
#define ANIM_CAPTAIN_RUN 1
#define ANIM_CRABBY_IDLE 2
#define ANIM_CRABBY_RUN 3
#define COUNTOF_ANIM 4

#define ANIM_DT 0.1F

/**
 * struct anim - An animation 
 * @start: First sprite in animation 
 * @end: Last sprite in animation 
 * 
 * Animation iterates through each sprite.
 * The sprites in animation will be consecutive.
 */
struct anim {
	uint8_t start;
	uint8_t end;
};

extern const char *const g_sprite_paths[COUNTOF_SPR];
extern const char *const g_anim_paths[COUNTOF_ANIM];
extern anim g_anims[COUNTOF_ANIM]; 

#endif
