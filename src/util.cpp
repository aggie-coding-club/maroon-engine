#include "util.hpp"
#include "render.hpp"

void fatal_crt_err(void)
{
	wchar_t buf[1024];

	_wcserror_s(buf, _countof(buf), errno);
	MessageBoxW(g_wnd, buf, L"CRT Fatal Error", MB_OK); 
	ExitProcess(1);
}

void *xmalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (!ptr) {
		fatal_crt_err();
	}

	return ptr;
}

void *xcalloc(size_t count, size_t size)
{
	void *ptr;

	ptr = calloc(count, size);
	if (!ptr) {
		fatal_crt_err();
	}

	return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (!ptr && size) {
		fatal_crt_err();
	}

	return ptr;
}
