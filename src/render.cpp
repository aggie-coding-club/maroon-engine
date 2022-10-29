#include <math.h>

#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <windows.h>

#include <glad/glad.h>
#include <stb_image.h>
#include <wglext.h>

#include "entity.hpp"
#include "render.hpp"
#include "game_map.hpp"

#define TILE_STRIDE (TILE_LEN * 4) 
#define TILE_SIZE (TILE_LEN * TILE_LEN)
#define SIZEOF_TILE (TILE_STRIDE * TILE_LEN)

#define ATLAS_TILE_LEN 16 
#define ATLAS_TILE_SIZE (ATLAS_TILE_LEN * ATLAS_TILE_LEN)

#define ATLAS_LEN (ATLAS_TILE_LEN * TILE_LEN) 
#define ATLAS_STRIDE (ATLAS_TILE_LEN * TILE_STRIDE) 
#define SIZEOF_ATLAS (ATLAS_STRIDE * ATLAS_LEN) 

#define LAYER_GRID 0
#define LAYER_ENTITY 1
#define LAYER_FORE 2
#define LAYER_BACK 3 

#define WGL_LOAD(func) func = (typeof(func)) wgl_load(#func)

#define MAX_SPRITES 1024 

/**
 * sprite - Render sprite 
 * @x: x-pos in camera pixels relative to left
 * @y: y-pos in camera pixels relative to top
 * @layer: layer of sprite 
 * @id: the id of the sprite from 0 to 255 
 */
struct sprite {
	int16_t x; 
	int16_t y; 
	uint8_t layer;
	uint8_t id;
};

struct sprite_buf {
	int count;
	sprite sprites[MAX_SPRITES];
};

HWND g_wnd;
HMENU g_menu;

v2 g_scroll;
bool g_grid_on = true;
bool g_running;

rect g_cam = {0, 0, VIEW_TW, VIEW_TH}; 

static const char *const g_sprite_files[COUNTOF_SPR] = {
	[SPR_GRASS] = "grass.png",
	[SPR_GROUND] = "ground.png",
	[SPR_SKY] = "sky.png",
	[SPR_WATER] = "water.png",
	[SPR_HORIZON_WATER] = "horizon-water.png",
	[SPR_SKY_HORIZON] = "sky-horizon.png",
	[SPR_GRID] = "grid.png"
};

static HDC g_hdc;
static HGLRC g_glrc;

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

static GLuint g_tex;

static GLuint g_sprite_prog;

static GLuint g_sprite_vao;
static GLuint g_sprite_vbo;

static GLint g_sprite_view_ul;
static GLint g_sprite_tex_ul;

/**
 * wgl_load() - load WGL extension function 
 * @name: name of function 
 *
 * Return: Valid function pointer
 *
 * Crashes if function not found
 */
static PROC wgl_load(const char *name) 
{
	PROC ret;

	ret = wglGetProcAddress(name);
	if (!ret) {
		wchar_t wname[64];
		wchar_t text[256];

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
				name, -1, wname, _countof(wname)); 
		_snwprintf(text, _countof(text), 
				L"Could not get proc: %s", wname);
		MessageBoxW(NULL, text, L"WGL Error", MB_ICONERROR);
		ExitProcess(1);
	}
	return ret;
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
		wprintf(L"%s", path);
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
 * avg_qcr() - Average 8-bit channels in a 32-bit color in a tile 
 * @src: Pointer to top-left channel 
 *
 * NOTE: src is expected to have TILE_STRIDE
 *
 * Return: The final color
 */
static int avg_qcr(const uint8_t *src)
{
	int tl, tr;
	int bl, br;

	tl = src[0] * src[0];
	tr = src[4] * src[4];
	bl = src[TILE_STRIDE] * src[TILE_STRIDE];
	br = src[TILE_STRIDE + 4 ] * src[TILE_STRIDE + 4];
	return sqrt((tl + tr + bl + br) / 4);
}

/**
 * create_menu_bitmap() - Creates menu bitmap from source data
 * @src: 32-bit RGBA source data
 */
static HBITMAP create_menu_bitmap(const uint8_t *src)
{
	uint8_t *dst;

	const uint8_t *sp;
	uint8_t *dp;
	int ny;

	HBITMAP hbm;

	dst = (uint8_t *) malloc(SIZEOF_TILE / 4); 
	if (!dst) {
		return NULL;
	}

	dp = dst;
	sp = src;
	ny = TILE_LEN / 2;
	while (ny-- > 0) {
		int nx;

		nx = TILE_LEN / 2;
		while (nx-- > 0) {
			dp[0] = avg_qcr(sp + 2);
			dp[1] = avg_qcr(sp + 1);
			dp[2] = avg_qcr(sp);
			dp[3] = avg_qcr(sp + 3);
			dp += 4;
			sp += 8;
		}
		sp += TILE_STRIDE;
	}

	hbm = CreateBitmap(TILE_LEN / 2, TILE_LEN / 2, 1, 32, dst);
	free(dst);

	return hbm;
}

/**
 * add_menu_bitmap() - Add menu bitmap
 * @src: 32-bit RGBA source data
 * @i: ID of sprite 
 */
static void add_menu_bitmap(const uint8_t *src, int id)
{
	HBITMAP hbm;
	int idm;

	id += 2;
	if (id >= COUNTOF_TILES) {
		return;
	}
	idm = g_tile_to_idm[id];
	if (idm == 0) {
		return;
	}
	hbm = create_menu_bitmap(src); 
	SetMenuItemBitmaps(g_menu, idm, 
			MF_BYCOMMAND, hbm, NULL);
}

/**
 * load_atlas() - Load images into atlas.
 *
 * Combines a bunch of individual images listed
 * in g_sprite_files into a single atlas,
 * start from the top left and going right
 * than down. 
 */
static void load_atlas(void)
{
	const char *const *file;
	size_t n;

	uint8_t *dst;
	uint8_t *dp;

	int i;

	dst = (uint8_t *) malloc(SIZEOF_ATLAS);
	if (!dst) {
		return;
	}
	
	dp = dst;
	i = 0;

	file = g_sprite_files; 
	n = COUNTOF_SPR; 
	while (n-- > 0) { 
		char path[MAX_PATH];

		uint8_t *src;
		int width;
		int height;

		uint8_t *sp;
		int row_n;

		/*sprites exceed what can fit in the atlas*/
		if (i >= 256) {
			fprintf(stderr, "too many sprites\n"); 
			break;
		}

		sprintf(path, "res/sprites/%s", *file); 
		src = stbi_load(path, &width, &height, NULL, 4);
		if (!src) {
			fprintf(stderr, "%s could not find\n", *file);
			break;
		}

		/*sprites are expected to be tile size*/
		if (width != TILE_LEN || height != TILE_LEN) {
			fprintf(stderr, "invalid dimensions\n");
			stbi_image_free(src);
			break;
		}	

		/*set tile menu bitmap*/
		add_menu_bitmap(src, i);

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
		file++;
		i++;
		if (i % 16) {
			dp -= ATLAS_STRIDE * 32;
		} else {
			dp -= ATLAS_STRIDE;
		}
		dp += TILE_STRIDE;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_LEN, 
			ATLAS_LEN, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst); 
	free(dst);
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
 * sprite_vaa_set_up() - Set up vertex attributes for sprite program
 */
static void sprite_vaa_set_up(void)
{
	glVertexAttribIPointer(0, 2, GL_SHORT, 
			sizeof(sprite), (void *) 0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 
			sizeof(sprite), (void *) 4);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, 
			sizeof(sprite), (void *) 5);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
}

/**
 * create_sprite_prog() - Create sprite program
 *
 * Vertex shader is created on the spot.
 */
static void create_sprite_prog(void)
{
	GLuint vs;
	GLuint gs;
	GLuint fs;

	vs = compile_shader(GL_VERTEX_SHADER, L"sprite.vert");
	gs = compile_shader(GL_GEOMETRY_SHADER, L"sprite.geom");
	fs = compile_shader(GL_FRAGMENT_SHADER, L"sprite.frag");

	g_sprite_prog = create_prog(vs, gs, fs); 
	glDeleteShader(fs);
	glDeleteShader(gs);
	glDeleteShader(vs);


	glGenVertexArrays(1, &g_sprite_vao);
	glGenBuffers(1, &g_sprite_vbo);

	glBindVertexArray(g_sprite_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_sprite_vbo);
	sprite_vaa_set_up();

	glBufferData(GL_ARRAY_BUFFER, MAX_SPRITES * sizeof(sprite), 
			NULL, GL_DYNAMIC_DRAW);

	glUseProgram(g_sprite_prog);
	g_sprite_view_ul = glGetUniformLocation(g_sprite_prog, "view");
	g_sprite_tex_ul = glGetUniformLocation(g_sprite_prog, "tex");
	glUniform1i(g_sprite_tex_ul, 0);
}

/**
 * init_gl_progs() - Create OpenGL programs
 */
static void init_gl_progs(void)
{
	create_atlas();
	create_sprite_prog();
}

void init_gl(void)
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

	init_gl_progs();
}

/**
 * min() - Minimum of two values
 * @a: Left value
 * @b: Right value 
 * 
 * Return: Return minimum of two values
 */
static int min(int a, int b)
{
	return a < b ? a : b;
}

/**
 * render_sprites() - Submit all sprites to GPU
 * @sprite_buf: Buffer of sprites
 */
static void render_sprites(sprite_buf *buf)
{
	glBindVertexArray(g_sprite_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_sprite_vbo);
	sprite_vaa_set_up();
	glBufferSubData(GL_ARRAY_BUFFER, 0, buf->count * sizeof(sprite), 
			buf->sprites);
	glDrawArrays(GL_POINTS, 0, buf->count);
}

/**
 * push_sprite() - Add sprite to render
 * @buf: Buffer to add sprites to
 * @x: x-pos in tiles relative to left of screen
 * @y: y-pos in tiles relative to top of screen
 * @layer: layer of sprite from 0 to 255, higher layers on bottom 
 * @id: ID of sprite
 * 
 * NOTE: push_sprite should only be used in update_sprites.
 */
static void push_sprite(sprite_buf *buf, float x, float y, int layer, int id)
{
	if (x > -1.0F && x < g_cam.w + 1.0F 
			&& y > -1.0F && y < g_cam.h + 1.0F) {
		sprite *s;

		s = buf->sprites + buf->count;
		s->x = x * 32;
		s->y = y * 32;
		s->layer = layer;
		s->id = id;
		buf->count++;
		if (buf->count == MAX_SPRITES) {
			render_sprites(buf);
			buf->count = 0;
		}
	}

}

/**
 * render_tiles() - Render tiles and possibly grid
 * @buf: Sprite buffer to add to
 */
static void render_tiles(sprite_buf *buf)
{
	int max_x;
	int max_y;
	int ty;

	max_x = min(g_cam.w + 1, g_gm->w);
	max_y = min(g_cam.h + 1, g_gm->h);

	for (ty = 0; ty < max_y; ty++) {
		int tx;
		for (tx = 0; tx < max_x; tx++) {
			static const uint8_t cols[] = {
				SPR_SKY,
				SPR_SKY,
				SPR_SKY,
				SPR_SKY_HORIZON,
				SPR_HORIZON_WATER,
				SPR_WATER
			};

			int sprite;
			int atx, aty;
			int tile;
			int stx, sty;

			atx = g_cam.x + tx;
			aty = g_cam.y + ty;
			tile = g_gm->rows[aty][atx];
	
			stx = tx - fmodf(g_cam.x, 1.0F);
			sty = ty - fmodf(g_cam.y, 1.0F);
			if (tile >= 2) { 
				push_sprite(buf, stx, sty, LAYER_FORE, tile - 2);
			} 

			sprite = cols[sty % _countof(cols)];
			push_sprite(buf, stx, sty - 0.25F, LAYER_BACK, sprite);
			if (!g_running && g_grid_on) {
				push_sprite(buf, stx, sty, LAYER_GRID, 
						SPR_GRID);
			}
		}
	}
}

/**
 * render_entites() - Renders entities 
 * @buf: Sprite buf to push entities to
 */
static void render_entities(sprite_buf *buf)
{
	entity *e;

	dl_for_each_entry(e, &g_entities, node) {
		float tx, ty;
		int sprite;

		tx = e->pos.x - g_cam.x;
		ty = e->pos.y - g_cam.y;
		sprite = e->meta->sprite;
		push_sprite(buf, tx, ty, LAYER_ENTITY, sprite);
	}
}

/**
 * start_sprites() - Creates empty sprite buffer
 */
static sprite_buf *start_sprites(void)
{
	sprite_buf *buf;
	buf = (sprite_buf *) malloc(sizeof(*buf));
	if (buf) {
		buf->count = 0;
	}
	return buf;
}

/**
 * end_sprites() - Renders all sprites and destroys buffer
 */
static void end_sprites(sprite_buf *buf) 
{
	render_sprites(buf);
	free(buf);
}

/**
 * update_sprites() - Update sprites
 *
 * NOTE: This is where rendering code goes, physics team.
 */
static void update_sprites(void)
{
	sprite_buf *buf;

	buf = start_sprites(); 
	if (!buf) {
		return;
	}

	render_tiles(buf);
	render_entities(buf);

	end_sprites(buf);
}

void render(void)
{
	glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_tex);

	glUseProgram(g_sprite_prog);
	glUniform2f(g_sprite_view_ul, g_cam.w, g_cam.h);
	update_sprites();
	SwapBuffers(g_hdc);
}
