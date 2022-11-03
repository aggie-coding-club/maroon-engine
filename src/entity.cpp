#include "entity.hpp"
#include <stdio.h>
#include "sprites_files_list.hpp"
#include "game_map.hpp"
#include "render.hpp"

float g_dt;
DL_HEAD(g_entities);

static float g_gravity;
static entity *g_player;

static const anim g_captain_idle_anim = {
	0.2F,
	SPR_CAPTAIN_SWORD_IDLE_1,
	SPR_CAPTAIN_SWORD_IDLE_5
};

static const anim g_captain_run_anim = {
	0.1F,
	SPR_CAPTAIN_SWORD_RUN_1,
	SPR_CAPTAIN_SWORD_RUN_6
};

entity_meta g_entity_metas[COUNTOF_EM] = {
	[EM_PLAYER] = {
		.sprite = SPR_CAPTAIN_SWORD_RUN_1,
		.mask = {
			{
				20.0F / TILE_LEN, 
				0.0F
			},
			{
				52.0F / TILE_LEN,
				30.0F / TILE_LEN
			}
		}
	}
};

entity *create_entity(int tx, int ty)
{
	entity *e;
	e = (entity *) xmalloc(sizeof(*e));

	dl_push_back(&g_entities, &e->node);
	e->meta = &g_entity_metas[EM_PLAYER];
	e->spawn.x = tx;
	e->spawn.y = ty;
	e->pos.x = tx;
	e->pos.y = ty;
	e->vel.x = 0.0F;
	e->vel.y = 0.0F;
	return e;
}

void destroy_entity(entity *e)
{
	dl_del(&e->node);
	free(e);
}

void start_entities(void)
{
	g_player = create_entity(3, 3);

	g_player->active = true;
	g_player->cur_anim = &g_captain_idle_anim;
	g_gravity = 2.0F;
}

void update_entities(void)
{
	/*simple left and right player movement*/
	v2 player_vel;
	float player_speed;
	entity *e, *n;
	
	player_vel.x = 0.0F;
	player_vel.y = 0.0F;
	player_speed = 4.0F;

	if (g_key_down[KEY_W] || g_key_down[KEY_UP]) {
		player_vel.y = -1.0F;
	}

	if (g_key_down[KEY_S] || g_key_down[KEY_DOWN]) {
		player_vel.y = 1.0F;
	}

	if (g_key_down[KEY_D] || g_key_down[KEY_LEFT]) {
		player_vel.x = -1.0F;
	}
	
	if (g_key_down[KEY_A] || g_key_down[KEY_RIGHT]) {
		player_vel.x = 1.0F;
	}

	g_player->vel = player_vel * player_speed;
	g_player->vel.y += g_gravity;

	/**
	 * Note(Lenny) - collision detection and resolution code should go here
	 * the current method is not ideal
	 */
	if (get_tile(g_player->offset.x, 
		g_player->offset.y + g_player->meta->mask.br.y)) {

		/* the position of the collided tile */
		/*TODO: Casting to int does not work with negatives!!
		 */
		v2 collided_tile_pos = {
			(float) (int) g_player->meta->mask.tl.x, 
			(float) (int) g_player->meta->mask.br.y
		};

		g_player->vel.y -= g_gravity;
	}

	/* figuring out which animiation to use*/
	bool anim_changed = false;
	if (g_player->vel.x > 0.05F || g_player->vel.x < -0.05F) {
		if (g_player->cur_anim != &g_captain_run_anim) {
			anim_changed = true;
		}
		g_player->cur_anim = &g_captain_run_anim;
	} else {
		if (g_player->cur_anim != &g_captain_idle_anim) {
			anim_changed = true;
		}
		g_player->cur_anim = &g_captain_idle_anim;
	}

	dl_for_each_entry_s (e, n, &g_entities, node) {
		if (e->active) {
			/**
			 * change anim state based on 
			 * the anim current anim type
			 */
			e->current_frame_time -= g_dt;
			
			if (anim_changed) {
				e->current_frame_time = e->cur_anim->dt;
				e->meta->sprite = e->cur_anim->sprite_start;
			}

			if (e->current_frame_time <= 0) {
				if (e->meta->sprite < e->cur_anim->sprite_end) {
					e->meta->sprite += 1;
				} else {
					e->meta->sprite = e->cur_anim->sprite_start;
				}

				e->current_frame_time = e->cur_anim->dt;
			}
		}

		e->pos.x += e->vel.x * g_dt;
		e->pos.y += e->vel.y * g_dt;
		/**
		 * updating the physics offset to reflect 
		 * the change in sprite position
		 */
		e->offset = e->pos + e->meta->mask.tl;
	}
}

void end_entities(void)
{
	g_player = NULL;
}

void clear_entities(void)
{
	entity *e, *n;

	dl_for_each_entry_s (e, n, &g_entities, node) {
		destroy_entity(e);
	}
}
