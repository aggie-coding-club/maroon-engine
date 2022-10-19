#include <assert.h>
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

#include "chunk.hpp"
#include "menu.hpp"
#include "render.hpp"

#define MAX_EDITS 256ULL

#define VIEW_WIDTH 20
#define VIEW_HEIGHT 15 

#define CLIENT_WIDTH 640
#define CLIENT_HEIGHT 480

enum edit_type {
	EDIT_NONE,
	EDIT_PLACE_TILE
};

struct edit {
	edit_type type;
	int x;
	int y;
	int tile;
};

static HINSTANCE g_ins;
static HMENU g_menu;
static HACCEL g_acc; 

static int g_client_width = CLIENT_WIDTH;
static int g_client_height = CLIENT_HEIGHT;

static wchar_t g_map_path[MAX_PATH];

static bool g_change;

static edit g_edits[MAX_EDITS];
static size_t g_edit_next;

static int g_place = 1;
static v2 g_cam; 

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
 * update_scrollbars() - Refresh scrollbar 
 * @width: The new width of the window
 * @height: The new height of the window
 */
static void update_scrollbars(int width, int height)
{
	SCROLLINFO si;

	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = g_chunk_map->tw * width / 20;
	si.nPage = width + 1;
	si.nPos = g_cam.x * width / 20;
	SetScrollInfo(g_wnd, SB_HORZ, &si, TRUE);

	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = g_chunk_map->th * height / 15;
	si.nPage = height + 1;
	si.nPos = g_cam.y * height / 15;
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
 * write_chunk() - Write chunk data out
 * @f: File handle
 * @c: Chunk
 *
 * Return: Returns 0 on success, -1 on failure
 */
static int write_chunk(FILE *f, chunk *c)
{
	uint8_t v[2];

	if (!c) {
		return 0;
	}

	/*get position of chunk*/
	v[0] = c->cx;
	v[1] = c->cy;
	if (fwrite(v, sizeof(v), 1, f) < 1) {
		return -1;
	}

	/*get chunk data*/
	if (fwrite(c->tiles, sizeof(c->tiles), 1, f) < 1) {
		return -1;
	}

	return 0;
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
	uint8_t v[2];
	chunk_row *cr;
	int ny;

	err = -1;
	f = _wfopen(path, L"wb");
	if (!f) {
		goto err0;
	}

	/*write width and height of map*/
	v[0] = g_chunk_map->tw;
	v[1] = g_chunk_map->th;
	if (fwrite(v, sizeof(v), 1, f) < 1) {
		goto err1;
	}

	/*write chunks*/
	cr = g_chunk_map->chunks;
	ny = g_chunk_map->ch;
	while (ny-- > 0) {
		chunk **cp;
		int nx;

		cp = *cr;
		nx = g_chunk_map->cw; 
		while (nx-- > 0) {
			if (write_chunk(f, *cp) < 0) {
				goto err1;
			}
			cp++;
		}
		cr++;
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
 * read_chunk(): Reads chunk and adds it to map
 * @f: File to read from
 * @map: Map to read into
 *
 * NOTE: Garbage chunk may be present if read fail
 * This function's only purpose is as helper function 
 *
 * Return: Returns -1 on failure, 0 on success, and 1 on completion 
 */
static int read_chunk(FILE *f, chunk_map *map)
{
	chunk *c;
	uint8_t v[2];

	/*get chunk position*/
	if (fread(&v, sizeof(v), 1, f) < 1) {
		/*if last chunk was alread read than EOF here*/
		if (feof(f)) {
			return 1;
		} else {
			return -1;
		}
	}

	/*check to see if chunk position is valid*/
	if (v[0] >= map->cw || v[1] >= map->ch) {
		return -1;
	}

	/*read in new chunk*/
	c = create_chunk(v[0], v[1]);
	map->chunks[v[1]][v[0]] = c;
	if (fread(c->tiles, sizeof(c->tiles), 1, f) < 1) { 
		return -1;
	}

	return 0;
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
	uint8_t v[2];
	chunk_map *map;
	int res;

	err = -1;
	f = _wfopen(path, L"rb");
	if (!f) {
		goto err0;
	}

	/*read width and height of map*/
	if (fread(&v, sizeof(v), 1, f) < 1) {
		goto err1;
	}
	if (v[0] >= MAP_LEN || v[1] >= MAP_LEN) {
		goto err1;
	}
	
	/*read chunks*/
	map = create_chunk_map(v[0], v[1]);
	while (!(res = read_chunk(f, map)));
	if (res < 0) {
		destroy_chunk_map(map);
		goto err1;
	}

	/*swap out old chunk map with new one*/
	destroy_chunk_map(g_chunk_map);
	g_chunk_map = map;

	/*error handling and cleanup*/
	err = 0;
err1:
	fclose(f);
	if (err >= 0) {
		g_change = false;
		update_scrollbars(g_client_width, g_client_height);
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
	case EDIT_PLACE_TILE:
		tp = touch_tile(ed->x, ed->y);
		tile = *tp;
		*tp = ed->tile;
		ed->tile = tile;
		break;
	default:
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
	case EDIT_PLACE_TILE:
		tp = touch_tile(ed->x, ed->y);
		tile = *tp;
		*tp = ed->tile;
		ed->tile = tile;
		break;
	default:
		break;
	}
}

/**
 * update_place() - Update selected tile base on menu command
 * @id: ID of menu item, must be valid tile submenu ID 
 */
static void update_place(int id)
{
	CheckMenuItem(g_menu, g_place + 0x3000, MF_UNCHECKED);
	CheckMenuItem(g_menu, id, MF_CHECKED);
	g_place = id - 0x3000;
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
			clear_chunk_map(g_chunk_map);
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
	default:
		if (id & 0x3000) {
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

	ed = g_edits + g_edit_next;
	ed->type = EDIT_PLACE_TILE;
	ed->x = x;
	ed->y = y;
	ed->tile = tile;
	g_edit_next = (g_edit_next + 1ULL) % MAX_EDITS; 
		
	/*needed to establish barrier between undo and redo*/
	ed = g_edits + g_edit_next;
	ed->type = EDIT_NONE;
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

	tx = g_cam.x + (float) x * VIEW_WIDTH / g_client_width;
	ty = g_cam.y + (float) y * VIEW_HEIGHT / g_client_height;

	tp = touch_tile(tx, ty);
	if (*tp != tile) {
		push_place_tile(tx, ty, *tp);
		*tp = tile;
		g_change = true;
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

		g_cam.x = si.nPos * 20.0F / g_client_width;
		g_scroll.x = -g_cam.x;
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
		g_scroll.y = -g_cam.y;
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
 * chunk_to_tile_map() - Move chunk to tile map
 * @off_cx: 0/1 indicates left and right quarter respectivily 
 * @off_cy: 0/1 indicates top and bottom quarter respectivily 
 */
static void chunk_to_tile_map(int off_cx, int off_cy)
{
	int stx, sty;
	chunk *c;
	int ty;

	stx = off_cx << 4; 
	sty = off_cy << 4; 
	c = touch_chunk(g_cam.x + stx, g_cam.y + sty);
	for (ty = 0; ty < CHUNK_MAP_LEN; ty++) {
		uint8_t *dp, *sp;
		dp = &g_tile_map[ty + sty][stx];
		sp = c->tiles[ty];
		memcpy(dp, sp, CHUNK_LEN);
	}
}

/**
 * place_chunks() - Update tile map to contain active chunks
 */
static void place_chunks(void)
{
	chunk_to_tile_map(0, 0);
	chunk_to_tile_map(1, 0);
	chunk_to_tile_map(0, 1);
	chunk_to_tile_map(1, 1);
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
		
		place_chunks();
		render();
	}
}

/**
 * wWinMain() - Entry point of program
 * @ins: Handle used to identify the executable
 * @prev: always zero, hold over from Win16
 * @cmd: command line arguments as a single string
 * @show: flag that is used to identify the intial state of the main window 
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
	g_chunk_map = create_chunk_map(20, 15);
	msg_loop();
	
	return 0;
}
