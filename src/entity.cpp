#include <math.h>

#include "entity.hpp"
#include "game-map.hpp"
#include "render.hpp"

float g_dt;
DL_HEAD(g_entities);
int g_key_down[256];

static const float g_gravity = 2.0F;
static int8_t cam_seek = 0;

const entity_meta g_entity_metas[COUNTOF_EM] = {
	[EM_CAPTAIN] = {
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
		.def_anim = ANIM_CAPTAIN_IDLE
	},
	[EM_CRABBY] = {
		.mask = {
			{
				17.0F / TILE_LEN,
				6.0F / TILE_LEN,
			},
			{
				58.0F / TILE_LEN,
				28.0F / TILE_LEN
			}
		},
		.def_anim = ANIM_CRABBY_IDLE
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
	e->anim_time = ANIM_DT;
	e->sprite = new_anim->start;
}

entity *create_entity(int tx, int ty, uint8_t em)
{
	entity *e;
	const entity_meta *meta;

	e = (entity *) xmalloc(sizeof(*e));

	dl_push_back(&g_entities, &e->node);
	e->em= em;
	e->spawn.x = tx;
	e->spawn.y = ty;

	e->pos.x = tx;
	e->pos.y = ty;
	e->vel.x = 0.0F;
	e->vel.y = 0.0F;

	e->flipped = 0;

	meta = g_entity_metas + em;
	set_animation(e, g_anims + meta->def_anim);

	switch (em) {
	case EM_CRABBY:
		e->vel.x = 2.0F;
		break;
	}
	return e;
}

void destroy_entity(entity *e)
{
	dl_del(&e->node);
	free(e);
}

void start_entities(void)
{
	uint8_t **r;
	int y;

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

		e->anim_time = ANIM_DT;
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
}

/**
 * update_captain() - Update captain specific behavoir
 * @e: Captain to update
 */
static void update_captain(entity *e)
{
	/* simple left and right captain movement */
	const entity_meta *meta;
	v2 captain_vel;
	float captain_speed;
	bool touch_below, touch_above;
	bool touch_left, touch_right;

	meta = g_entity_metas + e->em;

	/* check for tiles colliding with captain on all four sides */

	/* Collider abstraction: global coordinate representation of the 
		players 'rectangular' collider, fiddle with these values to 
		change the size/shape of collider */
	box collider = {
			{e->pos.x + meta->mask.tl.x, e->pos.y + meta->mask.tl.y},
			{e->pos.x + meta->mask.br.x, e->pos.y + meta->mask.br.y}
			};

	/* seperation vector: value uesed ot store the 'seperation' needed 
		between the collider and player,  think of it as 
		representation of an overlap */
	v2 sep_vector = {0,0};

	/* For all four directions, determine if a collision occurs, 
		if a collision occurs, calculate the amount of 'overlap' 
		between collider and tile, and set to respective sep_vector value
	The key thing here is that the sep_vector is set to the larger of the x 
		and larger of the y components
	Overlap is calculated by flooring the respective x/y location 
		of the collider and (if needed) adding 1 to get the 'edge' of 
		the tile the collider is in */
	touch_below = get_tile(collider.tl.x, collider.br.y) ||
			get_tile(collider.br.x, collider.br.y);

	if(touch_below) {
		if(fabs(floorf(collider.br.y) - collider.br.y) > fabs(sep_vector.y)) {
			sep_vector.y = floorf(collider.br.y) - collider.br.y;
		}
	}

	touch_above = get_tile(collider.tl.x, collider.tl.y) ||
			get_tile(collider.br.x, collider.tl.y);

	if(touch_above) {
		if(fabs(floorf(collider.tl.y + 1) - collider.tl.y) > 
			fabs(sep_vector.y)) {
			sep_vector.y = floorf(collider.tl.y + 1) - collider.tl.y;
		};
	}

	touch_left = get_tile(collider.tl.x, collider.tl.y) || 
		get_tile(collider.tl.x, collider.br.y);

	if (touch_left) {
		if (fabs(floorf(collider.tl.x + 1) - collider.tl.x) > 
			fabs(sep_vector.x)) {
			sep_vector.x = floorf(collider.tl.x + 1) - collider.tl.x;
		};
	}

	touch_right = get_tile(collider.br.x, collider.tl.y) || 
	get_tile(collider.br.x, collider.br.y);
	if (touch_right) {
		if (fabs(floorf(collider.br.x) - collider.br.x) > fabs(sep_vector.x)) {
			sep_vector.x = floorf(collider.br.x) - collider.br.x;
		};
	}

	/* This is a hard-coded edge case for corners. In a corner, 
		all colliders trigger and we need to flip our sep_vector for the 
		smallest of our overlaps and resolve both directions */
	if (touch_right && touch_left && touch_below && touch_above) {
		if(fabs(floorf(collider.tl.x + 1) - collider.tl.x) < 
			fabs((floorf(collider.br.x) - collider.br.x))) {
			sep_vector.x = floorf(collider.tl.x + 1) - collider.tl.x;
		} else {
			sep_vector.x = floorf(collider.br.x) - collider.br.x;
		}

		if(fabs(floorf(collider.tl.y+1) - collider.tl.y) < 
			fabs(floorf(collider.br.y) - collider.br.y)) {
			sep_vector.y = floorf(collider.tl.y + 1) - collider.tl.y;
		} else {
			sep_vector.y = floorf(collider.br.y) - collider.br.y;
		}

		e->pos.x += sep_vector.x;
		e->pos.y += sep_vector.y;
	} 

	/* When the corner edge-case is not being handled, the code will 
		calculate the smallest distance in the x or y direction to 
		move the player in order to resolve a collision
	 	and change the position of the player in only that direction */
	else if(fabs(sep_vector.x) < fabs(sep_vector.y) || 
		(sep_vector.y == 0 && sep_vector.x != 0)) {
		e->pos.x += sep_vector.x;
	} else if(sep_vector.y != 0) {
		e->pos.y += sep_vector.y;
	}

	captain_vel.x = 0.0F;
	captain_vel.y = 0.0F;
	captain_speed = 4.0F;

	if (g_key_down['W']) {
		captain_vel.y = -1.0F;
	}

	if (g_key_down['S']) {
		captain_vel.y = 1.0F;
	}

	if (g_key_down['A']) {
		captain_vel.x = -1.0F;
	}
	
	if (g_key_down['D']) {
		captain_vel.x = 1.0F;
	}

	e->vel = captain_vel * captain_speed;
	e->vel.y += g_gravity;

	/* figuring out which aniemation to use */
	if (fabsf(e->vel.x) > 0.05F) {
		change_animation(e, &g_anims[ANIM_CAPTAIN_RUN]);
	} else {
		change_animation(e, &g_anims[ANIM_CAPTAIN_IDLE]);
	}

	if (e->vel.x < 0) {
		e->flipped = true;
	}

	if (e->vel.x > 0) {
		e->flipped = false;
	}

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
	float dist_to_player;
	float captain_width;

	meta = g_entity_metas + e->em;
	offset = e->pos + meta->mask.tl;

	float crabby_speed = 1.0f;
	e->vel.x = crabby_speed;

	tile_id_left = get_tile(offset.x, 
			offset.y + meta->mask.br.y + 0.1F);
	tile_id_right = get_tile(offset.x + 
			(meta->mask.br.x - meta->mask.tl.x), 
			offset.y + meta->mask.br.y + 0.1F);

	if (tile_id_left == TILE_SOLID || tile_id_left != TILE_GRASS) {
		e->vel.x = crabby_speed;
	} else if (tile_id_right == TILE_SOLID || 
			tile_id_right != TILE_GRASS) {
		e->vel.x = -crabby_speed;
	} 

	/**
	 * detecting the player and charging at them
	 * 
	 * wall_collide will be handled once physics is implemented
	 * player_detected indicates direction of detection
	 * using these integers:
	 * 
	 * -1 indicates left, 1 indicates right, and 0 indicates
	 * no detection
	 * -2 and 2 represent directional detection when crabby
	 * is right next to player
	 */
	wall_collide = 0;
	player_detected = 0;
	/* horizontal distance to player */
	dist_to_player = e->pos.x - g_captain->pos.x;
	/* width of player using collision mask */
	captain_width = (g_entity_metas + g_captain->em)->mask.br.x 
					- (g_entity_metas + g_captain->em)->mask.tl.x;

	/* determine if crabby is close enough to player on either side */
	if (dist_to_player < 3 * captain_width && 
		dist_to_player > captain_width) {		
		player_detected = -1;
	}
	if (dist_to_player > 0 && dist_to_player < captain_width) {
		player_detected = -2;
	}
	if (dist_to_player > -3 * captain_width && 
		dist_to_player < -captain_width) {
		player_detected = 1;
	}
	if (dist_to_player < 0 && dist_to_player > -captain_width) {
		player_detected = 2;
	}

	/* movement behavior if player detected */
	if (player_detected != 0) {
		if (player_detected == -1 && wall_collide != -1) {
			e->vel.x = -3.0F;
		}
		if (player_detected == -2 && wall_collide != -1) {
			e->vel.x = 0.0F;
		}
		if (player_detected == 1 && wall_collide != 1) {
			e->vel.x = 3.0F;
		}
		if (player_detected == 2 && wall_collide != 1) {
			e->vel.x = 0.0F;
		}
	}

	/* selecting the animation */
	if (fabsf(e->vel.x) > 0.05F) {
		change_animation(e, &g_anims[ANIM_CRABBY_RUN]);
	} else {
		change_animation(e, &g_anims[ANIM_CRABBY_IDLE]);
	}

	if (e->vel.x < 0) {
		e->flipped = false;
	}

	if (e->vel.x > 0) {
		e->flipped = true;
	}
}

static void update_specific(entity *e)
{
	switch (e->em) {
	case EM_CAPTAIN:
		update_captain(e);
		break;
	case EM_CRABBY:
		update_crabby(e);
		break;
	}
}

void update_entities(void)
{
	entity *e, *n;

	dl_for_each_entry_s (e, n, &g_entities, node) {
		update_specific(e);
		update_animation(e);
		update_physics(e);
	}
}

void end_entities(void)
{
	clear_entities();
}

void clear_entities(void)
{
	entity *e, *n;

	dl_for_each_entry_s (e, n, &g_entities, node) {
		uint8_t *tile;

		tile = g_gm->rows[e->spawn.y] + e->spawn.x;
		*tile = g_em_to_tile[e->em];
		destroy_entity(e);
	}
}

