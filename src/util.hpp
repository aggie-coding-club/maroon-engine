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
	int16_t x;
	int16_t y;
};

inline v2 operator*(v2 a, float b)
{
	return (v2) {a.x * b, a.y * b};
}

inline v2 operator+(v2 a, v2 b)
{
	return (v2) {a.x + b.x, a.y + b.y};
}

inline v2 operator+=(v2 a, v2 b)
{
	return (v2) {a.x + b.x, a.y + b.y};
}

inline int div_up(int val, int div)
{
	return (val + div - 1) / div;
}

/**
 * min() - Minimum of two integers 
 * @a: Left value
 * @b: Right value 
 * 
 * NOTE: Use fmin and fminf for doubles/floats respectivly 
 *
 * Return: Return minimum of two integers 
 */
inline int min(int a, int b)
{
	return a < b ? a : b;
}

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

