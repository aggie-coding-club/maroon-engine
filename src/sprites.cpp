#include "sprites.hpp"

const char *const g_sprite_paths[COUNTOF_SPR] = {
	[SPR_GRASS] = "grass.png",
	[SPR_GROUND] = "ground.png",
	[SPR_SKY] = "sky.png",
	[SPR_WATER] = "water.png",
	[SPR_HORIZON_WATER] = "horizon-water.png",
	[SPR_SKY_HORIZON] = "sky-horizon.png",
	[SPR_BIG_CLOUDS] = "big-clouds.png",
	[SPR_GRID] = "grid.png"
};

const char *const g_anim_paths[COUNTOF_ANIM] = {
	[ANIM_CAPTAIN_IDLE] = "captain/idle",
	[ANIM_CAPTAIN_RUN] = "captain/run",
	[ANIM_CAPTAIN_JUMP] = "captain/jump",
	[ANIM_CAPTAIN_FALL] = "captain/fall",
	[ANIM_CRABBY_IDLE] = "crabby/idle", 
	[ANIM_CRABBY_RUN] = "crabby/run" 
};

const uint8_t g_anim_flags[COUNTOF_ANIM] = {
	[ANIM_CAPTAIN_IDLE] = AF_REPEAT,
	[ANIM_CAPTAIN_RUN] = AF_REPEAT,
	[ANIM_CAPTAIN_JUMP] = 0,
	[ANIM_CAPTAIN_FALL] = 0,
	[ANIM_CRABBY_IDLE] = AF_REPEAT, 
	[ANIM_CRABBY_RUN] = AF_REPEAT 
};

anim g_anims[COUNTOF_ANIM];

