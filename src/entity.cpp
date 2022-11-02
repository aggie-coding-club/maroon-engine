#include "entity.hpp"
#include <stdio.h>
#include "sprites_files_list.hpp"
#include "game_map.hpp"
#include "render.hpp"

float g_dt;
static float g_gravity;
static entity *g_player;

DL_HEAD(g_entities);
entity_meta g_entity_metas[COUNTOF_EM] = {
	/*INSERT PLAYER META*/
	SPR_CAPTAIN_SWORD_RUN_1
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
	e->physics.vel.x = 0.0F;
	e->physics.vel.y = 0.0F;
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

	/*top left*/
	g_player->physics.collision_box[0] = {
		20.0f / TILE_LEN, 
		0
	};

	/*bottom right*/
	g_player->physics.collision_box[1] = {
		20.0f / TILE_LEN + 1, 
		30.0f / TILE_LEN
	};

	g_player->animation_m.active = true;
	g_player->animation_m.animations[0] = {
		0.1f,
		SPR_CAPTAIN_SWORD_RUN_1,
		SPR_CAPTAIN_SWORD_RUN_6,
	};
	g_player->animation_m.animations[1] = {
		0.2f,
		SPR_CAPTAIN_SWORD_IDLE_1,
		SPR_CAPTAIN_SWORD_IDLE_5,
	};
	g_gravity = 2.0f;
}

void update_entities(void)
{
	/*simple left and right player movement*/
	v2 player_vel;
	float player_speed = 4;
	player_vel = {0, 0};

	if(g_key_down[KEY_W] || g_key_down[KEY_UP]) {
		player_vel.y = -1.0f;
	}

	if(g_key_down[KEY_S] || g_key_down[KEY_DOWN]) {
		player_vel.y = 1.0f;
	}

	if(g_key_down[KEY_D] || g_key_down[KEY_LEFT]) {
		player_vel.x = 1.0f;
	}
	
	if(g_key_down[KEY_A] || g_key_down[KEY_RIGHT]) {
		player_vel.x = -1.0f;
	}

	g_player->physics.vel = player_vel * player_speed;
	g_player->physics.vel.y += g_gravity;

	/**
	 * Note(Lenny) - collision detection and resolution code should go here
	 * the current method is not ideal
	*/
	if(get_tile(g_player->physics.offset.x, 
		g_player->physics.offset.y + g_player->physics.collision_box[1].y)) {

		/* the position of the collided tile */
		v2 collided_tile_pos = {
			(float)(int)g_player->physics.collision_box[0].x, 
			(float)(int)g_player->physics.collision_box[1].y
		};

		g_player->physics.vel.y -= g_gravity;
	}

	/* figuring out which animation to use*/
	bool animation_changed = false;
	if (g_player->physics.vel.x > 0.05f || g_player->physics.vel.x < -0.05f) {
		if (g_player->animation_m.current_animation_type != 0) {
			animation_changed = true;
		}
		g_player->animation_m.current_animation_type = 0;
	}else{
		if (g_player->animation_m.current_animation_type != 1) {
			animation_changed = true;
		}
		g_player->animation_m.current_animation_type = 1;
	}

	entity *e, *n;
	dl_for_each_entry_s (e, n, &g_entities, node) {
		if (e->animation_m.active) {
			/* change animation state based on the animation current animation type */
			animation_manager *am = &e->animation_m;
			animation *current = &am->animations[am->current_animation_type]; 

			am->current_frame_time -= g_dt;
			
			if (animation_changed) {
				am->current_frame_time = current->timeBtwFrames;
				e->meta->sprite = current->sprite_start;
			}

			if (am->current_frame_time <= 0) {
				if (e->meta->sprite < current->sprite_end) {
					e->meta->sprite += 1;
				} else {
					e->meta->sprite = current->sprite_start;
				}

				am->current_frame_time = current->timeBtwFrames;
			}
		}

		e->pos.x += e->physics.vel.x * g_dt;
		e->pos.y += e->physics.vel.y * g_dt;
		/*updating the physics offset to reflect the change in sprite position*/
		e->physics.offset.x = e->pos.x + e->physics.collision_box[0].x;
		e->physics.offset.y = e->pos.y + e->physics.collision_box[0].y;
	}

}

void end_entities()
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
