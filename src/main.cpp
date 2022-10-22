#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <windows.h>
#include <windowsx.h>

#include <commdlg.h>
#include <fileapi.h>
#include <glad/glad.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "menu.hpp"
#include "render.hpp"
#include "tile_map.hpp"

#define MAX_EDITS 256ULL

#define CLIENT_WIDTH 640
#define CLIENT_HEIGHT 480

#define MAX_MAP_LEN 999 

enum edit_type {
	EDIT_NONE,
	EDIT_PLACE,
	EDIT_RESIZE
};

struct edit_place {
	int x;
	int y;
	int tile;
};

struct edit_resize {
	int w;
	int h;
};

struct edit {
	edit_type type;
	union {
		edit_place place;
		edit_resize resize;
	};
};

static HINSTANCE g_ins;
static HMENU g_menu;
static HACCEL g_acc; 

static FT_Library g_freetype_library;

static int g_client_width = CLIENT_WIDTH;
static int g_client_height = CLIENT_HEIGHT;

static wchar_t g_map_path[MAX_PATH];

static bool g_change;

static edit g_edits[MAX_EDITS];
static size_t g_edit_next;

static int g_place = 1;

/** 
 * cd_parent() - Transforms full path into the parent full path 
 * @path: Path to transform
 */
static void cd_parent(wchar_t *path)
{
	wchar_t *find;

	find = wcsrchr(path, '\\');
	if (find) {
		*find = '\0';
	}
}

/** 
 * set_default_directory() - Set working directory to be repository folder 
 */
static void set_default_directory(void)
{
	wchar_t path[MAX_PATH];

	GetModuleFileNameW(NULL, path, MAX_PATH);
	cd_parent(path);
	cd_parent(path);
	SetCurrentDirectoryW(path);
}

/**
 * fclampf() - Single precision float clamp
 * @v: Value to clamp
 * @l: Min value
 * @h: Max value
 */
static float fclampf(float v, float l, float h)
{
	return fminf(fmaxf(v, l), h);
}

/**
 * update_scrollbars() - Refresh scrollbar 
 * @width: The new width of the window
 * @height: The new height of the window
 *
 * Used when the window size changes, the game map sizes,
 * or when the zoom factor changes.
 * 
 */
static void update_scrollbars(int width, int height)
{
	SCROLLINFO si;

	g_cam.x = fclampf(g_cam.x, g_tm.w - g_cam.w, 0);
	g_cam.y = fclampf(g_cam.y, g_tm.h - g_cam.h, 0);

	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = g_tm.w * width / g_cam.w;
	si.nPage = width + 1;
	si.nPos = g_cam.x * width / g_cam.w;
	SetScrollInfo(g_wnd, SB_HORZ, &si, TRUE);

	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = g_tm.h * height / g_cam.h;
	si.nPage = height + 1;
	si.nPos = g_cam.y * height / g_cam.h;
	SetScrollInfo(g_wnd, SB_VERT, &si, TRUE);
}

/**
 * update_size() - Called to acknowledge changes in client area size
 * @width: New client width
 * @height: New client height
 */
static void update_size(int width, int height)
{
	/*must be called before updating client dimensions*/
	update_scrollbars(width, height);

	g_client_width = width;
	g_client_height = height;
	glViewport(0, 0, width, height);
}

/**
 * unsaved_warning() - Display unsaved changes will be lost
 * if changes "g_change" is true.
 *
 * Return: If warning is not displayed, or the OK button
 * is pressed returns true, elsewise false 
 */
static bool unsaved_warning(void)
{
	return !g_change || MessageBoxW(g_wnd, 
			L"Unsaved changes will be lost", L"Warning", 
			MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK;
}

/**
 * write_map() - Save map to file.
 * @path - Path to map
 *
 * Display message box on failure
 *
 * Return: Returns zero on success, and negative number on failure
 */
static int write_map(const wchar_t *path) 
{
	int err;
	FILE *f;
	uint16_t v[2];
	uint8_t **row;
	int n;

	err = -1;
	f = _wfopen(path, L"wb");
	if (!f) {
		goto err0;
	}

	/*write width and height of map*/
	v[0] = g_tm.w;
	v[1] = g_tm.h;
	if (fwrite(v, sizeof(v), 1, f) < 1) {
		goto err1;
	}

	row = g_tm.rows;
	n = g_tm.h;
	while (n-- > 0) {
		if (fwrite(*row, g_tm.w, 1, f) < 1) {
			goto err1;
		}
		row++;
	}

	/*error handling and cleanup*/
	err = 0;
err1:
	fclose(f);
	if (err >= 0) {
		g_change = false;
	}
err0:
	if (err < 0) {
		MessageBoxW(g_wnd, L"Could not save map", 
				L"Error", MB_ICONERROR);
	}
	return err;
}

/**
 * read_map() - Read map
 * @path - Path to map
 *
 * Display message box on failure
 *
 * Return: Returns zero on success, and negative number on failure
 */
static int read_map(const wchar_t *path)
{
	int err;
	FILE *f;
	uint16_t v[2];
	tile_map tm;
	uint8_t **row;
	int n;

	err = -1;
	f = _wfopen(path, L"rb");
	if (!f) {
		goto err0;
	}

	/*read width and height of map*/
	if (fread(&v, sizeof(v), 1, f) < 1) {
		goto err1;
	}

	if (v[0] > MAX_MAP_LEN || v[1] > MAX_MAP_LEN) {
		goto err1;
	}
	init_tm(&tm);	
	size_tm(&tm, v[0], v[1]);

	row = tm.rows;
	n = tm.h;
	while (n-- > 0) {
		if (fread(*row, tm.w, 1, f) < 1) {
			reset_tm(&tm);
			goto err1;
		}
		row++;
	}
	reset_tm(&g_tm);
	g_tm = tm;
	
	/*error handling and cleanup*/
	err = 0;
err1:
	fclose(f);
	if (err >= 0) {
		g_change = false;
	}
err0:
	if (err < 0) {
		MessageBoxW(g_wnd, L"Could not read map", 
				L"Error", MB_ICONERROR);
	}
	return err;
}

/**
 * reset_edits() - Reset edit buffer
 *
 * Action buffer refers to the buffer for
 * undo and redo.
 */
static void reset_edits(void)
{
	int undo;

	/*establish barrier for redo*/
	g_edits[g_edit_next].type = EDIT_NONE;

	/*acts as barrier for undo*/
	undo = (g_edit_next - 1ULL) % MAX_EDITS;
	g_edits[undo].type = EDIT_NONE;
}

/**
 * open() - Open map dialog 
 */
static void open(void)
{
	wchar_t path[MAX_PATH];
	wchar_t init[MAX_PATH];
	OPENFILENAMEW ofn;

	if (!unsaved_warning()) {
		return;
	}

	wcscpy(path, g_map_path);
	GetFullPathNameW(L"res\\maps", MAX_PATH, init, NULL);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_wnd;
	ofn.lpstrFilter = L"Map (*.gm)\0*.gm\0";
	ofn.lpstrFile = g_map_path;
	ofn.nMaxFile = MAX_PATH; 
	ofn.lpstrInitialDir = init;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"gm";

	if (GetOpenFileName(&ofn) && read_map(g_map_path) >= 0) {
		g_change = false;
		reset_edits();
		g_cam.x = 0;
		g_cam.y = 0;
		update_scrollbars(g_client_width, g_client_height);
	} else {
		wcscpy(g_map_path, path);
	}
}

/**
 * save_as() - Save as map dialog 
 */
static void save_as(void)
{
	wchar_t path[MAX_PATH];
	wchar_t init[MAX_PATH];
	OPENFILENAMEW ofn;

	wcscpy(path, g_map_path);
	GetFullPathNameW(L"res\\maps", MAX_PATH, init, NULL);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_wnd;
	ofn.lpstrFilter = L"Map (*.gm)\0*.gm\0";
	ofn.lpstrFile = g_map_path;
	ofn.nMaxFile = MAX_PATH; 
	ofn.lpstrInitialDir = init;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"gm";

	if (!GetSaveFileNameW(&ofn) || write_map(g_map_path) < 0) {
		wcscpy(g_map_path, path);
	}
}

/**
 * save() - Saves file if path non-empty, else identical to "save_as"
 */
static void save(void)
{
	if (g_map_path[0]) {
		write_map(g_map_path);
	} else {
		save_as();
	}
}

/**
 * resize_edit() - Use resize edit to resize map
 * @ed: Edit
 */
static void resize_edit(edit *ed)
{
	int w, h;

	w = ed->resize.w;
	h = ed->resize.h;
	ed->resize.w = g_tm.w; 
	ed->resize.h = g_tm.h; 
	size_tm(&g_tm, w, h);
	update_scrollbars(g_client_width, g_client_height);
}

/**
 * undo() - Undos edit
 */
static void undo(void)
{
	edit *ed;
	uint8_t *tp;
	int tile;
	
	g_edit_next = (g_edit_next - 1ULL) % MAX_EDITS;
	ed = g_edits + g_edit_next;
	switch (ed->type) {
	case EDIT_NONE:
		g_edit_next = (g_edit_next + 1ULL) % MAX_EDITS; 
		break;
	case EDIT_PLACE:
		tp = &g_tm.rows[ed->place.y][ed->place.x];
		tile = *tp;
		*tp = ed->place.tile;
		ed->place.tile = tile;
		break;
	case EDIT_RESIZE:
		resize_edit(ed);
		break;
	}
}

/**
 * redo() - Redos edit 
 */
static void redo(void)
{
	edit *ed;
	uint8_t *tp;
	int tile;
	
	ed = g_edits + g_edit_next;
	g_edit_next = (g_edit_next + 1ULL) % MAX_EDITS;
	switch (ed->type) {
	case EDIT_NONE:
		g_edit_next = (g_edit_next - 1ULL) % MAX_EDITS;
		break;
	case EDIT_PLACE:
		tp = &g_tm.rows[ed->place.y][ed->place.x];
		tile = *tp;
		*tp = ed->place.tile;
		ed->place.tile = tile;
		break;
	case EDIT_RESIZE:
		resize_edit(ed);
		break;
	}
}

/**
 * update_place() - Update selected tile base on menu command
 * @id: ID of menu item, must be valid tile submenu ID 
 */
static void update_place(int id)
{
	CheckMenuItem(g_menu, g_place + 0x4000, MF_UNCHECKED);
	CheckMenuItem(g_menu, id, MF_CHECKED);
	g_place = id - 0x4000;
}

/**
 * push_edit() - Addes new edit
 * Return: New edit pointer
 */
static edit *push_edit(void)
{
	edit *ed, *br;

	ed = g_edits + g_edit_next;
	g_edit_next = (g_edit_next + 1ULL) % MAX_EDITS; 

	br = g_edits + g_edit_next;
	br->type = EDIT_NONE;

	return ed;
}

/**
 * attempt_resize() - Responds to OK button on resize dialog 
 * @wnd: Dialog window
 */
static void attempt_resize(HWND wnd)
{
	int err;
	BOOL success;
	int width;
	int height;

	err = 0;

	width = GetDlgItemInt(wnd, IDD_WIDTH, &success, FALSE);
	if (!success) {
		err = -1;
		MessageBoxW(wnd, L"Invalid width", 
				L"Error", MB_ICONERROR);
	} else if (width > 999) {
		err = -1;
		MessageBoxW(wnd, L"Width must be at most 999", 
				L"Error", MB_ICONERROR);
	} 

	height = GetDlgItemInt(wnd, IDD_HEIGHT, &success, FALSE);
	if (!success) {
		err = -1;
		MessageBoxW(wnd, L"Invalid height", 
				L"Error", MB_ICONERROR);
	} else if (height > 999) {
		err = -1;
		MessageBoxW(wnd, L"Height must be at most 999", 
				L"Error", MB_ICONERROR);
	}

	if (err >= 0) {
		edit *ed;

		ed = push_edit();
		ed->type = EDIT_RESIZE;
		ed->resize.w = g_tm.w;
		ed->resize.h = g_tm.h;

		size_tm(&g_tm, width, height);
		update_scrollbars(g_client_width, g_client_height);

		EndDialog(wnd, 0);
	}
}
		
/**
 * dlg_proc() - Callback for resize dialog 
 * @wnd: Modal dialog handle
 * @msg: Message to procces
 * @wp: Unsigned system word sized param 
 * @lp: Signed system word sized param 
 *
 * The meaning of wp and lp are message specific.
 *
 * Return: Result of message processing, usually zero if message was processed 
 */
static __stdcall INT_PTR dlg_proc(HWND wnd, UINT msg, 
		WPARAM wp, LPARAM lp)
{
		
	switch (msg) {
	case WM_INITDIALOG:
		SetDlgItemInt(wnd, IDD_WIDTH, g_tm.w, FALSE);
		SetDlgItemInt(wnd, IDD_HEIGHT, g_tm.h, FALSE);
		return TRUE;
	case WM_COMMAND:
		switch (wp) {
		case IDOK:
			attempt_resize(wnd);
			break;
		case IDCANCEL:
			EndDialog(wnd, 0);
			break;
		}
		return 0;
	}
	return 0;
}

/**
 * process_cmds() - Process menu commands
 */
static void process_cmds(int id)
{
	switch (id) {
	case IDM_NEW:
		if (unsaved_warning()) {
			g_map_path[0] = '\0';
			reset_edits();
			reset_tm(&g_tm);
			size_tm(&g_tm, 20, 15);
			g_cam.x = 0;
			g_cam.y = 0;
			update_scrollbars(g_client_width, g_client_height);
			g_change = false;
		}
		break;
	case IDM_OPEN:
		open();
		break;
	case IDM_SAVE:
		save();
		break;
	case IDM_SAVE_AS:
		save_as();
		break;
	case IDM_ZOOM_IN:
		if (g_cam.w > 2.0F) {
			g_cam.w /= 2.0F;
			g_cam.h /= 2.0F;
			update_scrollbars(g_client_width, g_client_height);
		}
		break;
	case IDM_ZOOM_OUT:
		if (g_cam.w < 160.0F) {
			g_cam.w *= 2.0F;
			g_cam.h *= 2.0F;
			update_scrollbars(g_client_width, g_client_height);
		}
		break;
	case IDM_ZOOM_DEF:
		g_cam.w = 20.0F;
		g_cam.h = 15.0F;
		update_scrollbars(g_client_width, g_client_height);
		break;
	case IDM_UNDO:
		undo();
		break;
	case IDM_REDO:
		redo();
		break;
	case IDM_GRID:
		if (g_grid_on) {
			g_grid_on = false;
			CheckMenuItem(g_menu, IDM_GRID, MF_UNCHECKED);
		} else {
			g_grid_on = true;
			CheckMenuItem(g_menu, IDM_GRID, MF_CHECKED);
		}
		break;
	case IDM_RESIZE:
            	DialogBoxParamW(NULL, MAKEINTRESOURCEW(ID_RESIZE), 
				g_wnd, dlg_proc, 0);
		break;
	default:
		if (id & 0x4000) {
			update_place(id);
		}
	}
}

/**
 * push_place_tile() - Adds tile edit to edit buffer 
 * @x: x position of tile
 * @y: y position of tile
 * @tile: tile to add
 */
static void push_place_tile(int x, int y, int tile)
{
	edit *ed;

	ed = push_edit();
	ed->type = EDIT_PLACE;
	ed->place.x = x;
	ed->place.y = y;
	ed->place.tile = tile;
}

/**
 * place_tile() - Place tile using cursor
 * @x: Client window x coord 
 * @y: Client window y coord 
 * @tile: Tile to place
 */
static void place_tile(int x, int y, int tile)
{
	int tx;
	int ty;
	uint8_t *tp;

	tx = g_cam.x + (float) x * g_cam.w / g_client_width;
	ty = g_cam.y + (float) y * g_cam.h / g_client_height;

	if (tx >= 0 && tx < g_tm.w && ty >= 0 && ty < g_tm.h) {
		tp = &g_tm.rows[ty][tx];
		if (*tp != tile) {
			push_place_tile(tx, ty, *tp);
			*tp = tile;
			g_change = true;
		}
	}
}

/**
 * button_down() - Respond to mouse button down 
 * @wp: WPARAM from wnd_proc
 * @lp: LPARAM from wnd_proc
 * @tile: Tile to place
 */
static void button_down(WPARAM wp, LPARAM lp, int tile)
{
	place_tile(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), tile);
}

/**
 * mouse_move() - Respond to mouse move
 * @lp: WPARAM from wnd_proc
 * @lp: LPARAM from wnd_proc
 */
static void mouse_move(WPARAM wp, LPARAM lp)
{
	int x;
	int y;

	x = GET_X_LPARAM(lp);
	y = GET_Y_LPARAM(lp);

	if (wp & MK_SHIFT) {
		if (wp & MK_LBUTTON) {
			place_tile(x, y, g_place);
		} else if (wp & MK_RBUTTON) {
			place_tile(x, y, 0);
		}
	}
}

/**
 * update_horz_scroll() - Update horizontal scroll position
 * @wp: WPARAM from wnd_proc  
 */
static void update_horz_scroll(WPARAM wp)
{
	if (LOWORD(wp) == SB_THUMBPOSITION) {
		SCROLLINFO si;

		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		si.nPos = HIWORD(wp); 
		SetScrollInfo(g_wnd, SB_HORZ, &si, TRUE);

		g_cam.x = si.nPos * g_cam.w / g_client_width;
	}
}

/**
 * update_vert_scroll() - Update vertical scroll position
 * @wp: WPARAM from wnd_proc  
 */
static void update_vert_scroll(WPARAM wp)
{
	if (LOWORD(wp) == SB_THUMBPOSITION) {
		SCROLLINFO si;

		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		si.nPos = HIWORD(wp); 
		SetScrollInfo(g_wnd, SB_VERT, &si, TRUE);

		g_cam.y = si.nPos * 15.0F / g_client_height;
	}
}

/**
 * wnd_proc() - Callback to prcoess window messages
 * @wnd: Handle to window, should be equal to g_wnd
 * @msg: Message to procces
 * @wp: Unsigned system word sized param 
 * @lp: Signed system word sized param 
 *
 * The meaning of wp and lp are message specific.
 *
 * Return: Result of message processing, usually zero if message was processed 
 */
static LRESULT __stdcall wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CLOSE:
		if (unsaved_warning()) {
			ExitProcess(0);
		}
		return 0;
	case WM_SIZE:
		update_size(LOWORD(lp), HIWORD(lp));
		return 0;
	case WM_COMMAND:
		process_cmds(LOWORD(wp));
		return 0;
	case WM_HSCROLL:
		update_horz_scroll(wp);
		return 0;
	case WM_VSCROLL:
		update_vert_scroll(wp);
		return 0;
	case WM_LBUTTONDOWN:
		button_down(wp, lp, g_place);
		return 0;
	case WM_RBUTTONDOWN:
		button_down(wp, lp, 0);
		return 0;
	case WM_MOUSEMOVE:
		mouse_move(wp, lp);
		return 0;
	}
	return DefWindowProcW(wnd, msg, wp, lp);
}

/**
 * create_main_window() - creates main window
 */
static void create_main_window(void)
{
	WNDCLASS wc;
	DWORD flags;
	RECT rect;
	int width;
	int height;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC; 
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = g_ins;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW); 
	wc.lpszClassName = L"WndClass"; 
	wc.lpszMenuName = MAKEINTRESOURCEW(ID_MENU);

	if (!RegisterClassW(&wc)) {
		MessageBoxW(NULL, L"Window registration failed", 
				L"Win32 Error", MB_ICONERROR);
		ExitProcess(1);
	}

	flags = WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL;
	rect.left = 0;
	rect.top = 0;
	rect.right = CLIENT_WIDTH;
	rect.bottom = CLIENT_HEIGHT;
	AdjustWindowRect(&rect, flags, TRUE);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	g_wnd = CreateWindowExW(0, wc.lpszClassName, L"Engine", flags, 
			CW_USEDEFAULT, CW_USEDEFAULT, width, height, 
			NULL, NULL, g_ins, NULL);
	if (!g_wnd) {
		MessageBoxW(NULL, L"Window creation failed", 
				L"Win32 Error", MB_ICONERROR);
		ExitProcess(1);
	}

	g_menu = GetMenu(g_wnd);
	g_acc = LoadAcceleratorsW(g_ins, MAKEINTRESOURCEW(ID_ACCELERATOR));
}

/**
 * msg_loop() - Main loop of program
 */
static void msg_loop(void)
{
	MSG msg; 

	ShowWindow(g_wnd, SW_SHOW);

	while (GetMessageW(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(g_wnd, g_acc, &msg)) {
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
		}
		
		render();
	}
}

/**
 * init_freetype() - Initializes FreeType library
 */
static void init_freetype()
{
	int error = FT_Init_FreeType(&g_freetype_library);

	if (error) {
		MessageBoxW(g_wnd, L"Could not Load FreeType Library", 
				L"Error", error);
	}
}

/**
 * wWinMain() - Entry point of program
 * @ins: Handle used to identify the executable
 * @prev: Always zero, hold over from Win16
 * @cmd: Command line arguments as a single string
 * @show: Flag that is used to identify the intial state of the main window 
 *
 * Return: Zero shall be returned on success, and one on faliure
 *
 * The use of ExitProcess is equivalent to returning from this function 
 */
int __stdcall wWinMain(HINSTANCE ins, HINSTANCE prev, wchar_t *cmd, int show) 
{
	UNREFERENCED_PARAMETER(prev);
	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(show);
	
	g_ins = ins;
	set_default_directory();
	create_main_window();
	init_gl();
	init_freetype();
	init_tm(&g_tm);
	size_tm(&g_tm, 20, 15);
	msg_loop();
	
	return 0;
}
