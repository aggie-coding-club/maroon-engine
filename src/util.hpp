#ifndef UTIL_HPP
#define UTIL_HPP

#include <stdlib.h>
#include <stdint.h>

#define endof(ary) (ary + _countof(ary))

struct v2 {
	float x;
	float y;
};

struct v2i {
	int16_t tx;
	int16_t ty;
};

/**
 * fatal_crt_error() - Display message box with CRT error and exit 
 *
 * This function is used if a C-Runtime (CRT) function fails with
 * a unrecoverable error. 
 */
void fatal_crt_err(void);

/**
 * xmalloc() - Alloc memory, crash on failure
 * @size: Size of allocation
 *
 * Return: Pointer to allocation 
 *
 * Pass to "free" to deallocate.
 */
void *xmalloc(size_t size);

/**
 * xcalloc() - Alloc zeroed out memory, crash on failure
 * @count: Count of elements
 * @size: Size of element 
 *
 * Return: Pointer to allocation 
 *
 * Pass to "free" to deallocate.
 */
void *xcalloc(size_t count, size_t size);

/**
 * xrealloc() - Reallocates memory, crashes on failure
 * @ptr: Pointer to realloc 
 * @size: New size
 *
 * Return: Pointer to allocation 
 *
 * Pass to "free" to deallocate.
 */
void *xrealloc(void *ptr, size_t size);

#endif

