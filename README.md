# Team Members
Lenny T. , James F. , Robert S. , Wesley T. , Jonah B. , Rahif M. , Neil K., Wyatt C., Jeremy M., Grace K., Shantanu R., Victor P.,
# The Project Goal

# Project Setup Instructions

## System Requirements
This project assumes you have access to x64 Windows 7 or later.  

You will also need to have a version of OpenGL version 3.3 or later drivers.  
You can check OpenGL version by using this download link:  
https://www.realtech-vr.com/api/deeplink?SKU=15&target=UrlWin32   

## Mingw-w64 Setup
We will be using mingw-w64 as our main compiler, linker, and build system.  
Click this link to download a pre-built binary for mingw-w64.  
https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev0/x86_64-12.2.0-release-win32-sjlj-rt_v10-rev0.7z  
Decompress the download using 7-Zip (You can download 7-Zip here: https://www.7-zip.org/a/7z2201-x64.exe).  
Move the "mingw64" folder to  "C:/Program Files".  

## Adding Mingw64 to Path
Use the start menu to search for "environment variables".  
Click "Edit the system environment variables".  
Click "Environment variables" at the bottom right of the prompt that opens.  
Click on "Path" under System Variables.  
Click new in the prompt that opens.  
Copy "C:\Program Files\mingw64\bin" into the textbox.  
And hit "ok" for all the windows that were opened.  

If you have any problems thus far, contact Lenny or James.  

## CMake
Install CMAKE, it will be necessary for building the external dependencies of the program.
You must select either "Add CMake to the system path for all users" or  "Add CMake to the system path for the current user".
I recommend the former.
Download Link: https://github.com/Kitware/CMake/releases/download/v3.25.0-rc1/cmake-3.25.0-rc1-windows-x86_64.msi

## Running
In order to confirm a successful setup open up git-bash. 
Type in "git clone https://github.com/aggie-coding-club/maroon-engine.git --recurse" and hit enter.  
Type in "cd maroon-engine" and hit enter.  
Type in "mingw32-make" and hit enter.  
If successful a bin folder will appear in the directory and within that bin folder will be "engine.exe".  
Use "git submodule update --init --recursive" to retrieve new dependencies.

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
			static void cd_parent(wchar_t *path)
			{
				wchar_t *find;

				find = wcsrchr(path, '\\');
				if (find) {
					*find = '\0';
				}
			}
		bad:
			static void cd_parent(wchar_t *path)
			{
				wchar_t *find;

				find = wcsrchr(path, '\\');
				if (find)
					*find = '\0';
			}

			static void cd_parent(wchar_t *path) {
				wchar_t *find;

				find = wcsrchr(path, '\\');
				if (find)
					*find = '\0';
			}

			static void cd_parent(wchar_t *path) 
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

