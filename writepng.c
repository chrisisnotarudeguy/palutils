/*---------------------------------------------------------------------------

   wpng - simple PNG-writing program                             writepng.c

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2007 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      The contents of this file are DUAL-LICENSED.  You may modify and/or
      redistribute this software according to the terms of one of the
      following two licenses (at your option):


      LICENSE 1 ("BSD-like with advertising clause"):

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "PNG: The Definitive Guide,"
            published by O'Reilly and Associates.


      LICENSE 2 (GNU GPL v2 or later):

      This program is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software Foundation,
      Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  ---------------------------------------------------------------------------*/

/* This is a slightly modified version of the writepng.c source code included in
 * the "PNG: the definitive guide" book. The original source can be found at:
 * http://www.libpng.org/pub/png/book/sources.html */


#include <stdlib.h>     /* for exit() prototype */
#include <png.h>        /* libpng header; includes zlib.h and setjmp.h */
#include <zlib.h>
#include <zlib.h>

#include "writepng.h"   /* typedefs, common macros, public prototypes */

/* local prototype */
static void writepng_error_handler(png_structp png_ptr, png_const_charp msg);
static void writepng_add_text(png_text *t, int *i, int compression, char *key, char *text);

void writepng_version_info(void)
{
    fprintf(stderr, "   Compiled with libpng %s; using libpng %s.\n", 
            PNG_LIBPNG_VER_STRING, png_libpng_ver);
    fprintf(stderr, "   Compiled with zlib %s; using zlib %s.\n", 
            ZLIB_VERSION, zlib_version);
}

/* returns 0 for success, 2 for libpng problem, 4 for out of memory; 
 * note that outfile might be stdout.
 * valid color_type values: PNG_COLOR_TYPE_GRAY, PNG_COLOR_TYPE_RGB, 
 * PNG_COLOR_TYPE_RGB_ALPHA */
int writepng_init(mainprog_info *mainprog_ptr, int color_type)
{
    png_structp png_ptr; /* note:  temporary variables! */
    png_infop info_ptr;
    int interlace_type;

    /* create the png struct and set up a custom error handler */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, mainprog_ptr, 
            writepng_error_handler, NULL);
    if (!png_ptr)
        return 4;   /* out of memory */

    info_ptr = png_create_info_struct(png_ptr); /* create info structure */
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        return 4;   /* out of memory */
    }

    /* setjmp() must be called in every function that calls a PNG-writing 
     * libpng function, unless an alternate error handler was installed--
     * but compatible error handlers must either use longjmp() themselves 
     * (as in this program) or exit immediately, so here we go: */
    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 2;
    }

    /* make sure outfile is (re)opened in BINARY mode */
    png_init_io(png_ptr, mainprog_ptr->outfile);

    /* set the compression levels--in general, always want to leave filtering turned on (except for palette images) and allow all of the filters,
     * which is the default; want 32K zlib window, unless entire image buffer is 16K or smaller (unknown here)--also the default; usually want max
     * compression (NOT the default); and remaining compression flags should be left alone */
    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
    /* >> this is default for no filtering; Z_FILTERED is default otherwise:
     * png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
     * >> these are all defaults:
     * png_set_compression_mem_level(png_ptr, 8);
     * png_set_compression_window_bits(png_ptr, 15);
     * png_set_compression_method(png_ptr, 8); */

    interlace_type = mainprog_ptr->interlaced? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE; /* interlace support */
    
    /* set up IHDR chunk */
    png_set_IHDR(png_ptr, info_ptr, mainprog_ptr->width, mainprog_ptr->height, mainprog_ptr->sample_depth, 
                 color_type, interlace_type, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (mainprog_ptr->gamma > 0.0) /* gamma support */
        png_set_gAMA(png_ptr, info_ptr, mainprog_ptr->gamma);

    if (mainprog_ptr->have_bg) {    /* set up background color */
        png_color_16 background;
        background.red = mainprog_ptr->bg_red;
        background.green = mainprog_ptr->bg_green;
        background.blue = mainprog_ptr->bg_blue;
        png_set_bKGD(png_ptr, info_ptr, &background);
    }

    if (mainprog_ptr->have_time) {  /* set up time chunk */
        png_time  modtime;
        png_convert_from_time_t(&modtime, mainprog_ptr->modtime);
        png_set_tIME(png_ptr, info_ptr, &modtime);
    }

    if (mainprog_ptr->have_text) {  /* set up text chunk */
        png_text text[6];
        int num_text = 0;

        if (mainprog_ptr->have_text & TEXT_TITLE)
            writepng_add_text(text, &num_text, PNG_TEXT_COMPRESSION_NONE, 
                    "Title", mainprog_ptr->title);
        if (mainprog_ptr->have_text & TEXT_AUTHOR)
            writepng_add_text(text, &num_text, PNG_TEXT_COMPRESSION_NONE, 
                    "Author", mainprog_ptr->author);
        if (mainprog_ptr->have_text & TEXT_DESC)
            writepng_add_text(text, &num_text, PNG_TEXT_COMPRESSION_NONE, 
                    "Description", mainprog_ptr->desc);
        if (mainprog_ptr->have_text & TEXT_COPY)
            writepng_add_text(text, &num_text, PNG_TEXT_COMPRESSION_NONE, 
                    "Copyright", mainprog_ptr->copyright);
        if (mainprog_ptr->have_text & TEXT_EMAIL)
            writepng_add_text(text, &num_text, PNG_TEXT_COMPRESSION_NONE, 
                    "E-mail", mainprog_ptr->email);
        if (mainprog_ptr->have_text & TEXT_URL)
            writepng_add_text(text, &num_text, PNG_TEXT_COMPRESSION_NONE, 
                    "URL", mainprog_ptr->url);
        png_set_text(png_ptr, info_ptr, text, num_text);
    }

    /* write all chunks up to (but not including) first IDAT */
    png_write_info(png_ptr, info_ptr);

    /* if we wanted to write any more text info *after* the image data, we
     * would set up text struct(s) here and call png_set_text() again, with
     * just the new data; png_set_tIME() could also go here, but it would
     * have no effect since we already called it above (only one tIME chunk
     * allowed) */

    /* set up the transformations:  for now, just pack low-bit-depth pixels
     * into bytes (one, two or four pixels per byte) */
    png_set_packing(png_ptr);
    /*  png_set_shift(png_ptr, &sig_bit);  to scale low-bit-depth values */

    /* make sure we save our pointers for use in writepng_encode_image() */
    mainprog_ptr->png_ptr = png_ptr;
    mainprog_ptr->info_ptr = info_ptr;

    return 0;
}

static void writepng_add_text(png_text *t, int *i, int compression, char *key, char *text)
{
    t[*i].compression = compression;
    t[*i].key = key;
    t[*i].text = text;
    ++(*i);
}

/* only for interlaced images
 * returns 0 for success, 2 for libpng (longjmp) problem */
int writepng_encode_image(mainprog_info *mainprog_ptr)
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;

    /* as always, setjmp() must be called in every function that calls a PNG-writing libpng function */
    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        mainprog_ptr->png_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }

    /* and now we just write the whole image; libpng takes care of interlacing for us */
    png_write_image(png_ptr, mainprog_ptr->row_pointers);

    /* since that's it, we also close out the end of the PNG file now--if we
     * had any text or time info to write after the IDATs, second argument
     * would be info_ptr, but we optimize slightly by sending NULL pointer: */
    png_write_end(png_ptr, NULL);

    return 0;
}

/* Encodes a single row. For non-interlaced PNG images.
 * returns 0 if succeeds, 2 if libpng problem */
int writepng_encode_row(mainprog_info *mainprog_ptr)  /* NON-interlaced only! */
{
    png_structp png_ptr = (png_structp) mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop) mainprog_ptr->info_ptr;

    /* setjmp() must be called in every function that calls a PNG-writing libpng function */
    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        mainprog_ptr->png_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }

    /* image_data points at our one row of image data */
    png_write_row(png_ptr, mainprog_ptr->image_data);

    return 0;
}

/* Finishes encoding of row for NON-intercaled images.
 * returns 0 if succeeds, 2 if libpng problem */
int writepng_encode_finish(mainprog_info *mainprog_ptr)
{
    png_structp png_ptr = (png_structp) mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop) mainprog_ptr->info_ptr;

    /* setjmp() must be called in every function that calls a 
     * PNG-writing libpng function */
    if (setjmp(mainprog_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        mainprog_ptr->png_ptr = NULL;
        mainprog_ptr->info_ptr = NULL;
        return 2;
    }

    /* close out PNG file; if we had any text or time info to write after
     * the IDATs, second argument would be info_ptr: */
    png_write_end(png_ptr, NULL);

    return 0;
}

void writepng_cleanup(mainprog_info *mainprog_ptr)
{
    png_structp png_ptr = (png_structp)mainprog_ptr->png_ptr;
    png_infop info_ptr = (png_infop)mainprog_ptr->info_ptr;

    if (png_ptr && info_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
}

static void writepng_error_handler(png_structp png_ptr, png_const_charp msg)
{
    mainprog_info *mainprog_ptr;

    /* This function, aside from the extra step of retrieving the "error pointer" (below) and the fact that it exists within the application
     * rather than within libpng, is essentially identical to libpng's default error handler. The second point is critical: since both
     * setjmp() and longjmp() are called from the same code, they are guaranteed to have compatible notions of how big a jmp_buf is,
     * regardless of whether _BSD_SOURCE or anything else has (or has not) been defined. */
    fprintf(stderr, "ERROR: writepng libpng error: %s\n", msg);
    fflush(stderr);

    mainprog_ptr = png_get_error_ptr(png_ptr);
    if (mainprog_ptr == NULL) {         /* we are completely hosed now */
        fprintf(stderr, "ERROR: writepng severe error: jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        exit(1);
    }

    longjmp(mainprog_ptr->jmpbuf, 1);
}
