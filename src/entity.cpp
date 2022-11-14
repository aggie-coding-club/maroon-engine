#include <math.h>

#include "entity.hpp"
#include "game-map.hpp"
#include "render.hpp"

#define TLF 1
#define TRF 2
#define BLF 4
#define BRF 8

#define NEGF 1
#define POSF 2

#define CHECK_FLAGS(flags, check) ((flags &(check)) == (check))

typedef int resolve_col_fn(entity *e, const box *ebox, const box *obox);

float g_dt;
DL_HEAD(g_entities);
int g_key_down[256];

static entity *g_captain;
static const float g_gravity = 2.0F;
static int8_t cam_seek;

const entity_meta g_entity_metas[COUNTOF_EM] = {
	[EM_CAPTAIN] = {
		.mask = {
			{
				20.0F / TILE_LEN, 
				2.0F / TILE_LEN
			},
			{
				44.0F / TILE_LEN,
				32.0F / TILE_LEN
			},
		},
		.def_anim = ANIM_CAPTAIN_IDLE,
		.max_health = 10
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
		.def_anim = ANIM_CRABBY_IDLE,
		.max_health = 3
	}
};

/**
 * set_animation - Set current animation for entity
 * @e: Entity to change
 * @aid: Animation to set
 *
 * NOTE: Use change_animation to avoid animation reset in the case the 
 * new animation is the same as the old.
 */
static void set_animation(entity *e, int aid)
{
	const anim *a;

	a = g_anims + aid; 
	e->anim = aid;
	e->anim_time = ANIM_DT;
	e->sprite = a->start;
}

entity *create_entity(int tx, int ty, uint8_t em)
{
	entity *e;
	const entity_meta *meta;

	e = (entity *) xmalloc(sizeof(*e));

	dl_push_back(&g_entities, &e->node);
	e->em = em;
	e->spawn.x = tx;
	e->spawn.y = ty;

	e->pos.x = tx;
	e->pos.y = ty;
	e->vel.x = 0.0F;
	e->vel.y = 0.0F;

	e->flags = 0;

	meta = g_entity_metas + em;
	set_animation(e, meta->def_anim);

	switch (em) {
	case EM_CRABBY:
		e->vel.x = 2.0F;
		break;
	}
	
	e->health = meta->max_health;
	return e;
}

void destroy_entity(entity *e)
{
	dl_del(&e->node);
	free(e);
}

static int spawn_entity(int x, int y)
{
	uint8_t *tile;
	int em;
	entity *e;

	tile = &g_gm->rows[y][x]; 
	em = g_tile_to_em[*tile];
	if (em == EM_INVALID) {
		return 0;
	}
	e = create_entity(x, y, em); 
	if (em == EM_CAPTAIN) {
		if (g_captain) {
			err_wnd(g_wnd, L"Too many captains");
			return -1;
		}
		g_captain = e;
	}
	*tile = 0;
	return 1;
}

int start_entities(void)
{
	int y;

	for (y = 0; y < g_gm->h; y++) {
		int x;

		for (x = 0; x < g_gm->w; x++) {
			if (spawn_entity(x, y) < 0) {
				goto end;
			}
		}
	}

	if (!g_captain) {
		err_wnd(g_wnd, L"No captain found");
		goto end;
	}
	return 0;
end:
	end_entities();
	return -1;
}

/**
 * change_animation() - Change animation
 * @e: Enity to change the animation of
 * @anim: New animation to change to
 *
 * NOTE: Will not reset animation timer if new
 * animation is the same as the old.
 */
static void change_animation(entity *e, int anim)
{
	if (e->anim != anim) {
		set_animation(e, anim);
	}
}

/**
 * update_animation() - Update animation for entity
 * @e: Entity to update animation of
 */
static void update_animation(entity *e)
{
	const anim *anim;

	anim = g_anims + e->anim; 
	e->anim_time -= g_dt;
	if (e->anim_time <= 0.0F) {
		if (e->sprite < anim->end) {
			e->sprite++;
		} else if (g_anim_flags[e->anim] & AF_REPEAT) {
			e->sprite = anim->start;
		}

		e->anim_time = ANIM_DT;
	}
}

static inline bool v2_in_box(const box *b, float x, float y)
{
	bool horz, vert;
	horz = (x >= b->tl.x && x < b->br.x);
	vert = (y >= b->tl.y && y < b->br.y); 
	return horz && vert;
}

static int get_col_flags(const box *ebox, const box *obox)
{
	int flags;

	flags = 0;
	if (v2_in_box(obox, ebox->tl.x, ebox->tl.y)) {
		flags |= TLF;
	}
	if (v2_in_box(obox, ebox->br.x, ebox->tl.y)) {
		flags |= TRF;
	}
	if (v2_in_box(obox, ebox->tl.x, ebox->br.y)) {
		flags |= BLF;
	}
	if (v2_in_box(obox, ebox->br.x, ebox->br.y)) {
		flags |= BRF;
	}
	return flags;
}

static int resolve_horz_col(entity *e, const box *ebox, const box *obox)
{
	const entity_meta *em;
	int flags;

	em = g_entity_metas + e->em;
	flags = get_col_flags(ebox, obox);

	if (flags & (TLF | BLF)) {
		e->pos.x = obox->br.x - em->mask.tl.x; 
		return NEGF;
	} 
	if (flags & (TRF | BRF)) {
		e->pos.x = obox->tl.x - em->mask.br.x;
		return POSF;
	} 
	return 0;
}

static int resolve_vert_col(entity *e, const box *ebox, const box *obox)
{
	const entity_meta *em;
	int flags;

	em = g_entity_metas + e->em;
	flags = get_col_flags(ebox, obox);

	if (flags & (TLF | TRF)) {
		e->pos.y = obox->br.y - em->mask.tl.y; 
		return NEGF;
	} 
	if (flags & (BLF | BRF)) {
		e->pos.y = obox->tl.y - em->mask.br.y;
		return POSF;
	}
	return 0;
}

static int update_cols(entity *e, resolve_col_fn *resolve)
{
	const entity_meta *em;
	box ebox;
	int flags;

	int x0, y0;
	int x1, y1;
	int x, y;

	em = g_entity_metas + e->em;
	ebox = em->mask + e->pos;
	flags = 0;

	x0 = floorf(ebox.tl.x);
	y0 = floorf(ebox.tl.y); 
	x1 = ceilf(ebox.br.x);
	y1 = ceilf(ebox.br.y); 
	for (y = y0; y < y1; y++) {
		for (x = x0; x < x1; x++) {
			int tile;
			box tbox;

			tile = get_tile(x, y);
			if (!tile) {
				continue;
			}
			
			tbox.tl.x = x;
			tbox.tl.y = y;
			tbox.br.x = x + 1.0F;
			tbox.br.y = y + 1.0F;
			flags |= resolve(e, &ebox, &tbox);
		}
	}
	return flags;
}

/**
 * update_physics() - Update physics of entity
 * @e: Entity to update physics of
 */
static void update_physics(entity *e)
{
	int flags;

	e->pos.x += e->vel.x * g_dt;
	update_cols(e, resolve_horz_col);
	e->pos.y += e->vel.y * g_dt;

	flags = update_cols(e, resolve_vert_col);
	if ((flags & POSF)) {
		e->vel.y = 0.0F;
		e->flags |= EF_GROUND;
	} else if ((flags & NEGF) && e->vel.y < 0.0F) {
		e->vel.y = 0.0F;
	} else {
		e->vel.y += 20.0F * g_dt;
	}
}

static bool can_jump(const entity *e) 
{
	const entity_meta *em;
	box ebox;

	int x0, x1;
	int x, y;

	em = g_entity_metas + e->em;
	ebox = em->mask + e->pos;

	x0 = floorf(ebox.tl.x);
	x1 = ceilf(ebox.br.x);
	y = floorf(ebox.tl.y); 
	
	for (x = x0; x < x1; x++) {
		int tile;

		tile = get_tile(x, y - 1.0F);
		if (tile) {
			return false;
		}
	}

	return e->flags & EF_GROUND;
}

/**
 * update_captain() - Update captain specific behavoir
 * @e: Captain to update
 */
static void update_captain(entity *e)
{
	e->vel.x = 0.0F;

	if (g_key_down[VK_SPACE] == 1 && can_jump(e)) {
		e->vel.y = -10.0F;
		e->flags &= ~EF_GROUND;
	}

	if (g_key_down['A']) {
		e->vel.x = -4.0F;
		e->flags |= EF_FLIP;
	} 
	
	if (g_key_down['D']) {
		e->vel.x = 4.0F;
		e->flags &= ~EF_FLIP;
	} 

	if (e->vel.y < 0.0F) {
		change_animation(e, ANIM_CAPTAIN_JUMP);
	} else if (e->vel.y > 1.0F) {
		change_animation(e, ANIM_CAPTAIN_FALL);
	} else if (fabsf(e->vel.x) > 0.0F) {
		change_animation(e, ANIM_CAPTAIN_RUN);
	} else {
		change_animation(e, ANIM_CAPTAIN_IDLE);
	}

	update_physics(e);

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
	float tile_level_diff;

	float crabby_speed = 1.0F;
	
	meta = g_entity_metas + e->em;
	offset = e->pos + meta->mask.tl;

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
	/* check for crabby and player at same tile level */
	tile_level_diff = e->pos.y - g_captain->pos.y;
	if (tile_level_diff < 0) {
		tile_level_diff *= -1;
	}
	/* width of player using collision mask */
	captain_width = (g_entity_metas + g_captain->em)->mask.br.x 
					- (g_entity_metas + g_captain->em)->mask.tl.x;

	/* determine if crabby is close enough to player on either side */
	if (tile_level_diff < 0.5F) {
		if (dist_to_player < 3 * captain_width && 
			dist_to_player > captain_width) {		
			player_detected = -1;
		}
		if (dist_to_player > 0.0F && dist_to_player < captain_width) {
			player_detected = -2;
		}
		if (dist_to_player > -3.8F * captain_width && 
			dist_to_player < -1.9F * captain_width) {
			player_detected = 1;
		}
		if (dist_to_player < 0 && dist_to_player > -1.9F * captain_width) {
			player_detected = 2;
		}
	}

	/* movement behavior if player detected */
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

	/* normal crabby movement */
	if (player_detected == 0) {
		if (e->vel.x == 0.0F || e->vel.x == 3.0F || e->vel.x == -3.0F) {
			e->vel.x = crabby_speed;
		}
		
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
	}

	/* selecting the animation */
	if (fabsf(e->vel.x) > 0.05F) {
		change_animation(e, ANIM_CRABBY_RUN);
	} else {
		change_animation(e, ANIM_CRABBY_IDLE);
	}

	if (e->vel.x < 0) {
		e->flags &= ~EF_FLIP;
	}

	if (e->vel.x > 0) {
		e->flags |= EF_FLIP;
	}
	update_physics(e);
}

void heal(entity *e, int heal_amount)
{
	e->health = e->health + heal_amount;
}

void take_damage(entity *e, int damage_amount)
{
	e->health = e->health - damage_amount;
	if (e->health < 0) {
		/* add whatever happens upon death*/
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
	}
}

void end_entities(void)
{
	g_captain = NULL;
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

