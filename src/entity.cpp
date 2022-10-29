#include "entity.hpp"
#include <stdio.h>

float g_dt;

DL_HEAD(g_entities);
entity_meta g_entity_metas[COUNTOF_EM] = {
	/*INSERT PLAYER META*/
};

entity *create_entity(int tx, int ty)
{
	entity *e;
	e = (entity *) xmalloc(sizeof(*e));

	dl_push_back(&g_entities, &e->node);
	e->meta = &g_entity_metas[EM_PLAYER];
	e->spawn.tx = tx;
	e->spawn.ty = ty;
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

void update_entities(void)
{
	entity *e, *n;
	dl_for_each_entry_s (e, n, &g_entities, node) {
		e->pos.x += e->vel.x * g_dt;
		e->pos.y += e->vel.y * g_dt;
	}
}

void clear_entities(void)
{
	entity *e, *n;

	dl_for_each_entry_s (e, n, &g_entities, node) {
		destroy_entity(e);
	}
}
