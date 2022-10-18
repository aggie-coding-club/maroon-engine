#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <windows.h>

#include <glad/glad.h>
#include <stb_image.h>
#include <wglext.h>

#include "render.hpp"

#define TILE_STRIDE (TILE_LEN * 4) 

#define ATLAS_TILE_LEN 16 
#define ATLAS_TILE_SIZE (ATLAS_TILE_LEN * ATLAS_TILE_LEN)

#define ATLAS_LEN (ATLAS_TILE_LEN * TILE_LEN) 
#define ATLAS_STRIDE (ATLAS_TILE_LEN * TILE_STRIDE) 
#define SIZEOF_ATLAS (ATLAS_STRIDE * ATLAS_LEN) 

#define WGL_LOAD(func) func = (typeof(func)) wgl_load(#func)

enum tile_map {
	TM_FORE,
	TM_GRID
};

HWND g_wnd;

uint8_t g_tile_map[32][32];
v2 g_scroll;
bool g_grid_on = true;

object g_objects[MAX_OBJS];
size_t g_object_count;

static HDC g_hdc;
static HGLRC g_glrc;

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

static GLuint g_tm_prog;

static GLuint g_tm_vao;
static GLuint g_tm_vbo;

static GLint g_tm_scroll_ul; 
static GLint g_tm_tex_ul; 

static GLuint g_tex;

static GLuint g_obj_prog;

static GLuint g_obj_vao;
static GLuint g_obj_vbo;

static GLint g_obj_tex_ul;

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
 * fatal_crt_error() - Display message box with CRT error and exit 
 *
 * This function is used if a C-Runtime (CRT) function fails with
 * a unrecoverable error. 
 */
static void fatal_crt_err(void)
{
	wchar_t buf[1024];

	_wcserror_s(buf, _countof(buf), errno);
	MessageBoxW(g_wnd, buf, L"CRT Fatal Error", MB_OK); 
	ExitProcess(1);
}

/**
 * xmalloc() - Alloc memory, crash on failure
 * @size: Size of allocation
 *
 * Return: Pointer to allocation 
 *
 * Pass to "free" to deallocate.
 */
static void *xmalloc(size_t size)
{
	void *ptr;

	if (size == 0) {
		size = 1;
	}

	ptr = (char *) malloc(size);
	if (!ptr) {
		fatal_crt_err();
	}

	return ptr;
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
	uint8_t maps[2][32][32];

	vs = compile_shader(GL_VERTEX_SHADER, L"tm.vert");
	g_tm_prog = create_prog(vs, gs, fs);
	glDeleteShader(vs);

	glGenVertexArrays(1, &g_tm_vao);
	glGenBuffers(1, &g_tm_vbo);
	
	glBindVertexArray(g_tm_vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_tm_vbo);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, 1, NULL);
	glEnableVertexAttribArray(0);
	memset(maps[TM_FORE], 0, sizeof(maps[TM_FORE]));
	memset(maps[TM_GRID], 4, sizeof(maps[TM_GRID]));
	glBufferData(GL_ARRAY_BUFFER, sizeof(maps), 
			maps, GL_STATIC_DRAW);

	glUseProgram(g_tm_prog);
	g_tm_scroll_ul = glGetUniformLocation(g_tm_prog, "scroll");
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

void render(void)
{
	glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_tex);

	glUseProgram(g_tm_prog);
	glBindVertexArray(g_tm_vao);
	glUniform2f(g_tm_scroll_ul, g_scroll.x, g_scroll.y);

	glBindBuffer(GL_ARRAY_BUFFER, g_tm_vbo);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, 1, NULL);
	glEnableVertexAttribArray(0);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(g_tile_map), g_tile_map);
        glDrawArrays(GL_POINTS, 0, (g_grid_on ? 2 : 1) * sizeof(g_tile_map));

	glUseProgram(g_obj_prog);
	glBindVertexArray(g_obj_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_obj_vbo);
	obj_vaa_set_up();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(g_objects), g_objects);
	glDrawArrays(GL_POINTS, 0, g_object_count);
	SwapBuffers(g_hdc);
}