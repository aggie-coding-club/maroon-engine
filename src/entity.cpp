#include <math.h>

#include "input.hpp"
#include "render.hpp"
#include "win32.hpp"

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

static entity *g_captain;
static float g_focus;

const entity_meta g_entity_metas[COUNTOF_EM] = {
	[EM_CAPTAIN] = {
		.mask = {
			{
				24.0F / TILE_LEN, 
				2.0F / TILE_LEN
			},
			{
				39.0F / TILE_LEN,
				32.0F / TILE_LEN
			},
		},
	},
	[EM_CRABBY] = {
		.mask = {
			{
				28.0F / TILE_LEN,
				6.0F / TILE_LEN,
			},
			{
				47.0F / TILE_LEN,
				29.0F / TILE_LEN
			}
		},
	}
};

const uint8_t g_def_anims[COUNTOF_EM] = {
	[EM_CAPTAIN] = ANIM_CAPTAIN_IDLE,
	[EM_CRABBY] = ANIM_CRABBY_IDLE
};

static const uint8_t g_healths[COUNTOF_EM] = {
	[EM_CAPTAIN] = 10,
	[EM_CRABBY] = 3
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

	set_animation(e, g_def_anims[em]);
	
	e->health = g_healths[em];
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

static bool is_tile_solid(int tile)
{
	return g_tile_props[tile] & PROP_SOLID;
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
			if (is_tile_solid(tile)) {
				tbox.tl.x = x;
				tbox.tl.y = y;
				tbox.br.x = x + 1.0F;
				tbox.br.y = y + 1.0F;
				flags |= resolve(e, &ebox, &tbox);
			}
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
	flags = update_cols(e, resolve_horz_col);
	if (flags & (POSF | NEGF)) {
		e->vel.x = 0.0F;
	}

	e->pos.y += e->vel.y * g_dt;
	flags = update_cols(e, resolve_vert_col);
	if (flags & POSF) {
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
		if (is_tile_solid(tile)) {
			return false;
		}
	}

	return e->flags & EF_GROUND;
}

static void idle_or_run_anim(entity *e, int run, int idle)
{
	if (fabsf(e->vel.x) > 0.05F) {
		change_animation(e, run);
	} else {
		change_animation(e, idle);
	}
}

static void auto_flip(entity *e)
{
	if (e->vel.x < 0.0F) {
		e->flags &= ~EF_FLIP;
	} else if (e->vel.x > 0.0F) {
		e->flags |= EF_FLIP;
	}
}

static void update_cam(entity *e) 
{
	box mask;
	float off;
	float dx;

	mask = g_entity_metas[e->em].mask;
	off = e->pos.x + mask.tl.x - g_cam.x;
	dx = e->vel.x * g_dt;
	if (e->vel.x > 0.0F) {
		if (g_focus < 0.5F) {
			g_focus += dx;
		} else {
			g_focus = 0.5F;
			if (off < 3.0F) {
				g_cam.x -= dx; 
			} else if (off > 4.0F) { 
				g_cam.x += dx; 
			}
			g_cam.x += dx;
		}
	} else if (e->vel.x < 0.0F) {
		if (g_focus > -0.5F) {
			g_focus += dx;
		} else {
			g_focus = -0.5F;
			if (off < 4.0F) {
				g_cam.x += dx; 
			} else if (off > 4.5F) { 
				g_cam.x -= dx; 
			}
			g_cam.x += dx;
		}
	}

	if (bound_cam()) { 
		g_focus = 0.0F;
	}
}


/**
 * update_captain() - Update captain specific behavoir
 * @e: Captain to update
 */
static void update_captain(entity *e)
{
	e->vel.x = 0.0F;

	if (g_buttons[BT_JUMP] == 1 && can_jump(e)) {
		e->vel.y = -10.0F;
		e->flags &= ~EF_GROUND;
	}

	if (g_buttons[BT_LEFT]) {
		e->vel.x = -4.0F;
		e->flags |= EF_FLIP;
	} 
	
	if (g_buttons[BT_RIGHT]) {
		e->vel.x = 4.0F;
		e->flags &= ~EF_FLIP;
	} 

	if (e->vel.y < 0.0F) {
		change_animation(e, ANIM_CAPTAIN_JUMP);
	} else if (e->vel.y > 1.0F) {
		change_animation(e, ANIM_CAPTAIN_FALL);
	} else {
		idle_or_run_anim(e, ANIM_CAPTAIN_RUN, ANIM_CAPTAIN_IDLE);
	}

	update_physics(e);
	update_cam(e);
}

static bool crabby_to_player(entity *e)
{
	v2 dis;
	const box *cap_mask;
	float cap_width;

	dis = e->pos - g_captain->pos;
	if (fabsf(dis.y) >= 0.5F) {
		return false;
	}

	cap_mask = &g_entity_metas[g_captain->em].mask; 
	cap_width = cap_mask->br.x - cap_mask->tl.x;

	/*crabby is far right of captain*/
	if (dis.x > cap_width && dis.x < 3.0F * cap_width) {
		e->vel.x = -3.0F;
		return true;
	}

	/*crabby is near right of captain*/
	if (dis.x > 0.0F && dis.x < cap_width) {
		e->vel.x = 0.0F;
		return true;
	}

	if (dis.x > -3.8F * cap_width && dis.x < -1.9F * cap_width) {
		e->vel.x = 3.0F;
		return true;
	}
	if (dis.x < 0.0F && dis.x > -1.9F * cap_width) {
		e->vel.x = 0.0F;
		return true;
	}

	return false;
}

static void crabby_walk(entity *e) 
{
	const entity_meta *meta;
	box col;
	int l, r;
	int bl, br;

	meta = g_entity_metas + e->em;

	col = meta->mask + e->pos;
	l = get_tile(col.tl.x - 0.15F, col.br.y - 1.0F);
	r = get_tile(col.br.x, col.br.y - 1.0F);

	bl = get_tile(col.tl.x, col.br.y + 0.1F);
	br = get_tile(col.br.x, col.br.y + 0.1F);

	if (!is_tile_solid(bl) || is_tile_solid(l)) {
		e->vel.x = 1.0F;
	} else if (!is_tile_solid(br) || is_tile_solid(r)) {
		e->vel.x = -1.0F;
	} else if (fabsf(e->vel.x) != 1.0F) {
		e->vel.x = 1.0F;
	}
}

static void update_crabby(entity *e)
{
	if (!crabby_to_player(e)) {
		crabby_walk(e);
	}
	idle_or_run_anim(e, ANIM_CRABBY_RUN, ANIM_CRABBY_IDLE);
	auto_flip(e);
	update_physics(e);
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
	g_focus = 0.0F;
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

