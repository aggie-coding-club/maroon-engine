#include <math.h>

#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <windows.h>

#include <glad/glad.h>
#include <stb_image.h>
#include <wglext.h>

#include "menu.hpp"
#include "render.hpp"
#include "tile_map.hpp"
#include "util.hpp"

#define TILE_STRIDE (TILE_LEN * 4) 
#define TILE_SIZE (TILE_LEN * TILE_LEN)
#define SIZEOF_TILE (TILE_STRIDE * TILE_LEN)

#define ATLAS_TILE_LEN 16 
#define ATLAS_TILE_SIZE (ATLAS_TILE_LEN * ATLAS_TILE_LEN)

#define ATLAS_LEN (ATLAS_TILE_LEN * TILE_LEN) 
#define ATLAS_STRIDE (ATLAS_TILE_LEN * TILE_STRIDE) 
#define SIZEOF_ATLAS (ATLAS_STRIDE * ATLAS_LEN) 

#define WGL_LOAD(func) func = (typeof(func)) wgl_load(#func)

#define MAX_SQUARES 1024 

/**
 * square - Render square 
 * @x: x-pos relative to left
 * @y: y-pos relative to top
 * @z: depth, z goes from -1 to 0, lower means on top 
 * @tile: the tile to render
 */
struct square {
	float x; 
	float y; 
	float z;
	uint32_t tile;
};

struct square_buf {
	int count;
	square squares[MAX_SQUARES];
};

HWND g_wnd;
HMENU g_menu;

v2 g_scroll;
bool g_grid_on = true;

rect g_cam = {0, 0, 20, 15}; 

static HDC g_hdc;
static HGLRC g_glrc;

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

static GLuint g_tex;

static GLuint g_square_prog;

static GLuint g_square_vao;
static GLuint g_square_vbo;

static GLint g_square_view_ul;
static GLint g_square_tex_ul;

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

static HBITMAP create_menu_icon(const uint8_t *src)
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
	
	dp = dst;
	src_n = 256;
	while (fscanf(f, "%260s", file) == 1) { 
		char path[MAX_PATH];

		uint8_t *src;
		int width;
		int height;

		HBITMAP hbm;

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

		/*set tile menu icon*/
		hbm = create_menu_icon(src); 
		SetMenuItemBitmaps(g_menu, IDM_BLANK + 256 - src_n, 
				MF_BYCOMMAND, hbm, NULL);

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
 * square_vaa_set_up() - Set up vertex attributes for square program
 */
static void square_vaa_set_up(void)
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
			sizeof(square), (void *) 0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 
			sizeof(square), (void *) 12);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

/**
 * create_square_prog() - Create square program
 * @gs: Geometry shader
 * @fs: Fragment shader
 *
 * Vertex shader is created on the spot.
 */
static void create_square_prog(GLuint gs, GLuint fs)
{
	GLuint vs;

	vs = compile_shader(GL_VERTEX_SHADER, L"square.vert");
	g_square_prog = create_prog(vs, gs, fs); 
	glDeleteShader(vs);

	glGenVertexArrays(1, &g_square_vao);
	glGenBuffers(1, &g_square_vbo);

	glBindVertexArray(g_square_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_square_vbo);
	square_vaa_set_up();

	glBufferData(GL_ARRAY_BUFFER, MAX_SQUARES * sizeof(square), 
			NULL, GL_DYNAMIC_DRAW);

	glUseProgram(g_square_prog);
	g_square_view_ul = glGetUniformLocation(g_square_prog, "view");
	g_square_tex_ul = glGetUniformLocation(g_square_prog, "tex");
	glUniform1i(g_square_tex_ul, 0);
}

/**
 * init_gl_progs() - Create OpenGL programs
 */
static void init_gl_progs(void)
{
	GLuint gs;
	GLuint fs;

	gs = compile_shader(GL_GEOMETRY_SHADER, L"square.geom");
	fs = compile_shader(GL_FRAGMENT_SHADER, L"square.frag");

	create_atlas();
	create_square_prog(gs, fs);

	glDeleteShader(fs);
	glDeleteShader(gs);
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	init_gl_progs();
}

static int min(int a, int b)
{
	return a < b ? a : b;
}

/**
 * render_squares() - Submit all squares to GPU
 * @square_buf: Buffer of squares
 */
static void render_squares(square_buf *buf)
{
	glBindVertexArray(g_square_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_square_vbo);
	square_vaa_set_up();
	glBufferSubData(GL_ARRAY_BUFFER, 0, buf->count * sizeof(square), 
			buf->squares);
	glDrawArrays(GL_POINTS, 0, buf->count);
}

/**
 * push_square() - Add square to render
 * @buf: Buffer to add squares to
 * @x: x-pos in tiles relative to left of screen
 * @y: y-pos in tiles relative to top of screen
 * @layer: layer of square from 0 to 255, higher layers on bottom 
 * 
 * NOTE: push_square should only be used in update_squares.
 */
static void push_square(square_buf *buf, float x, float y, int layer, int tile)
{
	square *s;
	if (buf->count == MAX_SQUARES) {
		render_squares(buf);
		buf->count = 0;
	}
	s = buf->squares + buf->count;
	s->x = x;
	s->y = y;
	s->z = layer / 256.0F;
	s->tile = tile;
	buf->count++;
}

/**
 * render_tiles() - Render tiles and possibly grid
 * @buf: Square buffer to add to
 */
static void render_tiles(square_buf *buf)
{
	int max_x;
	int max_y;
	int ty;

	max_x = min(g_cam.w + 1, g_tm.w);
	max_y = min(g_cam.h + 1, g_tm.h);

	for (ty = 0; ty < max_y; ty++) {
		int tx;
		for (tx = 0; tx < max_x; tx++) {
			int atx, aty;
			int tile;
			int stx, sty;

			atx = g_cam.x + tx;
			aty = g_cam.y + ty;
			tile = g_tm.rows[aty][atx];
	
			stx = tx - fmodf(g_cam.x, 1.0F);
			sty = ty - fmodf(g_cam.y, 1.0F);
			push_square(buf, stx, sty, 1, tile);
			if (g_grid_on) {
				push_square(buf, stx, sty, 0, 4);
			}
		}
	}
}

/**
 * start_squares() - Creates empty square buffer
 */
static square_buf *start_squares(void)
{
	square_buf *buf;
	buf = (square_buf *) malloc(sizeof(*buf));
	if (buf) {
		buf->count = 0;
	}
	return buf;
}

/**
 * end_squares() - Renders all squares and destroys buffer
 */
static void end_squares(square_buf *buf) 
{
	render_squares(buf);
	free(buf);
}

/**
 * update_squares() - Update squares
 *
 * NOTE: This is where rendering code goes, physics team.
 */
static void update_squares(void)
{
	square_buf *buf;

	buf = start_squares(); 
	if (!buf) {
		return;
	}

	render_tiles(buf);
	/*TODO: Insert rendering code here*/

	end_squares(buf);
}

void render(void)
{
	glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_tex);

	glUseProgram(g_square_prog);
	glUniform2f(g_square_view_ul, g_cam.w, g_cam.h);
	update_squares();
	SwapBuffers(g_hdc);
}
