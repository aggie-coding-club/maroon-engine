#ifndef INPUT_HPP
#define INPUT_HPP

#define BT_LEFT 0
#define BT_RIGHT 1
#define BT_JUMP 2
#define COUNTOF_BT 3

extern int g_key_down[256];
extern int g_buttons[COUNTOF_BT];

/**
 * init_input() - Initialize input
 */
void init_input(void);

/**
 * update_input() - Update button values
 *
 * NOTE: Does not update g_key_down
 */
void update_input(void);

/**
 * clear_input() - Set buttons and keys to clear
 */
void clear_input(void);

#endif
