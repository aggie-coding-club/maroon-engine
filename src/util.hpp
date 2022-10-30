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

/**
 * min() - Minimum of two integers 
 * @a: Left value
 * @b: Right value 
 * 
 * NOTE: Use fmin and fminf for doubles/floats respectivly 
 *
 * Return: Return minimum of two integers 
 */
static int min(int a, int b)
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

enum {
    #define DECLARE_KEY(name, string) KEY_##name,
    DECLARE_KEY(null, "Invalid Key")

    DECLARE_KEY(A, "a")
    DECLARE_KEY(B, "b")
    DECLARE_KEY(C, "c")
    DECLARE_KEY(D, "d")
    DECLARE_KEY(E, "e")
    DECLARE_KEY(F, "f")
    DECLARE_KEY(G, "g")
    DECLARE_KEY(H, "h")
    DECLARE_KEY(I, "i")
    DECLARE_KEY(J, "j")
    DECLARE_KEY(K, "k")
    DECLARE_KEY(L, "l")
    DECLARE_KEY(M, "m")
    DECLARE_KEY(N, "n")
    DECLARE_KEY(O, "o")
    DECLARE_KEY(P, "p")
    DECLARE_KEY(Q, "q")
    DECLARE_KEY(R, "r")
    DECLARE_KEY(S, "s")
    DECLARE_KEY(T, "t")
    DECLARE_KEY(U, "u")
    DECLARE_KEY(V, "v")
    DECLARE_KEY(W, "w")
    DECLARE_KEY(X, "x")
    DECLARE_KEY(Y, "y")
    DECLARE_KEY(Z, "z")

    DECLARE_KEY(0, "0")
    DECLARE_KEY(1, "1")
    DECLARE_KEY(2, "2")
    DECLARE_KEY(3, "3")
    DECLARE_KEY(4, "4")
    DECLARE_KEY(5, "5")
    DECLARE_KEY(6, "6")
    DECLARE_KEY(7, "7")
    DECLARE_KEY(8, "8")
    DECLARE_KEY(9, "9")

    DECLARE_KEY(UP, "Arrow Up")
    DECLARE_KEY(RIGHT, "Arrow Down")
    DECLARE_KEY(DOWN, "Arrow Left")
    DECLARE_KEY(LEFT, "Arrow Right")

    DECLARE_KEY(SPACE, "backspace")
    DECLARE_KEY(ENTER, "enter")
    DECLARE_KEY(ESCAPE, "escape")
    DECLARE_KEY(BACKSPACE, "backspace")
    DECLARE_KEY(PERIOD, "period")

    DECLARE_KEY(SHIFT , "shift")
    
    #undef DECLARE_KEY
    KEY_MAX
};

#endif

