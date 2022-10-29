#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <stdint.h>
#include "dl.hpp"
#include "util.hpp"

#define EM_PLAYER 0
#define COUNTOF_EM 1

/**
 * struct entity_meta - Includes common information for entity 
 * @sprite: The sprite of the entity
 */
struct entity_meta {
	uint8_t sprite;	
};

/**
 * @node: Used to point to next entity
 * @meta: Pointer to common entites of this type
 * @v2i: Spawn position
 * @pos: Current position in tiles
 * @vel: Current velocity in tiles per second
 */
struct entity {
	dl_head node;
	v2i spawn;
	entity_meta *meta;
	v2 pos;
	v2 vel;
};

/** 
 * g_dt - Frame delta in seconds
 * g_entites - Linked list of entities
 * g_entity_metas - Metadata of all entity
 */
extern float g_dt;
extern dl_head g_entities;
extern entity_meta g_entity_metas[COUNTOF_EM];

/**
 * create_entity() - Creates an entity
 * @tx: Spawn x-pos
 * @ty: Spawn y-pos
 *
 * Return: The entity
 */
entity *create_entity(int tx, int ty);

/**
 * update_entities - Updates entities 
 */
void update_entities(void);

/**
 * destroy_entity() - Destroys an entity
 * @e: Entity to be destroyed
 */
void destroy_entity(entity *e);

/**
 * clear_entities() - Destroy all entities
 */
void clear_entities(void);

#endif
