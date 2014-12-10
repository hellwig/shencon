#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <png.h>

#include "pngwriter.h"

using namespace std;

void abort_(const char *s, ...) {
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

int writePng(const char *fname, const unsigned char *buf, const unsigned int width, const unsigned int height, const unsigned int bits) {
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep * row_pointers;
    unsigned int y;

    // allocate heap space
    row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * height);
    for (y=0; y<height; y++) row_pointers[y] = (png_byte *) (buf+y*width*4);
    // create file
    FILE *fp = fopen(fname, "wb");
    if (!fp) abort_("[writePng] File %s could not be opened for writing", fname);

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) abort_("[writePng] png_create_write_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) abort_("[writePng] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr))) abort_("[writePng] Error during init_io");
    png_init_io(png_ptr, fp);

    /* write header */
    if (setjmp(png_jmpbuf(png_ptr))) abort_("[writePng] Error during writing header");
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    /* write image data */
    if (setjmp(png_jmpbuf(png_ptr))) abort_("[writePng] Error during write of image data");
    png_write_image(png_ptr, row_pointers);

    /* write end */
    if (setjmp(png_jmpbuf(png_ptr))) abort_("[writePng] Error during end of write");
    png_write_end(png_ptr, NULL);

    // cleanup heap allocation
    free(row_pointers);

    // close file
    fclose(fp);

    return 0;
}
