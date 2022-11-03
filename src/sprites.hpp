#ifndef SPRITES_HPP
#define SPRITES_HPP

enum {
	SPR_GRASS,
	SPR_GROUND,
	SPR_SKY,
	SPR_WATER,
	SPR_HORIZON_WATER,
	SPR_SKY_HORIZON,
	SPR_BIG_CLOUDS,
	SPR_GRID,

	SPR_CAPTAIN_SWORD_IDLE_1,
	SPR_CAPTAIN_SWORD_IDLE_2,
	SPR_CAPTAIN_SWORD_IDLE_3,
	SPR_CAPTAIN_SWORD_IDLE_4,
	SPR_CAPTAIN_SWORD_IDLE_5,

	SPR_CAPTAIN_SWORD_RUN_1,
	SPR_CAPTAIN_SWORD_RUN_2,
	SPR_CAPTAIN_SWORD_RUN_3,
	SPR_CAPTAIN_SWORD_RUN_4,
	SPR_CAPTAIN_SWORD_RUN_5,
	SPR_CAPTAIN_SWORD_RUN_6,

	SPR_CRABBY_IDLE_1,
	SPR_CRABBY_IDLE_2,
	SPR_CRABBY_IDLE_3,
	SPR_CRABBY_IDLE_4,
	SPR_CRABBY_IDLE_5,
	SPR_CRABBY_IDLE_6,
	SPR_CRABBY_IDLE_7,
	SPR_CRABBY_IDLE_8,
	SPR_CRABBY_IDLE_9,

	SPR_CRABBY_RUN_1,
	SPR_CRABBY_RUN_2,
	SPR_CRABBY_RUN_3,
	SPR_CRABBY_RUN_4,
	SPR_CRABBY_RUN_5,
	SPR_CRABBY_RUN_6,

	COUNTOF_SPR
};

const char *const g_sprite_files[COUNTOF_SPR] = {
	[SPR_GRASS] = "grass.png",
	[SPR_GROUND] = "ground.png",
	[SPR_SKY] = "sky.png",
	[SPR_WATER] = "water.png",
	[SPR_HORIZON_WATER] = "horizon-water.png",
	[SPR_SKY_HORIZON] = "sky-horizon.png",
	[SPR_BIG_CLOUDS] = "big-clouds.png",
	[SPR_GRID] = "grid.png",

	[SPR_CAPTAIN_SWORD_IDLE_1] = "captain clown nose/with sword/09-Idle Sword/Idle Sword 01.png",
	[SPR_CAPTAIN_SWORD_IDLE_2] = "captain clown nose/with sword/09-Idle Sword/Idle Sword 02.png",
	[SPR_CAPTAIN_SWORD_IDLE_3] = "captain clown nose/with sword/09-Idle Sword/Idle Sword 03.png",
	[SPR_CAPTAIN_SWORD_IDLE_4] = "captain clown nose/with sword/09-Idle Sword/Idle Sword 04.png",
	[SPR_CAPTAIN_SWORD_IDLE_5] = "captain clown nose/with sword/09-Idle Sword/Idle Sword 05.png",

	[SPR_CAPTAIN_SWORD_RUN_1] = "captain clown nose/with sword/10-Run Sword/Run Sword 01.png",
	[SPR_CAPTAIN_SWORD_RUN_2] = "captain clown nose/with sword/10-Run Sword/Run Sword 02.png",
	[SPR_CAPTAIN_SWORD_RUN_3] = "captain clown nose/with sword/10-Run Sword/Run Sword 03.png",
	[SPR_CAPTAIN_SWORD_RUN_4] = "captain clown nose/with sword/10-Run Sword/Run Sword 04.png",
	[SPR_CAPTAIN_SWORD_RUN_5] = "captain clown nose/with sword/10-Run Sword/Run Sword 05.png",
	[SPR_CAPTAIN_SWORD_RUN_6] = "captain clown nose/with sword/10-Run Sword/Run Sword 06.png",

	[SPR_CRABBY_IDLE_1] = "crabby/01-idle/Idle 01.png",
	[SPR_CRABBY_IDLE_2] = "crabby/01-idle/Idle 02.png",
	[SPR_CRABBY_IDLE_3] = "crabby/01-idle/Idle 03.png",
	[SPR_CRABBY_IDLE_4] = "crabby/01-idle/Idle 04.png",
	[SPR_CRABBY_IDLE_5] = "crabby/01-idle/Idle 05.png",
	[SPR_CRABBY_IDLE_6] = "crabby/01-idle/Idle 06.png",
	[SPR_CRABBY_IDLE_7] = "crabby/01-idle/Idle 07.png",
	[SPR_CRABBY_IDLE_8] = "crabby/01-idle/Idle 08.png",
	[SPR_CRABBY_IDLE_9] = "crabby/01-idle/Idle 09.png",

	[SPR_CRABBY_RUN_1] = "crabby/02-Run/Run 01.png",
	[SPR_CRABBY_RUN_2] = "crabby/02-Run/Run 02.png",
	[SPR_CRABBY_RUN_3] = "crabby/02-Run/Run 03.png",
	[SPR_CRABBY_RUN_4] = "crabby/02-Run/Run 04.png",
	[SPR_CRABBY_RUN_5] = "crabby/02-Run/Run 05.png",
	[SPR_CRABBY_RUN_6] = "crabby/02-Run/Run 06.png",
};

#endif
