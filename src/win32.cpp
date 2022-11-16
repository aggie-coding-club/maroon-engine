#include <stdarg.h>
#include <stdio.h>

#include "win32.hpp"

static wchar_t g_res_path[MAX_PATH];

HMODULE load_procs(const char *path, 
		const char *const *names, FARPROC *procs)
{
	HMODULE lib;	
	FARPROC *proc;
	const char *const *name;

	lib = LoadLibraryA(path); 
	if (!lib) { 
		return NULL;
	}

	proc = procs;
	for (name = names; *name; name++) {
		*proc = GetProcAddress(lib, *name);
		if (!*proc) {
			FreeLibrary(lib);
			return NULL;
		}

		proc++;
	}

	return lib;
}

HMODULE load_procs_ver(const char *const *paths, 
		const char *const *names, FARPROC *procs) 
{
	const char *const *path;

	for (path = paths; *path; path++) {
		HMODULE lib;

		lib = load_procs(*path, names, procs); 
		if (lib) {
			return lib;
		}
	}
	return NULL;
}

void cd_parent(wchar_t *path)
{
	wchar_t *find;

	find = wcsrchr(path, L'\\');
	if (find) {
		*find = L'\0';
	}
}

void init_res_path(void)
{
	GetModuleFileNameW(NULL, g_res_path, MAX_PATH);
	cd_parent(g_res_path);
	cd_parent(g_res_path);
	wcscat_s(g_res_path, MAX_PATH, L"\\res");
}

void get_res_path(wchar_t *dst, const wchar_t *src)
{
	swprintf_s(dst, MAX_PATH, L"%s\\%s", g_res_path, src);
}

void get_res_pathf(wchar_t *dst, const wchar_t *fmt, ...)
{
	wchar_t src[MAX_PATH];
	va_list ap;

	va_start(ap, fmt);
	vswprintf_s(src, MAX_PATH, fmt, ap);
	va_end(ap);
	
	get_res_path(dst, src);
}
