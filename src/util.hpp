#ifndef UTIL_HPP
#define UTIL_HPP

#include <stdlib.h>
#include <stdint.h>

#define endof(ary) (ary + _countof(ary))

struct v2 {
	float x;
	float y;
};

inline v2 operator*(v2 a, float b){
	return {a.x * b, a.y * b};
}

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

enum
{
    #define _Key(name, string) KEY_##name,
    _Key(null, "Invalid Key")

    _Key(A, "a")
    _Key(B, "b")
    _Key(C, "c")
    _Key(D, "d")
    _Key(E, "e")
    _Key(F, "f")
    _Key(G, "g")
    _Key(H, "h")
    _Key(I, "i")
    _Key(J, "j")
    _Key(K, "k")
    _Key(L, "l")
    _Key(M, "m")
    _Key(N, "n")
    _Key(O, "o")
    _Key(P, "p")
    _Key(Q, "q")
    _Key(R, "r")
    _Key(S, "s")
    _Key(T, "t")
    _Key(U, "u")
    _Key(V, "v")
    _Key(W, "w")
    _Key(X, "x")
    _Key(Y, "y")
    _Key(Z, "z")

    _Key(0, "0")
    _Key(1, "1")
    _Key(2, "2")
    _Key(3, "3")
    _Key(4, "4")
    _Key(5, "5")
    _Key(6, "6")
    _Key(7, "7")
    _Key(8, "8")
    _Key(9, "9")

    _Key(up, "Arrow Up")
    _Key(right, "Arrow Down")
    _Key(down, "Arrow Left")
    _Key(left, "Arrow Right")

    _Key(space, "backspace")
    _Key(enter, "enter")
    _Key(escape, "escape")
    _Key(backspace, "backspace")
    _Key(period, "period")

    _Key(shift, "shift")
    
    #undef _Key
    KEY_MAX
};

#endif

