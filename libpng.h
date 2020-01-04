/*
 * the png library
 * Brandon M. Green
 * (c) 2019-2020
 * Based on libtga by Israel Jacquez <mrkotfw@gmail.com>
 * and David Oberhollenzer
 */


#ifndef PNG_H_
#define PNG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PNG_FILE_OK                      0
#define PNG_FILE_UNKNOWN_FORMAT         -1
#define PNG_FILE_CORRUPTED              -2
#define PNG_FILE_NOT_SUPPORTED          -3
#define PNG_MEMORY_ERROR                -4
#define PNG_MEMORY_UNALIGNMENT_ERROR    -5

typedef struct {
    const char *png_file;
    const char *data_start;

#define PNG_COLOR_TYPE_GRAYSCALE       	0
#define PNG_COLOR_TYPE_GREYSCALE       	0 //For the European programmers. Greatings from Louisiana!
#define PNG_COLOR_TYPE_RGB             	2
#define PNG_COLOR_TYPE_INDEXED         	3
#define PNG_COLOR_TYPE_GRAYSCALE_ALPHA 	4
#define PNG_COLOR_TYPE_GREYSCALE_ALPHA 	4
#define PNG_COLOR_TYPE_RGB_ALPHA       	6

#define PNG_CHUNK_UTIL_SIZE				12

#define PNG_CHUNK_TYPE_IHDR 			0x49484452
#define PNG_CHUNK_TYPE_PLTE     		0x504C5445
#define PNG_CHUNK_TYPE_IDAT     		0x49444154
#define PNG_CHUNK_TYPE_IEND     		0x49454E44

#define PNG_COMPRESSION_DEFLATE			0

#define PNG_FILTER_METHOD_0				0

#define PNG_FILTER_TYPE_NONE			0
#define PNG_FILTER_TYPE_SUB				1
#define PNG_FILTER_TYPE_UP				2
#define PNG_FILTER_TYPE_AVG				3
#define PNG_FILTER_TYPE_PAETH			4

#define PNG_INTERLACE_TYPE_NONE			0
#define PNG_INTERLACE_TYPE_ADAM7		1

    uint16_t png_width;
    uint16_t png_height;
    uint8_t  png_bpp;
    uint8_t  png_color_type;
    uint8_t  png_compression_method;
    uint8_t  png_filter_method;
    uint8_t	 png_interlace_method;

} png_t __attribute__ ((aligned(4)));

typedef struct {
    uint32_t chunk_length;
    uint32_t chunk_type;
    const uint8_t  *chunk_data;
    uint32_t CRC;
} png_chunk;

int32_t png_read(png_t *, const uint8_t *);
int32_t png_image_decode(const png_t *, void *);
int32_t png_plte_decode(const png_t *, uint16_t *);
const char *png_error_stringify(int);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PNG_H_*/
