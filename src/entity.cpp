#include <math.h>

#include "entity.hpp"
#include "game_map.hpp"
#include "render.hpp"

float g_dt;
DL_HEAD(g_entities);
int g_key_down[256];

static const float g_gravity = 2.0F;
static int8_t cam_seek = 0;

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
	}
};

static entity *g_captain = NULL;

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
	g_player = create_entity(3, 3);

	r = g_gm->rows;
	for (y = 0; y < g_gm->h; y++) {
		uint8_t *c;
		int x;

		c = *r; 
		for (x = 0; x < g_gm->w; x++) {
			int em;

			em = g_tile_to_em[*c];
			if (em != EM_INVALID) {
				entity *e = create_entity(x, y, em); 

				if (em == EM_CAPTAIN) {
					g_captain = e;
				}

				*c = 0;
			}
			c++;
		}
		r++;
	}

	if (g_captain == NULL) {
		printf("No player on this level!\n");
	}
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
	
	player_vel.x = 0.0F;
	player_vel.y = 0.0F;
	player_speed = 4.0F;

	if (g_key_down['W']) {
		player_vel.y = -1.0F;
	}

	if (g_key_down['S']) {
		player_vel.y = 1.0F;
	}

	if (g_key_down['A']) {
		player_vel.x = -1.0F;
	}
	
	if (g_key_down['D']) {
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

		v2 collided_tile_pos = {
			(float) (int) g_player->meta->mask.tl.x, 
			(float) (int) g_player->meta->mask.br.y
		};

	/* camera follow */
	box b = g_entity_metas[e->em].mask;
	v2 cap_pos = e->pos + b.tl;
	float bound = 2.0F;

	float dist_to_end = (g_cam.w - cap_pos.x) + g_cam.x;
	float dist_to_start = (g_cam.w - dist_to_end);

	/*TODO(Lenny): Make the camera slow down as it gets closer to cap*/
	float catch_up_speed = 4.0F;
	if (cam_seek) {

		g_cam.x += catch_up_speed * cam_seek * g_dt;

		if (dist_to_end >= g_cam.w / 2 && cam_seek == 1) {
			cam_seek = 0;
			catch_up_speed = catch_up_speed;
		} else if (dist_to_start >= g_cam.w / 2 && cam_seek == -1) {
			cam_seek = 0;
			catch_up_speed = catch_up_speed;
		}
	}

	if (dist_to_end < bound && cam_seek == false) {
		cam_seek = 1;
	} else if (dist_to_start < bound && cam_seek == false) {
		cam_seek = -1;
	}

	if (g_cam.x < 0.0F) {
		g_cam.x = 0.0F;
		cam_seek = 0;
	}

	bound_cam();
}

static void update_crabby(entity *e)
{
	/* crabby movement */
	const entity_meta *meta;
	v2 offset;
	uint8_t tile_id_left; 
	uint8_t tile_id_right;

	/* for player detection/charging */
	int wall_collide;
	int player_detected;

	meta = g_entity_metas + e->em;
	offset = e->pos + meta->mask.tl;

	float crabby_speed = 1.0f;

	tile_id_left = get_tile(offset.x, 
			offset.y + meta->mask.br.y + 0.1F);
	tile_id_right = get_tile(offset.x + 
			(meta->mask.br.x - meta->mask.tl.x), 
			offset.y + meta->mask.br.y + 0.1F);

	if (tile_id_left == TILE_SOLID || tile_id_left != TILE_GRASS) {
		e->vel.x *= -crabby_speed;
	} else if (tile_id_right == TILE_SOLID || 
			tile_id_right != TILE_GRASS) {
		e->vel.x *= -crabby_speed;
	} 

	/**
	 * detecting the player and charging at them
	 * 
	 * variables will be set appropriately once tile and
	 * player detection is implemented for crabby
	 * 
	 * -1 indicates left, 1 indicates right, and 0 indicates
	 * no detection
	 */
	wall_collide = 0;
	player_detected = 0;

	if (player_detected != 0) {
		/* crabby moves left if player is left and no wall is left*/
		if (player_detected == -1 && wall_collide != -1) {
			e->vel.x = -3.0F;
		}
		/* crabby moves right if player is right and no wall is right */
		if (player_detected == 1 && wall_collide != 1) {
			e->vel.x = 3.0F;
		}
	}

	/* selecting the animation */
	if (fabsf(e->vel.x) > 0.05F) {
		change_animation(e, &g_anims[ANIM_CRABBY_RUN]);
	} else {
		change_animation(g_player, &g_captain_idle_anim);
	}

	dl_for_each_entry_s (e, n, &g_entities, node) {
		update_animation(e);
		update_physics(e);
	}
		/*check for tiles colliding with player on all four sides*/

		box collider = {
			{g_player->pos.x+g_player->meta->mask.tl.x, g_player->pos.y+g_player->meta->mask.tl.y}
			{g_player->pos.x+g_player->meta->mask.br.x, g_player->pos.y+g_player->meta->mask.br.y}
			}
		double COLLIDER_OFFSET = 0.001

	touch_below = get_tile(collider.tl.x, collider.br.y) ||
			get_tile(collider.tl.x, collider.br.y);
	if(touch_below) {
		g_player->pos.y += (((int) (collider.br.y)) - (collider.br.y));
	}
	touch_above = get_tile(collider.tl.x, collider.tl.y) ||
			get_tile(collider.br.x, collier.br.y);
	if(touch_above) {
		g_player->pos.y += (((int)collider.tl.y)+1 - collider.tl.y);
	}
	touch_left = get_tile(collider.tl.x, collider.tl.y+COLLIDER_OFFSET) || get_tile(collider.tl.x, collider.br.y-COLLIDER_OFFSET);
	if(touch_left) {
		g_player->pos.x +=
		((int) floorf(collider.tl.x)) + 1 -(collider.tl.x);
	}
	touch_right = get_tile(collider.br.x, collider.tl.y+COLLIDER_OFFSET) || get_tile(collider.br.x, collider.br.y-COLLIDER_OFFSET);
	if(touch_right) {
		g_player->pos.x +=
		((int) (collider.br.x)) - (collider.br.x);
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

