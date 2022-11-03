#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <stdint.h>
#include "dl.hpp"
#include "util.hpp"

#define EM_PLAYER 0
#define COUNTOF_EM 1

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
 * struct entity_meta - Includes common information for entity 
 * @sprite: The sprite of the entity
 * @mask: Mask for data 
 */
struct entity_meta {
	uint8_t sprite;	
	const box mask;
};

/**
 * struct anim - Animation list
 * @dt: the time between the anim frame change
 * @sprite_start: first anim frame sprite id
 * @sprite_end: last anim frame sprite id
 * anim system assumes the sprites ids are next together
*/
#define ANIM_TYPE_COUNT 6
struct anim {
	float dt;
	uint8_t sprite_start;
	uint8_t sprite_end;
};

/**
 * @active: for determing sprite manager is active
 * @current_anim_type: idle, run, attack, etc.
 * @current_frame_time: the time until the current frame switches
 * @cur_anim: Current animation
*/
struct anim_manager {
};

/**
 * @node: Used to point to next entity
 * @meta: Pointer to common entites of this type
 * @v2i: Spawn position
 *
 * @pos: Current position in tiles
 * @vel: Current velocity in tiles per second
 * @offset: cached absolute position of collision mask 
 * @mask: collison mask 
 *
 * @
 */
struct entity {
	/*misc*/
	dl_head node;
	entity_meta *meta;
	v2i spawn;

	/*physics*/
	v2 pos;
	v2 vel;
	v2 offset;

	/*animation*/
	bool active;
	uint8_t current_anim_frame;
	float current_frame_time;
	const anim *cur_anim;
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
 * start_entities() - Setup entity system
 * This is a temporary function that will be deleted once we are able
 * to create entities throught the editor.
*/
void start_entities(void);

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
