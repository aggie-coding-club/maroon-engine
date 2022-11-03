#include <math.h>

#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <windows.h>

#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>
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
#define LAYER_CLOUD 3
#define LAYER_BACK 4 

#define WGL_LOAD(func) func = (typeof(func)) wgl_load(#func)

#define MAX_SQUARES 1024 

/**
 * square - Render square 
 * @x: x-pos in camera pixels relative to left
 * @y: y-pos in camera pixels relative to top
 * @layer: layer of square 
 * @id: the id of the square from 0 to 255 
 */
struct square {
	int16_t x; 
	int16_t y; 
	uint8_t layer;
	uint8_t id;
};

/**
 * sprite - Set of squares corresponding to a single image
 * @base: Base square id
 * @count: Count of unique positions
 * @pts: Position of each square
 */
struct sprite {
	uint8_t base;
	uint8_t count;
	v2i pts[];
};

struct square_buf {
	int count;
	square squares[MAX_SQUARES];
};

HWND g_wnd;
HMENU g_menu;

v2 g_scroll;
bool g_grid_on = true;
bool g_running;

rect g_cam = {0, 0, VIEW_TW, VIEW_TH}; 

static int g_square_next;
static sprite *g_sprites[COUNTOF_SPR];

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
 * load_square() - Load rectangle
 * @dst: pointer to topleft of destination pixels 
 * @src: pointer to topleft of entire source (not offest by x and y)
 * @x: x-pos of src
 * @y: y-pos of src
 * @w: width of src
 * @h: height of src
 */
static void load_square(uint8_t *dst, const uint8_t *src, 
		int x, int y, int w, int h)
{
	int nx0, ny0;
	int xzero, yzero;
	int dskip, sskip;

	uint8_t *dp;
	const uint8_t *sp;
	int ny;

	/*don't grab more than present*/
	nx0 = min(w - x, TILE_LEN);
	ny0 = min(h - y, TILE_LEN);

	/*zero out of bounds*/
	xzero = TILE_STRIDE - nx0 * 4;
	yzero = TILE_STRIDE - ny0 * 4; 

	/*skip needed to get to next row*/
	dskip = ATLAS_STRIDE - nx0 * 4; 
	sskip = (w - nx0) * 4;

	/*copying begins, along with clear out of bounds columns*/
	dp = dst;
	sp = src;
	ny = ny0;
	while (ny-- > 0) {
		int nx;

		nx = nx0;
		while (nx-- > 0) {
			memcpy(dp, sp, 4);
			dp += 4;
			sp += 4;
		}
		memset(dp, 0, xzero);
		dp += dskip;
		sp += sskip; 
	}

	/*zero out out of bound rows*/
	ny = yzero; 
	while (ny-- > 0) {
		memset(dp, 0, TILE_STRIDE);
		dp += ATLAS_STRIDE;
	}
}

/**
 * load_sprite() - Load sprite
 * @dst: Destination to write sqaure
 * @src: Source of squares
 * @w: Width of source
 * @h: Height of soruce
 * @pspr: Pointer to sprite slot
 *
 * Return: Zero on succcess, negative on failure
 */
static int load_sprite(uint8_t **dst, const uint8_t *src, int w, int h, 
		sprite **pspr)
{
	int max;
	sprite *spr;

	const uint8_t *sp;
	int y;

	/*sprite preparation*/
	max = div_up(w, TILE_LEN) * div_up(h, TILE_LEN);
	spr = (sprite *) malloc(sizeof(*spr) + max * sizeof(*spr->pts));
	if (!spr) {
		return -1;
	}
	spr->base = g_square_next;
	spr->count = 0;
	*pspr = spr;

	/*find the active squares*/
	sp = src;
	for (y = 0; y < h; y += TILE_LEN) {
		int x;
		const uint8_t *c;

		/*search for opaque column*/
		c = sp;
		for (x = 0; x < w; x++) {
			const uint8_t *r;
			int ny;

			r = c;
			ny = min(h - y, TILE_LEN);
			while (ny-- > 0) {
				/*check to see if pixel is opaque*/
				if (r[3] == 0xFF) {
					v2i *pos;

					load_square(*dst, c, x, y, w, h);

					pos = spr->pts + spr->count++; 
					pos->x = x;
					pos->y = y;

					x += TILE_LEN - 1;
					c += TILE_STRIDE - 4; 
					g_square_next++;
					if (g_square_next % 16 == 0) {
						*dst += ATLAS_STRIDE * 
								(TILE_LEN - 1);
					}
					*dst += TILE_STRIDE;
					break;
				}
				r += w * 4;
			}
			c += 4;
		}
		sp += 4 * TILE_LEN * w;
	}
	/*shrink to fit*/
	spr = (sprite *) realloc(spr, sizeof(*spr) + 
			spr->count * sizeof(*spr->pts));
	if (spr) {
		*pspr = spr;
	}

	return 0;
}

/**
 * load_atlas() - Load sprites into atlas.
 *
 * Combines a bunch of individual sprites listed
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

	int spr_i;
	sprite **spr;

	dst = (uint8_t *) malloc(SIZEOF_ATLAS);
	if (!dst) {
		return;
	}
	
	dp = dst;
	spr = g_sprites;

	file = g_sprite_files; 
	n = COUNTOF_SPR; 
	while (n-- > 0) { 
		char path[MAX_PATH];

		uint8_t *src;
		int width;
		int height;

		sprintf(path, "res/sprites/%s", *file);
		src = stbi_load(path, &width, &height, NULL, 4);
		if (!src) {
			fprintf(stderr, "%s could not find\n", *file);
			break;
		}

		/*TODO: Make this work with sprites not 32x32*/
		add_menu_bitmap(src, spr_i);

		if (load_sprite(&dp, src, width, height, spr) < 0) {
			fprintf(stderr, "could not load sprite %s\n", *file);
			break;
		}
		stbi_image_free(src);

		file++;
		spr_i++;
		spr++;
	}
	stbi_write_png("res/tex/tex.png", ATLAS_LEN, 
			ATLAS_LEN, 4, dst, ATLAS_STRIDE);
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
			sizeof(square), (void *) 0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 
			sizeof(square), (void *) 4);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, 
			sizeof(square), (void *) 5);

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

	glBufferData(GL_ARRAY_BUFFER, MAX_SQUARES * sizeof(square), 
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
 * render_sprites() - Submit all sprites to GPU
 * @square_buf: Buffer of sprites
 */
static void render_sprites(square_buf *buf)
{
	glBindVertexArray(g_sprite_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_sprite_vbo);
	sprite_vaa_set_up();
	glBufferSubData(GL_ARRAY_BUFFER, 0, buf->count * sizeof(square), 
			buf->squares);
	glDrawArrays(GL_POINTS, 0, buf->count);
}

/**
 * push_sprite() - Add sprite to render
 * @buf: Buffer to add sprites to
 * @x: x-pos in tiles relative to left of screen
 * @y: y-pos in tiles relative to top of screen
 * @layer: layer of sprite from 0 to 255, higher layers on bottom 
 * @id: ID of sprite
 */
static void push_sprite(square_buf *buf, float x, float y, int layer, int id)
{
	int px0, py0;
	sprite *spr;
	v2i *pt;
	int i;

	px0 = x * TILE_LEN;
	py0 = y * TILE_LEN;

	spr = g_sprites[id];
	if (!spr) {
		return;
	}

	pt = spr->pts;
	for (i = 0; i < spr->count; i++) {
		int px;
		int py;
		bool xbound;
		bool ybound;

		px = px0 + pt->x;
		py = py0 + pt->y; 
		xbound = px > -TILE_LEN && px < g_cam.w * TILE_LEN + TILE_LEN;
		ybound = py > -TILE_LEN && py < g_cam.h * TILE_LEN + TILE_LEN;
		if (xbound && ybound) {
			square *s;

			s = buf->squares + buf->count;
			s->x = px;
			s->y = py;
			s->layer = layer;
			s->id = spr->base + i;
			buf->count++;
			if (buf->count == MAX_SQUARES) {
				render_sprites(buf);
				buf->count = 0;
			}
		}
		pt++;
	}
}

/**
 * render_tiles() - Render tiles and possibly grid
 * @buf: Sprite buffer to add to
 */
static void render_tiles(square_buf *buf)
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
				push_sprite(buf, stx, sty, 
						LAYER_FORE, tile - 2);
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
static void render_entities(square_buf *buf)
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
static square_buf *start_sprites(void)
{
	square_buf *buf;
	buf = (square_buf *) malloc(sizeof(*buf));
	if (buf) {
		buf->count = 0;
	}
	return buf;
}

/**
 * end_sprites() - Renders all sprites and destroys buffer
 */
static void end_sprites(square_buf *buf) 
{
	render_sprites(buf);
	free(buf);
}

/**
 * update_sprites() - Update sprites
 */
static void update_sprites(void)
{
	square_buf *buf;

	buf = start_sprites(); 
	if (!buf) {
		return;
	}

	render_tiles(buf);
	if (g_running) {
		static float w;

		push_sprite(buf, w, 0, LAYER_CLOUD, SPR_BIG_CLOUDS);
		push_sprite(buf, w + 14.0F, 0, LAYER_CLOUD, SPR_BIG_CLOUDS);
		w -= 1.0F * g_dt;
		if (w < -14.0F) {
			w += 14.0F;
		}
	}
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
