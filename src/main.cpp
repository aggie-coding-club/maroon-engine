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
#include <stb_image.h>
#include <wglext.h>

#include "helpers.hpp"
#include "menu.hpp"

#define MAX_EDITS 256ULL
#define MAX_OBJS 1024ULL 

#define VIEW_WIDTH 20
#define VIEW_HEIGHT 15 

#define CLIENT_WIDTH 640
#define CLIENT_HEIGHT 480

#define TILE_LEN 32 
#define TILE_STRIDE (TILE_LEN * 4) 

#define ATLAS_TILE_LEN 16 
#define ATLAS_TILE_SIZE (ATLAS_TILE_LEN * ATLAS_TILE_LEN)

#define ATLAS_LEN (ATLAS_TILE_LEN * TILE_LEN) 
#define ATLAS_STRIDE (ATLAS_TILE_LEN * TILE_STRIDE) 
#define SIZEOF_ATLAS (ATLAS_STRIDE * ATLAS_LEN) 

enum edit_type {
	EDIT_NONE,
	EDIT_PLACE_TILE
};

enum tile_map {
	TM_FORE,
	TM_GRID
};

struct v2 {
	float x;
	float y;
};

struct edit {
	edit_type type;
	int x;
	int y;
	int tile;
};

struct object {
	_Float16 x; 
	_Float16 y; 
	uint8_t tile; 
	uint8_t flags;
};

static HINSTANCE g_ins;
static HWND g_wnd;
static HMENU g_menu;
static HACCEL g_acc; 
static HDC g_hdc;
static HGLRC g_glrc;

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

static GLuint g_tm_prog;

static GLuint g_tm_vao;
static GLuint g_tm_vbo[2];

static GLint g_tm_scroll_ul; 
static GLint g_tm_layer_ul;
static GLint g_tm_tex_ul; 

static GLuint g_tex;

static GLuint g_obj_prog;

static GLuint g_obj_vao;
static GLuint g_obj_vbo;

static GLint g_obj_tex_ul;

static uint8_t g_tile_map[32][32];

static v2 g_scroll;

static int g_client_width = CLIENT_WIDTH;
static int g_client_height = CLIENT_HEIGHT;

static wchar_t g_map_path[MAX_PATH];

static bool g_change;

static edit g_edits[MAX_EDITS];
static size_t g_edit_next;

static object g_objects[MAX_OBJS];
static size_t g_object_count;

static int g_place = 1;
static bool g_grid_on = true;

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
 * update_size() - Called to acknowledge changes in client area size
 * @width: New client width
 * @height: New client height
 */
static void update_size(int width, int height)
{
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
	return !g_change || MessageBoxW(g_wnd, L"Unsaved changes will be lost", 
			L"Warning", MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK;
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
	HANDLE fh;
	DWORD write;

	fh = CreateFileW(path, GENERIC_WRITE, 0, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
	if (fh == INVALID_HANDLE_VALUE) {
		goto err;
	}
	
	WriteFile(fh, g_tile_map, sizeof(g_tile_map), &write, NULL); 
	CloseHandle(fh);

	if (write < sizeof(g_tile_map)) {
		goto err;
	}

	g_change = false;
	return 0;
err:
	MessageBoxW(g_wnd, L"Could not save map", L"Error", MB_ICONERROR);
	return -1;
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
	HANDLE fh;
	DWORD read;
	uint8_t tmp[1024];

	fh = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
	if (fh == INVALID_HANDLE_VALUE) {
		goto err;
	}
	
	ReadFile(fh, tmp, sizeof(tmp), &read, NULL); 
	CloseHandle(fh);
	if (read < sizeof(g_tile_map)) {
		goto err;
	}

	memcpy(g_tile_map, tmp, sizeof(g_tile_map));
	return 0;
err:
	MessageBoxW(g_wnd, L"Could not read map", L"Error", MB_ICONERROR);
	return -1;
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
	int tile;
	
	g_edit_next = (g_edit_next - 1ULL) % MAX_EDITS;
	ed = g_edits + g_edit_next;
	switch (ed->type) {
	case EDIT_NONE:
		g_edit_next = (g_edit_next + 1ULL) % MAX_EDITS; 
		break;
	case EDIT_PLACE_TILE:
		tile = g_tile_map[ed->y][ed->x]; 
		g_tile_map[ed->y][ed->x] = ed->tile;
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
	int tile;
	
	ed = g_edits + g_edit_next;
	g_edit_next = (g_edit_next + 1ULL) % MAX_EDITS;
	switch (ed->type) {
	case EDIT_NONE:
		g_edit_next = (g_edit_next - 1ULL) % MAX_EDITS;
		break;
	case EDIT_PLACE_TILE:
		tile = g_tile_map[ed->y][ed->x]; 
		g_tile_map[ed->y][ed->x] = ed->tile;
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
			memset(g_tile_map, 0, sizeof(g_tile_map));
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
	x = x * VIEW_WIDTH / g_client_width;
	y = y * VIEW_HEIGHT / g_client_height;

	if (g_tile_map[y][x] != tile) {
		push_place_tile(x, y, g_tile_map[y][x]);
		g_tile_map[y][x] = tile;
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

	rect.left = 0;
	rect.top = 0;
	rect.right = CLIENT_WIDTH;
	rect.bottom = CLIENT_HEIGHT;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	g_wnd = CreateWindowExW(0, wc.lpszClassName, L"Engine", 
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 
			CW_USEDEFAULT, width, height, 
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
 * init_gl() - initialize OpenGL context and load necessary extensions 
 */
static void init_gl(void)
{
	static const int attrib_list[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 
		3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 
		3,
		WGL_CONTEXT_FLAGS_ARB, 
		0,
		WGL_CONTEXT_PROFILE_MASK_ARB, 
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 
		0 
	};

	PIXELFORMATDESCRIPTOR pfd;
	int fmt;
	HGLRC tmp;

	g_hdc = GetDC(g_wnd);

	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER |  
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	fmt = ChoosePixelFormat(g_hdc, &pfd);
	SetPixelFormat(g_hdc, fmt, &pfd);

	tmp = wglCreateContext(g_hdc);
	wglMakeCurrent(g_hdc, tmp);

	WGL_LOAD(wglCreateContextAttribsARB);
	g_glrc = wglCreateContextAttribsARB(g_hdc, 0, attrib_list);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(tmp);
	wglMakeCurrent(g_hdc, g_glrc);

	WGL_LOAD(wglSwapIntervalEXT);
	wglSwapIntervalEXT(1);

	gladLoadGL();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/*
 * get_error_text() - Turn last Win32 error into a string. 
 * @buf: Buffer for characters
 * @size: Size of buffer  
 * 
 * Return: The length of string in buffer. 
 *
 * Zero is returned if "size" is zero.
 * If size is nonzero and their is an error, the string
 * in the buffer will be an empty string. 
 */
static DWORD get_error_text(wchar_t *buf, DWORD size)
{
	DWORD flags;
	DWORD err;
	DWORD lang;
	DWORD ret;

	flags = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;
	err = GetLastError();
	lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	ret = FormatMessageW(flags, NULL, err, lang, buf, size, NULL); 
	if (!ret && size) {
		buf[0] = L'\0';
	}
	return ret;	
}

/**
 * fatal_win32_error() - Display message box with Win32 error and exit.
 *
 * This function is used if a Win32 function fails with
 * a unrecoverable error.
 */
static void fatal_win32_err(void)
{
	wchar_t buf[1024];

	get_error_text(buf, _countof(buf));
	MessageBoxW(g_wnd, buf, L"Win32 Fatal Error", MB_OK);
	ExitProcess(1);
}

/**
 * read_all_str() - Reads entire file as a narrow string. 
 *
 * Exit with message box on failure.
 *
 * Return: The narrow string, destroy with "free" function 
 */
static char *read_all_str(const wchar_t *path)
{
	HANDLE fh;
	DWORD size;
	DWORD read;
	char *buf;

	fh = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
	if (fh == INVALID_HANDLE_VALUE) {
		fatal_win32_err();
	}

	size = GetFileSize(fh, NULL); 
	if (size == INVALID_FILE_SIZE) {
		fatal_win32_err();
	}

	buf = (char *) xmalloc(size + 1);

	ReadFile(fh, buf, size, &read, NULL);
	if (read < size) {
		fatal_win32_err();
	}
	buf[size] = '\0';

	CloseHandle(fh);
	return buf;
}

/**
 * prog_print() - Print OpenGL program log to standard error
 * @msg: A string to prepend the error  
 * @prog: The program 
 */
static void prog_print(const char *msg, GLuint prog)
{
	char err[1024];

	glGetProgramInfoLog(prog, sizeof(err), NULL, err);
	if (glGetError()) {
		fprintf(stderr, "gl error: %s\n", msg);
	} else {
		fprintf(stderr, "gl error: %s: %s\n", msg, err);
	}
}

/**
 * prog_printf() - Formatted equavilent of "prog_print" 
 * @fmt: The format string for the message to prepend the error
 * @prog: The program
 * @...: Format arguments
 */
__attribute__((format(printf, 1, 3)))
static void prog_printf(const char *fmt, GLuint prog, ...)
{
	va_list ap;
	char msg[1024];

	va_start(ap, prog);
	vsprintf_s(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	prog_print(msg, prog);
}

/**
 * to_utf8() - Convert UTF-16 to UTF-8 string
 * @dst: Buffer for UTF8 string
 * @src: UTF16 string to convert
 * @size: Size of "dst" buffer 
 *
 * Exits on failure to convert or if the "size" is zero 
 */
static void to_utf8(char *dst, const wchar_t *src, size_t size)
{
	if (!WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, size, NULL, NULL)) {
		if (size == 0) {
			SetLastError(ERROR_INVALID_PARAMETER);
		}
		fatal_win32_err();
	}
}	

/**
 * compile_shader() - Compile shader
 * @type: Type of shader (ex. GL_VERTEX_SHADER)
 * @path: path of shader (relative to res/shaders)
 *
 * Return: The shader 
 */
static GLuint compile_shader(GLuint type, const wchar_t *path)
{
	wchar_t wpath[MAX_PATH];
	char *src;
	GLuint shader;
	int success;

	_snwprintf(wpath, _countof(wpath), L"res/shaders/%s", path); 
	src = read_all_str(wpath);
	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const char **) &src, NULL);
	free(src);

	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char buf[MAX_PATH];
		to_utf8(buf, path, _countof(buf));
		prog_printf("\"%s\" compilation failed", shader, buf); 
	}

	return shader;
}

/**
 * create_prog() - Create OpenGL program
 * @vs_path: Vertex shader
 * @gs_path: Geometry shader
 * @fs_path: Fragment shader
 *
 * Return: The program
 */
static GLuint create_prog(GLuint vs, GLuint gs, GLuint fs) 
{
	GLuint prog;
	int success;

	prog = glCreateProgram();

	glAttachShader(prog, vs);
	glAttachShader(prog, gs);
	glAttachShader(prog, fs);

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		prog_print("program link failed", prog);
	}

	glDetachShader(prog, fs);
	glDetachShader(prog, gs);
	glDetachShader(prog, vs);

	return prog;
}

/**
 * load_atlas() - Load images into atlas.
 *
 * Combines a bunch of individual images listed
 * in res/images/list.h into a singel atlas,
 * start from the top left and going right
 * than down. 
 */
static void load_atlas(void)
{
	FILE *f;
	uint8_t *dst;
	uint8_t *dp;
	int src_n;
	char file[MAX_PATH - 32];

	/*list needed to allow for specific order of images*/
	f = _wfopen(L"res/images/list.txt", L"r");
	if (!f) {
		fprintf(stderr, "Cannot load image list\n");
		return;
	}

	dst = (uint8_t *) malloc(SIZEOF_ATLAS);
	if (!dst) {
		goto err0;
	}
	
	src_n = 256;
	dp = dst;
	while (fscanf(f, "%260s", file) == 1) { 
		char path[MAX_PATH];

		uint8_t *src;
		int width;
		int height;

		uint8_t *sp;
		int row_n;

		/*images exceed what can fit in the atlas*/
		if (src_n <= 0) {
			fprintf(stderr, "too many images\n"); 
			goto err1;
		}

		sprintf(path, "res/images/%s", file); 
		src = stbi_load(path, &width, &height, NULL, 4);
		if (!src) {
			fprintf(stderr, "%s could not find\n", file);
			goto err1;
		}

		/*images are expected to be tile size*/
		if (width != TILE_LEN || height != TILE_LEN) {
			fprintf(stderr, "invalid dimensions\n");
			stbi_image_free(src);
			goto err1;
		}	

		/*copy a single image into atlas*/
		sp = src;
		row_n = 32;
		while (row_n-- > 0) {
			memcpy(dp, sp, TILE_STRIDE); 
			dp += ATLAS_STRIDE;
			sp += TILE_STRIDE;
		}
		stbi_image_free(src);

		/**
		 * At this point "dp" points to first pixel
		 * of a tile a row after the one copied. 
		 *
		 * If the last tile was at the end of the tile row, 
		 * than the next tile is on this same row,
		 * but all the way to the left.
		 *
		 * Elsewise, the next tile, is immediatly to
		 * the right. 
		 */
		src_n--;
		if (src_n % 16) {
			dp -= ATLAS_STRIDE * 32;
		} else {
			dp -= ATLAS_STRIDE;
		}
		dp += TILE_STRIDE;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_LEN, 
			ATLAS_LEN, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst); 
err1:
	free(dst);
err0:
	fclose(f);
}

/**
 * create_atlas() - Create texture atlas 
 */
static void create_atlas(void)
{
	glGenTextures(1, &g_tex);
	glBindTexture(GL_TEXTURE_2D, g_tex);

  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	load_atlas();
}

/**
 * create_tm_prog() - Creates program to render background map 
 */
static void create_tm_prog(GLuint gs, GLuint fs)
{
	GLuint vs;
	uint8_t grid[32][32];

	vs = compile_shader(GL_VERTEX_SHADER, L"tm.vert");
	g_tm_prog = create_prog(vs, gs, fs);
	glDeleteShader(vs);

	glGenVertexArrays(1, &g_tm_vao);
	glGenBuffers(2, g_tm_vbo);
	
	glBindVertexArray(g_tm_vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_tm_vbo[TM_FORE]);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, 1, NULL);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_tile_map), 
			g_tile_map, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, g_tm_vbo[TM_GRID]);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, 1, NULL);
	glEnableVertexAttribArray(0);
	memset(grid, 4, sizeof(grid));
	glBufferData(GL_ARRAY_BUFFER, sizeof(grid), 
			grid, GL_STATIC_DRAW);

	glUseProgram(g_tm_prog);
	g_tm_scroll_ul = glGetUniformLocation(g_tm_prog, "scroll");
	g_tm_layer_ul = glGetUniformLocation(g_tm_prog, "layer");
	g_tm_tex_ul = glGetUniformLocation(g_tm_prog, "tex");

	glUniform1i(g_tm_tex_ul, 0);
}

/**
 * obj_vaa_set_up() - Set up vertex attributes for object program
 */
static void obj_vaa_set_up(void)
{
	glVertexAttribPointer(0, 2, GL_HALF_FLOAT, GL_FALSE, 
			sizeof(object), (void *) 0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 
			sizeof(object), (void *) 4);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, 
			sizeof(object), (void *) 5);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glEnableVertexAttribArray(2);
}

/**
 * create_obj_prog() - Create object program
 * @gs: Geometry shader
 * @fs: Fragment shader
 *
 * Vertex shader is created on the spot.
 */
static void create_obj_prog(GLuint gs, GLuint fs)
{
	GLuint vs;

	vs = compile_shader(GL_VERTEX_SHADER, L"obj.vert");
	g_obj_prog = create_prog(vs, gs, fs); 
	glDeleteShader(vs);

	glGenVertexArrays(1, &g_obj_vao);
	glGenBuffers(1, &g_obj_vbo);

	glBindVertexArray(g_obj_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_obj_vbo);
	obj_vaa_set_up();

	glBufferData(GL_ARRAY_BUFFER, sizeof(g_objects), 
			g_objects, GL_DYNAMIC_DRAW);

	glUseProgram(g_obj_prog);
	g_obj_tex_ul = glGetUniformLocation(g_obj_prog, "tex");
	glUniform1i(g_obj_tex_ul, 0);
}

/**
 * init_gl_progs() - Create OpenGL programs
 */
static void init_gl_progs(void)
{
	GLuint gs;
	GLuint fs;

	gs = compile_shader(GL_GEOMETRY_SHADER, L"tile.geom");
	fs = compile_shader(GL_FRAGMENT_SHADER, L"tile.frag");

	create_atlas();
	create_tm_prog(gs, fs);
	create_obj_prog(gs, fs);

	glDeleteShader(fs);
	glDeleteShader(gs);
}

/**
 * render() - Render tile map
 */
static void render(void)
{
	glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_tex);

	glUseProgram(g_tm_prog);
	glBindVertexArray(g_tm_vao);
	glUniform2f(g_tm_scroll_ul, g_scroll.x, g_scroll.y);

	glUniform1i(g_tm_layer_ul, -1);
	glBindBuffer(GL_ARRAY_BUFFER, g_tm_vbo[TM_FORE]);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, 1, NULL);
	glEnableVertexAttribArray(0);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(g_tile_map), g_tile_map);
        glDrawArrays(GL_POINTS, 0, sizeof(g_tile_map));

	if (g_grid_on) {
		glUniform1i(g_tm_layer_ul, 1);
		glBindBuffer(GL_ARRAY_BUFFER, g_tm_vbo[TM_GRID]);
		glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, 1, NULL);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_POINTS, 0, sizeof(g_tile_map));
	}

	glUseProgram(g_obj_prog);
	glBindVertexArray(g_obj_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_obj_vbo);
	obj_vaa_set_up();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(g_objects), g_objects);
	glDrawArrays(GL_POINTS, 0, g_object_count);
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
		SwapBuffers(g_hdc);
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
	init_gl_progs();
	msg_loop();
	
	return 0;
}
