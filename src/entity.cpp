#include <math.h>

#include "entity.hpp"
#include "game_map.hpp"
#include "render.hpp"

float g_dt;
DL_HEAD(g_entities);
int g_key_down[256];

static float g_gravity;
static entity *g_player;

static const anim g_captain_idle_anim = {
	0.1F,
	SPR_CAPTAIN_SWORD_IDLE_1,
	SPR_CAPTAIN_SWORD_IDLE_5
};

static const anim g_captain_run_anim = {
	0.1F,
	SPR_CAPTAIN_SWORD_RUN_1,
	SPR_CAPTAIN_SWORD_RUN_6
};

static const anim g_crabby_idle_anim = {
	0.1F,
	SPR_CRABBY_IDLE_1,
	SPR_CRABBY_IDLE_9
};

static const anim g_crabby_run_anim = {
	0.1F,
	SPR_CRABBY_RUN_1,
	SPR_CRABBY_RUN_6
};

const entity_meta g_entity_metas[COUNTOF_EM] = {
	[EM_PLAYER] = {
		.mask = {
			{
				20.0F / TILE_LEN, 
				0.0F
			},
			{
				52.0F / TILE_LEN,
				32.0F / TILE_LEN
			}
		},
		.def_anim = &g_captain_idle_anim
	},
	[EM_CRABBY] = {
		.mask = {
			{
				17.0F / TILE_LEN,
				6.0F / TILE_LEN,
			},{
				58.0F / TILE_LEN,
				28.0F / TILE_LEN
			}
		},
		.def_anim = &g_crabby_idle_anim
	}
};

/**
 * set_animation - Set current animation for entity
 * @e: Entity to change
 * @anim: Animation to set
 *
 * NOTE: Use change_animation to avoid animation reset in the case the 
 * new animation is the same as the old.
 */
static void set_animation(entity *e, const anim *new_anim)
{
	e->cur_anim = new_anim;
	e->anim_time = new_anim->dt;
	e->sprite = new_anim->start;
}

entity *create_entity(int tx, int ty, uint8_t meta)
{
	entity *e;
	e = (entity *) xmalloc(sizeof(*e));

	dl_push_back(&g_entities, &e->node);
	e->meta = &g_entity_metas[meta];
	e->spawn.x = tx;
	e->spawn.y = ty;

	e->pos.x = tx;
	e->pos.y = ty;
	e->vel.x = 0.0F;
	e->vel.y = 0.0F;
	e->offset = e->pos + e->meta->mask.tl;

	set_animation(e, e->meta->def_anim);
	return e;
}

void destroy_entity(entity *e)
{
	dl_del(&e->node);
	free(e);
}

void start_entities(void)
{
	g_player = create_entity(3, 3, EM_PLAYER);
	g_gravity = 2.0F;
}

/**
 * change_animation() - Change animation
 * @e: Enity to change the animation of
 * @new_anim: New animation to change to
 *
 * NOTE: Will not reset animation timer if new
 * animation is the same as the old.
 */
static void change_animation(entity *e, const anim *new_anim)
{
	if (e->cur_anim != new_anim) {
		set_animation(e, new_anim);
	}
}

/**
 * update_animation() - Update animation for entity
 * @e: Entity to update animation of
 */
static void update_animation(entity *e)
{
	e->anim_time -= g_dt;
	
	if (e->anim_time <= 0.0F) {
		if (e->sprite < e->cur_anim->end) {
			e->sprite++;
		} else {
			e->sprite = e->cur_anim->start;
		}

		e->anim_time = e->cur_anim->dt;
	}
}

/**
 * update_physics() - Update physics of entity
 * @e: Entity to update physics of
 */
static void update_physics(entity *e)
{
	e->pos.x += e->vel.x * g_dt;
	e->pos.y += e->vel.y * g_dt;
	/**
	 * updating the physics offset to reflect 
	 * the change in sprite position
	 */
	e->offset = e->pos + e->meta->mask.tl;
}

void update_entities(void)
{
	/*simple left and right player movement*/
	v2 player_vel;
	float player_speed;
	entity *e, *n;
	bool touch_below, touch_above;
	bool touch_left, touch_right;

	/*check for tiles colliding with player on all four sides*/
	touch_below = get_tile(g_player->offset.x, 
			g_player->offset.y + g_player->meta->mask.br.y) ||
			get_tile(g_player->offset.x + g_player->meta->mask.tl.x,
			g_player->offset.y + g_player->meta->mask.br.y);
	touch_above = get_tile(g_player->offset.x, 
			g_player->offset.y + g_player->meta->mask.tl.y) ||
			get_tile(g_player->offset.x + g_player->meta->mask.tl.x,
			g_player->offset.y + g_player->meta->mask.tl.y);
	touch_left = get_tile(g_player->offset.x - 
			g_player->meta->mask.tl.x / 5, g_player->offset.y + 
			g_player->meta->mask.tl.y) || get_tile(g_player->offset.x - 
			g_player->meta->mask.tl.x / 5, g_player->offset.y + 
			g_player->meta->mask.br.y * 0.75);
	touch_right = get_tile(g_player->offset.x + 
			g_player->meta->mask.tl.x * 1.25, g_player->offset.y + 
			g_player->meta->mask.tl.y) || get_tile(g_player->offset.x + 
			g_player->meta->mask.tl.x * 1.25, g_player->offset.y + 
			g_player->meta->mask.br.y * 0.75);
	
	player_vel.x = 0.0F;
	player_vel.y = 0.0F;
	player_speed = 4.0F;

	if (g_key_down['W'] && !touch_above) {
		player_vel.y = -1.0F;
	}

	if (g_key_down['S'] && !touch_below) {
		player_vel.y = 1.0F;
	}

	if (g_key_down['A'] && !touch_left) {
		player_vel.x = -1.0F;
	}
	
	if (g_key_down['D'] && !touch_right) {
		player_vel.x = 1.0F;
	}

	g_player->vel = player_vel * player_speed;
	g_player->vel.y += g_gravity;

	/**
	 * Note(Lenny) - collision detection and 
	 * resolution code should go here
	 * the current method is not ideal
	 */
	if (touch_below) {
		g_player->vel.y -= g_gravity;
	}

	/* figuring out which animiation to use*/
	if (fabsf(g_player->vel.x) > 0.05F) {
		change_animation(g_player, &g_captain_run_anim);
	} else {
		change_animation(g_player, &g_captain_idle_anim);
	}

	dl_for_each_entry_s (e, n, &g_entities, node) {

		/* checking type of entity based on meta pointer */
		if (e->meta == (g_entity_metas + EM_CRABBY)) {
			/* crabby movement */
			uint8_t tile_id_left = get_tile(e->offset.x, 
					e->offset.y + e->meta->mask.br.y + 0.1F);

			uint8_t tile_id_right = get_tile(e->offset.x + 
					(e->meta->mask.br.x - e->meta->mask.tl.x), 
					e->offset.y + e->meta->mask.br.y + 0.1F);

			if (tile_id_left == TILE_SOLID || tile_id_left != TILE_GRASS) {
				e->vel.x *= -1;
			} else if (tile_id_right == TILE_SOLID || 
					tile_id_right != TILE_GRASS) {
				e->vel.x *= -1;
			} 

			/* selecting the animation */
			if (fabsf(e->vel.x) > 0.05F) {
				change_animation(e, &g_crabby_run_anim);
			} else {
				change_animation(e, &g_crabby_idle_anim);
			}
		}

		update_animation(e);
		update_physics(e);
	}
}

void end_entities(void)
{
	clear_entities();
	g_player = NULL;
}

void clear_entities(void)
{
	entity *e, *n;

	dl_for_each_entry_s (e, n, &g_entities, node) {
		destroy_entity(e);
	}
}

