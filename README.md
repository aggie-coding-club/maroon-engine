# The Project Goal

# Current Features

# Upcoming Features

# Project Setup Instructions

# Style Guide 
	tabs used for indentation

	line limit of 80 chars

	break up lines with double tab

	lowercase snake_case for naming variables, functions, 
	iterator macros, primitive macros, and types

	typedef all function pointers 

	postfix function pointer with _fn

	typedef all function pointers to require an "*" when used
		good: typedef int fun_fn(int a, int b);
		bad: typedef int (*fun_fn)(int a, int b); 

	use capital SNAKE_CASE for all constant macros, 
	function-like macros, and enumeration constants

	globals are to be prefixed with g_
		exception: function pointers loaded using 
		either GetProcAddress or wglGetProcAddress 

	variables declared at the top of function
		static variables declared before non-static

	K&R style brackets with mandatory brackets on single line:
		good:
			static void cd\_parent(wchar\_t *path)
			{
				wchar_t *find;

				find = wcsrchr(path, '\\');
				if (find) {
					*find = '\0';
				}
			}
		bad:
			static void cd\_parent(wchar\_t *path)
			{
				wchar_t *find;

				find = wcsrchr(path, '\\');
				if (find)
					*find = '\0';
			}

			static void cd\_parent(wchar\_t *path) {
				wchar_t *find;

				find = wcsrchr(path, '\\');
				if (find)
					*find = '\0';
			}

			static void cd\_parent(wchar\_t *path) 
			{
				wchar_t *find;

				find = wcsrchr(path, '\\');
				if (find)
				{
					*find = '\0';
				}
			}
	space should be between control statement and expression 
		good: while (i)
		bad: while(1)
	
	spaces between math operators
		good: 3 + 2
		bad: 3+2

	paranthesies should always be used for sizeof

	paranthesies preferred over memorizing the order of operations 
		excepiton: PEMDAS is assumed
		good: (3 + 2 * 4) >> 2 
		bad: 3 + 2 * 4 >> 2
		bad: (3 + (2 * 4)) >> 2

	file structure
		include guard (if applicable)
		includes
		macros
		type definitions
		global variables
		functions

	include guard at the top of all headers
		Include guard macro should be identical to the path
		relative to src but with underscores instead of
		slashes

		example:
			obj/tree.h -> OBJ_TREE_H 

	unicode	
		use wide version of Win32 function (UTF-16), elsewise use UTF-8
	
	includes at top file (after include guard, if header)
		seperate includes into three groups (order is significant):
			built-in system includes  (ex. #include <stdio.h>)
			non-built-in system includes (ex. #include <wglext.h>)
			directory includes (#include "world.h")
		include groups seperate by new lines
		include within a group should be in alphabetical order 

	strict C-subset of C++:
		exceptions: TBD
	
	file and folder naming convention:
		File and folder names should be named using lowercase
		snakecase. Keep names relativily short. 

		Header and source files should either come in pairs
		with the same name except the extension or be just
		header (a single header should not represent 
		multiple source files, nor the inverse) 

		header files named with .hpp
		source files named with .cpp
	
