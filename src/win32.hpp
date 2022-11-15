#ifndef WIN32_HPP
#define WIN32_HPP

#include <windows.h>

/**
 * err_wnd() - Show generic error message box
 * @parent: Parent window 
 * @err: Error message 
 */
inline void err_wnd(HWND parent, const wchar_t *err)
{
	MessageBoxW(parent, err, L"Error", MB_ICONERROR);
}

/**
 * load_procs() - Load library with procedures 
 * @path: Path to library
 * @names: Names of procedures load, termianted with NULL
 * @procs: Buffer procs to load into
 *
 * Return: The loaded library, or NULL on failure
 */
HMODULE load_procs(const char *path, const char *const *names, FARPROC *procs);

/**
 * load_procs_ver() - Attempt to one version of library with procedures 
 * @paths: Paths to attempt to load, in order give, termianted with NULL 
 * @procs: Buffer procs to load
 * @names: Names of procedures load, termianted with NULL
 *
 * Return: The loaded library, or NULL on failure
 */
HMODULE load_procs_ver(const char *const *paths, 
		const char *const *names, FARPROC *procs); 

#endif
