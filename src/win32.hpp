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
 * cd_parent() - Transforms full path into the parent full path 
 * @path: Path to transform
 */
void cd_parent(wchar_t *path);

/**
 * init_res_path() - Initialize resource file path
 *
 * Should be called before any calls to get_res_path 
 */
void init_res_path(void);

/**
 * get_res_path() - Turns path relative to res to full path
 * @dst: Full path output (should be MAX_PATH in size) 
 * @src: Relative path input
 */
void get_res_path(wchar_t *dst, const wchar_t *src);

/**
 * get_res_path() - Use format to turn realtive to res to full path
 * @dst: Full path output (should be MAX_PATH in size) 
 * @fmt: Format
 * @...: Variadic arguments
 */
void get_res_pathf(wchar_t *dst, const wchar_t *fmt, ...);

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
