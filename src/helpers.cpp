#include "helpers.hpp"

/**
 * fatal_crt_error() - Display message box with CRT error and exit 
 *
 * This function is used if a C-Runtime (CRT) function fails with
 * a unrecoverable error. 
 */
void fatal_crt_err()
{
	wchar_t buf[1024];

	_wcserror_s(buf, _countof(buf), errno);
	MessageBoxW(NULL, buf, L"CRT Fatal Error", MB_OK); 
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
void *xmalloc(size_t size)
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
 * wgl_load() - load WGL extension function 
 * @name: name of function 
 *
 * Return: Valid function pointer
 *
 * Crashes if function not found
 */
PROC wgl_load(const char *name) 
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