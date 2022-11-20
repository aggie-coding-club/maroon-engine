#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <stdint.h>
#include "dl.hpp"
#include "util.hpp"
#include "sprites.hpp"

#define EM_CAPTAIN 0
#define EM_CRABBY 1
#define EM_INVALID 255
#define COUNTOF_EM 2

#define EF_FLIP 1
#define EF_GROUND 2
#define EF_CEIL 4

/**
 * struct box - Box
 * @tl: Top-left of box
 * @br: Bottom-right of box
 */
struct box {
	v2 tl;
	v2 br;
};

/**
 * struct entity_meta - Includes constant info for entity 
 * @mask: Collision mask
 */
struct entity_meta {
	box mask;
};

/**
 * misc:
 * @node: Used to point to next entity
 * @v2i: Spawn position
 * @em: Index of meta
 * @health: current health
 * @flags: flags for entity 
 *
 * physics:
 * @pos: Current position in tiles
 * @vel: Current velocity in tiles per second
 *
 * animation:
 * @anim_time: Time till next animation frame
 * @anim: Animation
 */
struct entity {
	dl_head node;
	v2 pos;
	v2 vel;
	float health;
	float anim_time;
	v2i spawn;
	uint8_t flags;
	uint8_t sprite;	
	uint8_t anim;
	uint8_t em;
};

/** 
 * g_dt - Frame delta in seconds
 * g_entites - Linked list of entities
 * g_entity_metas - Metadata of all entity
 */
extern float g_dt;
extern dl_head g_entities;
extern const entity_meta g_entity_metas[COUNTOF_EM];
extern const uint8_t g_def_anims[COUNTOF_EM]; 

/**
 * operator+ - Offset box by 2D vector
 * @b: Box to offset
 * @v: Offset
 *
 * Return: The offseted box
 */
inline box operator+(box b, v2 v)
{
	return (box) {b.tl + v, b.br + v};
}

/**
 * create_entity() - Creates an entity
 * @tx: Spawn x-pos
 * @ty: Spawn y-pos
 *
 * Return: The entity
 */
entity *create_entity(int tx, int ty, uint8_t meta, float health, float max_health);

/**
 * start_entities() - Setup entity system
 * This is a temporary function that will be deleted once we are able
 * to create entities throught the editor.
 *
 * Return: Returns zero on success and negative on failure
 *
 * Failure occurs if no player is found
*/
int start_entities(void);

/**
 * update_entities - Updates entities 
 */
void update_entities(void);

/**
 * end_entities - End entity system 
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
