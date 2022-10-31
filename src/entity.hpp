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
 * @collision_box: collisions box of an entity
 * [0] is the top left coordinate and [1] is the bottom rogjt coordinate
 * @vel: Current velocity in tiles per second
 * @offset: offset the bounding box from the entity sprite position
 * offset position is essentially the top left of the collision_box
*/
struct entity_physics_properties{
	v2 collision_box[2];
	v2 offset;
	v2 vel;
};

/**
 * @node: Used to point to next entity
 * @meta: Pointer to common entites of this type
 * @v2i: Spawn position
 * @pos: Current position in tiles
 * @physics: contains all the physics data
 */
struct entity {
	dl_head node;
	v2i spawn;
	entity_meta *meta;
	v2 pos;
	entity_physics_properties physics;
};


/** 
 * g_dt - Frame delta in seconds
 * g_entites - Linked list of entities
 * g_entity_metas - Metadata of all entity
 * g_key_down - states for key down presses
 */
extern float g_dt;
extern dl_head g_entities;
extern entity_meta g_entity_metas[COUNTOF_EM];
extern int g_key_down[KEY_MAX];

/**
 * create_entity() - Creates an entity
 * @tx: Spawn x-pos
 * @ty: Spawn y-pos
 *
 * Return: The entity
 */
entity *create_entity(int tx, int ty);

/**
 * start_entities() - setup for entity related data
 * this is a temporary function that will be deleted once we are able
 * to create entities throught the editor.
*/
void start_entities(void);

/**
 * update_entities - Updates entities 
 */
void update_entities(void);

/**
 * end_entities - clean up for any data initialized in start_entities
*/
void end_entities(void);

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
