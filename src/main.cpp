#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#include <windows.h>

#include <glad/glad.h>
#include <wglext.h>

#define CLIENT_WIDTH 640
#define CLIENT_HEIGHT 480

static HINSTANCE g_ins;
static HWND g_wnd;
static HDC g_hdc;
static HGLRC g_glrc;

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

static void cd_parent(LPWSTR path)
{
	WCHAR *find;

	find = wcsrchr(path, '\\');
	if (find) {
		*find = '\0';
	}
}

static void set_default_directory(void)
{
	WCHAR path[MAX_PATH];

	GetModuleFileName(NULL, path, MAX_PATH);
	cd_parent(path);
	cd_parent(path);
	SetCurrentDirectory(path);
}

static LRESULT __stdcall wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		glViewport(0, 0, LOWORD(lp), HIWORD(lp));
		return 0;
	case WM_PAINT:
		glClear(GL_COLOR_BUFFER_BIT);  
		glClearColor(0.0F, 1.0F, 0.0F, 1.0F);  
		SwapBuffers(g_hdc);
		return 0;
	}
	return DefWindowProc(wnd, msg, wp, lp);
}

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
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); 
	wc.lpszClassName = L"WndClass"; 

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, L"Window registration failed", 
				L"Error", MB_ICONERROR);
		exit(1);
	}

	rect.left = 0;
	rect.top = 0;
	rect.right = CLIENT_WIDTH;
	rect.bottom = CLIENT_HEIGHT;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	g_wnd = CreateWindow(wc.lpszClassName, L"Engine", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height, 
			NULL, NULL, g_ins, NULL);
	if (!g_wnd) {
		MessageBox(NULL, L"Window creation failed", 
				L"Error", MB_ICONERROR);
		exit(1);
	}
}
	



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

	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) 
		wglGetProcAddress("wglCreateContextAttribsARB");
	g_glrc = wglCreateContextAttribsARB(g_hdc, 0, attrib_list);	

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(tmp);
	wglMakeCurrent(g_hdc, g_glrc);

	gladLoadGL();
}

static void msg_loop(void)
{
	MSG msg; 

	ShowWindow(g_wnd, SW_SHOW);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

int __stdcall wWinMain(HINSTANCE ins, HINSTANCE prev, LPWSTR cmd, int show) 
{
	UNREFERENCED_PARAMETER(prev);
	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(show);

	g_ins = ins;
	set_default_directory();
	create_main_window();
	init_open_gl();
	msg_loop();
	
	return 0;
}
