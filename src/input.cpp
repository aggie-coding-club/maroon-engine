#include <stdint.h>
#include <xinput.h>

#include "input.hpp"
#include "win32.hpp"

static const uint16_t g_bt_to_gp[] = {
	[BT_LEFT] = XINPUT_GAMEPAD_DPAD_LEFT,
	[BT_RIGHT] = XINPUT_GAMEPAD_DPAD_RIGHT,
	[BT_JUMP] = XINPUT_GAMEPAD_A
};

static const uint8_t g_bt_to_key[] = {
	[BT_LEFT] = VK_LEFT,
	[BT_RIGHT] = VK_RIGHT,
	[BT_JUMP] = 'Z' 
};

int g_key_down[256];
int g_buttons[COUNTOF_BT];

typedef DWORD WINAPI xinput_get_state_fn(DWORD, XINPUT_STATE *);

static DWORD WINAPI xinput_get_state_stub(DWORD ui, XINPUT_STATE *xs);
xinput_get_state_fn *g_xinput_get_state = xinput_get_state_stub;

static DWORD WINAPI xinput_get_state_stub(DWORD ui, XINPUT_STATE *xs)
{
	UNREFERENCED_PARAMETER(ui);
	UNREFERENCED_PARAMETER(xs);
	return ERROR_DEVICE_NOT_CONNECTED;
}

void init_input(void)
{
	static const char *const paths[] = {
		"xinput1_4.dll",
		"xinput1_3.dll",
		"xinput9_1_0.dll",
		NULL
	};

	static const char *const names[] = {
		"XInputGetState",
		NULL
	};

	HMODULE lib;
	FARPROC proc;

	lib = load_procs_ver(paths, names, &proc);
	if (lib) {
		g_xinput_get_state = (xinput_get_state_fn *) proc;
	}
}

static uint16_t unify_xinput(void)
{
	XINPUT_STATE xs;
	XINPUT_GAMEPAD *gp;	

	if (g_xinput_get_state(0, &xs) != ERROR_SUCCESS) {
		return 0;
	}

	gp = &xs.Gamepad;
        if (gp->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
            gp->wButtons |= XINPUT_GAMEPAD_DPAD_UP;
        } 
        if (gp->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
            gp->wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
        }
        if (gp->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
            gp->wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
        }
        if (gp->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
            gp->wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
        } 

	return gp->wButtons;
}

void update_input(void)
{
	uint16_t wb;
	int i;

	wb = unify_xinput();

	for (i = 0; i < COUNTOF_BT; i++) {
		if (g_key_down[g_bt_to_key[i]] || (wb & g_bt_to_gp[i])) {
			if (g_buttons[i] < INT_MAX) {
				g_buttons[i]++;
			}
		} else {
			g_buttons[i] = 0;
		}
	}
}

void clear_input(void)
{
	memset(g_key_down, 0, sizeof(g_key_down));
	memset(g_buttons, 0, sizeof(g_buttons));
}
