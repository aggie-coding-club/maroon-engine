#include "sprites.hpp"

const char *const g_sprite_files[COUNTOF_SPR] = {
	[SPR_GRASS] = "grass.png",
	[SPR_GROUND] = "ground.png",
	[SPR_SKY] = "sky.png",
	[SPR_WATER] = "water.png",
	[SPR_HORIZON_WATER] = "horizon-water.png",
	[SPR_SKY_HORIZON] = "sky-horizon.png",
	[SPR_BIG_CLOUDS] = "big-clouds.png",
	[SPR_GRID] = "grid.png",

	[SPR_CAPTAIN_SWORD_IDLE_1] = "captain/idle/01.png",
	[SPR_CAPTAIN_SWORD_IDLE_2] = "captain/idle/02.png",
	[SPR_CAPTAIN_SWORD_IDLE_3] = "captain/idle/03.png",
	[SPR_CAPTAIN_SWORD_IDLE_4] = "captain/idle/04.png",
	[SPR_CAPTAIN_SWORD_IDLE_5] = "captain/idle/05.png",

	[SPR_CAPTAIN_SWORD_RUN_1] = "captain/run-sword/01.png",
	[SPR_CAPTAIN_SWORD_RUN_2] = "captain/run-sword/02.png",
	[SPR_CAPTAIN_SWORD_RUN_3] = "captain/run-sword/03.png",
	[SPR_CAPTAIN_SWORD_RUN_4] = "captain/run-sword/04.png",
	[SPR_CAPTAIN_SWORD_RUN_5] = "captain/run-sword/05.png",
	[SPR_CAPTAIN_SWORD_RUN_6] = "captain/run-sword/06.png",

	[SPR_CRABBY_IDLE_1] = "crabby/idle/01.png",
	[SPR_CRABBY_IDLE_2] = "crabby/idle/02.png",
	[SPR_CRABBY_IDLE_3] = "crabby/idle/03.png",
	[SPR_CRABBY_IDLE_4] = "crabby/idle/04.png",
	[SPR_CRABBY_IDLE_5] = "crabby/idle/05.png",
	[SPR_CRABBY_IDLE_6] = "crabby/idle/06.png",
	[SPR_CRABBY_IDLE_7] = "crabby/idle/07.png",
	[SPR_CRABBY_IDLE_8] = "crabby/idle/08.png",
	[SPR_CRABBY_IDLE_9] = "crabby/idle/09.png",

	[SPR_CRABBY_RUN_1] = "crabby/run/01.png",
	[SPR_CRABBY_RUN_2] = "crabby/run/02.png",
	[SPR_CRABBY_RUN_3] = "crabby/run/03.png",
	[SPR_CRABBY_RUN_4] = "crabby/run/04.png",
	[SPR_CRABBY_RUN_5] = "crabby/run/05.png",
	[SPR_CRABBY_RUN_6] = "crabby/run/06.png",
};
