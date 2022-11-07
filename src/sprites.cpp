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
	[ANIM_CRABBY_IDLE] = "crabby/idle", 
	[ANIM_CRABBY_RUN] = "crabby/run" 
};

anim g_anims[COUNTOF_ANIM];
