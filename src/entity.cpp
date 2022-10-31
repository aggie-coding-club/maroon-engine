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
	e->vel.y = 0.0F;
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

	/*top left*/
	g_player->physics.collision_box[0] = {
		20.0f / TILE_LEN, 
		0
	};

	/*bottom right*/
	g_player->physics.collision_box[1] = {
		20.0f / TILE_LEN + 1, 
		28.0f / TILE_LEN
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

	g_player->vel = player_vel * player_speed;
	g_player->vel.y += g_gravity;

	/**
	 * Note(Lenny) - collision detection and resolution code should go here
	*/
	if(get_tile(g_player->physics.offset.x, g_player->physics.offset.y + g_player->physics.collision_box[1].y)) {

		v2 collided_tile_pos = {
			g_player->physics.collision_box[0].x, 
			g_player->physics.collision_box[1].y
		};

		g_player->vel.y -= g_gravity;
	}

	entity *e, *n;
	dl_for_each_entry_s (e, n, &g_entities, node) {
		e->pos.x += e->vel.x * g_dt;
		e->pos.y += e->vel.y * g_dt;
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
