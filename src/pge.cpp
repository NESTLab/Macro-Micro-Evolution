/*
	This is where Pixel Game Engine will be compiled / implemented
	This is used for VisualEvo only, and is not necessary for the project.

	The PGE header file has some modifications which allow this project to have
		a smaller binary (less static library linkage) ; This means you cannot drop in
		a newer version of PGE.
*/

#ifdef WEXTRA_COMPONENTS
#define OLC_IMAGE_LIBPNG
#endif

#define OLC_PGE_APPLICATION

#include "olcPGE.h"