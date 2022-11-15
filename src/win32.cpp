#include "win32.hpp"

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
