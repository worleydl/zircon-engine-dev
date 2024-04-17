/*
	Copyright (C) 2006  Serge "(515)" Ziryukin, Ashley Rose Hale (LadyHavoc)

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/

//LadyHavoc: rewrote most of this.

#if 0 && defined(_MSC_VER) && _MSC_VER < 1900 // Baker: cutoff?

	#include "quakedef.h"

	unsigned char *PNG_LoadImage_BGRA (const unsigned char *pbytes, int data_size, int *miplevel_unused)
	{
		return NULL;
	}

#else
	#include "darkplaces.h"
	#include "image.h"
	# undef sprintf
	# undef snprintf
	# undef vsnprintf
	#include "image_png.h"




	/*
	=================================================================

	  DLL load & unload

	=================================================================
	*/

	#undef strcat
	#undef strncat
	#undef strcpy
	#undef strncpy


	#include "lodepng.h"

	unsigned char *PNG_LoadImage_BGRA (const unsigned char *pbytes, int data_size, int *miplevel_unused)
	{
		byte *lodepng_data = NULL;
		unsigned uwidth, uheight, lodepng_error, numpels;

		lodepng_error = lodepng_decode32 (&lodepng_data, &uwidth, &uheight, pbytes, (size_t) data_size);

		if (lodepng_error) {
			return NULL;
			//log_fatal("error %u: %s", error, lodepng_error_text(error)); return NULL;
		}

		// Baker: we have some globals here
		image_width = (int)uwidth;
		image_height = (int)uheight;
		numpels = image_width * image_height;

		unsigned char *image_data = NULL;

		image_data = (unsigned char *)Mem_Alloc(tempmempool, numpels * 4);

		if (image_data) {
			memcpy (image_data, lodepng_data, numpels * 4);
		}

		free (lodepng_data);

		if (!image_data) {
			Con_PrintLinef ("PNG_LoadImage : not enough memory");
			return NULL;
		}

		// swizzle RGBA to BGRA
		unsigned y, c;
		for (y = 0; y < (unsigned int)(image_width * image_height * 4); y += 4) {
			c = image_data[y+0];
			image_data[y+0] = image_data[y+2];
			image_data[y+2] = c;
		}

		return image_data;
	}

	void *Image_Save_PNG_Memory_Alloc (const unsigned *rgba, int width, int height, /*required*/ size_t *png_length, const char *description)
	{		
		byte *png_out = NULL;

		unsigned error = lodepng_encode32 (&png_out, png_length, (byte *)rgba, width, height);

		if (error) {
			Con_PrintLinef ("Image_Save_PNG_Memory_Alloc: Error saving image to memory '%s'", description);
			return NULL;
		}

		return png_out;
	}

#endif


/*
====================
PNG_SaveImage_preflipped

Save a preflipped PNG image to a file
====================
*/
qbool PNG_SaveImage_preflipped (const char *filename, int width, int height, qbool has_alpha, unsigned char *data)
{


	return false;
}
