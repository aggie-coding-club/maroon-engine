#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <windows.h>

#include <glad/glad.h>
#include <wglext.h>

#include "toolkit.h"

#define CLIENT_WIDTH 640
#define CLIENT_HEIGHT 480

#define WGL_LOAD(func) func = (typeof(func)) wgl_load(#func)

static HINSTANCE g_ins;
static HWND g_wnd;
static HDC g_hdc;
static HGLRC g_glrc;

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

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
	case WM_DESTROY:
		ExitProcess(0);
	case WM_SIZE:
		glViewport(0, 0, LOWORD(lp), HIWORD(lp));
		return 0;
	case WM_PAINT:
		glClear(GL_COLOR_BUFFER_BIT);  
		glClearColor(0.0F, 1.0F, 0.0F, 1.0F);  
		SwapBuffers(g_hdc);
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

	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC; 
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = g_ins;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW); 
	wc.lpszClassName = L"WndClass"; 

	if (!RegisterClassW(&wc)) {
		MessageBoxW(NULL, L"Window registration failed", 
				L"Win32 Error", MB_ICONERROR);
		ExitProcess(1);
	}

	rect.left = 0;
	rect.top = 0;
	rect.right = CLIENT_WIDTH;
	rect.bottom = CLIENT_HEIGHT;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

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
}

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

/**
 * init_open_gl() - initialize OpenGL context and load necessary extensions 
 */
static void init_open_gl(void)
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

	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER |  
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;

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
}

/**
 * msg_loop() - main loop of program
 */
static void msg_loop(void)
{
	MSG msg; 

	ShowWindow(g_wnd, SW_SHOW);

	while (1) {
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
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
 * The use ExitProcess is equavilent to returning from this function 
 */
int __stdcall wWinMain(HINSTANCE ins, HINSTANCE prev, LPWSTR cmd, int show) 
{
	UNREFERENCED_PARAMETER(prev);
	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(show);

	g_ins = ins;
	set_default_directory();
	create_main_window();
	init_open_gl();

	setup();
	update_and_render(); // NOTE(Lenny): main game loop. runs once every frame
	msg_loop();
	
	return 0;
}
