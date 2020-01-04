#include <string.h>
#include <stddef.h>
#include <math.h> //floor, min
#include <stdlib.h>
#include <stdio.h>

#include <endian.h>

#include "libpng.h"
#include "miniz.h"

//8 bits per pixel
#define RGB24_TO_RGB555(r, g, b) ((((b) >> 3) << 10) | (((g) >> 3) << 5)  | \
        ((r) >> 3))

#define RGBA32_TO_RGB555 RGB24_TO_RGB555

//16 bits per pixel
#define RGB48_TO_RGB555(r, g, b) (uint8_t) ( ((uint8_t)((b) >> 11) << 10) | \
											 ((uint8_t)((g) >> 11) << 5)  | \
											 ((uint8_t)((b) >> 11)) )

#define RGBA64_TO_RGB555 RGB48_TO_RGB555

static int32_t indexed_image_decode(uint8_t *, const png_t *);
static int32_t sampled_image_decode(uint8_t *, const png_t *);
static int8_t png_paeth_predictor(uint8_t *, uint32_t, uint16_t, uint8_t);
static int32_t sampled_image_filter_decode(uint8_t **, uint32_t, const png_t *, uint8_t **);
static int32_t interlace_adam7 (const uint8_t *, uint32_t, const png_t *, uint8_t *);
static int32_t getChunk(png_chunk *, const uint8_t *);
static void next_chunk();

int32_t png_read(png_t *png, const uint8_t *file)
{
    const uint8_t sigcheck[7] 	= {80, 78, 71, 13, 10, 26,10};
    const uint8_t IHDR_Check[4] = {73, 72, 68, 82};
    const uint8_t *header;

    uint8_t  magic_8_bit;
    const uint8_t  *signature;

    uint16_t image_width;
    uint16_t image_height;
    uint8_t  image_bpp;
    uint8_t  image_color_type;
    uint8_t  image_compression_method;
    uint8_t  image_filter_method;
    uint8_t	 image_interlace_method;

    /* Read the PNG signature */
    magic_8_bit = file[0];
    if (magic_8_bit != 0x89)
        return PNG_FILE_CORRUPTED;

    signature = &file[1];
    for (int i=0; i<7; i++)
    {
        if(signature[i] != sigcheck[i])
            return PNG_FILE_CORRUPTED;
    }

    /* Read the first chunk and check if it's the header chunk*/
    /**/
    png_chunk chunk;
    getChunk(&chunk, &file[8]);
    if ( chunk.chunk_type != PNG_CHUNK_TYPE_IHDR )
        return PNG_FILE_CORRUPTED;
    
    image_width  = be32toh( *(uint32_t*)&chunk.chunk_data[0] );
    image_height = be32toh( *(uint32_t*)&chunk.chunk_data[4] ); 
    image_bpp = chunk.chunk_data[8];
    image_color_type = chunk.chunk_data[9];
    image_compression_method = chunk.chunk_data[10];
    image_filter_method = chunk.chunk_data[11];
    image_interlace_method = chunk.chunk_data[12];
    
    // Sanity Checks
    switch(image_color_type)
    {
    case PNG_COLOR_TYPE_GRAYSCALE:
    case PNG_COLOR_TYPE_RGB:
    case PNG_COLOR_TYPE_INDEXED:
    case PNG_COLOR_TYPE_GRAYSCALE_ALPHA:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        break;
    default:
        return PNG_FILE_CORRUPTED;
    }

    switch(image_compression_method)
    {
    case PNG_COMPRESSION_DEFLATE:
        break;
    default:
        return PNG_FILE_CORRUPTED;
    }
    
    switch(image_filter_method)
    {
    case PNG_FILTER_METHOD_0:
        break;
    default:
        return PNG_FILE_CORRUPTED;
    }
    
    switch(image_interlace_method)
    {
    case PNG_INTERLACE_TYPE_NONE:
    case PNG_INTERLACE_TYPE_ADAM7:
        break;
    default:
        return PNG_FILE_CORRUPTED;
    }
    
    //loading to the png struct
    png->png_file 					= file;
    png->data_start					= &file[8];
    png->png_width 					= image_width;
    png->png_height 				= image_height;
    png->png_bpp 					= image_bpp;
    png->png_color_type 			= image_color_type;
    png->png_compression_method 	= image_compression_method;
    png->png_filter_method 			= image_filter_method;
    png->png_interlace_method 		= image_interlace_method;

    return PNG_FILE_OK;
}

int32_t png_image_decode(const png_t *png, void *dst)
{
    switch(png->png_color_type)
    {
    case PNG_COLOR_TYPE_INDEXED:
        return indexed_image_decode(dst, png);
    case PNG_COLOR_TYPE_GRAYSCALE:
    case PNG_COLOR_TYPE_RGB:
    case PNG_COLOR_TYPE_GRAYSCALE_ALPHA:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        return sampled_image_decode(dst, png);
    default:
        return PNG_FILE_CORRUPTED;
    }
}

int32_t png_plte_decode(const png_t *png, uint16_t *dst)
{
}

const char *png_error_stringify(int error)
{
    switch (error)
    {
    case PNG_FILE_OK:
        return "File OK";
    case PNG_FILE_UNKNOWN_FORMAT:
        return "File unknown format";
    case PNG_FILE_CORRUPTED:
        return "File corrupted";
    case PNG_FILE_NOT_SUPPORTED:
        return "File not supported";
    case PNG_MEMORY_ERROR:
        return "Memory error";
    default:
        return "Unknown error";
    }
}

static int32_t getChunk(png_chunk *chunk, const uint8_t *chunkstart)
{
    chunk->chunk_length = be32toh( *(uint32_t*)&chunkstart[0] );
    chunk->chunk_type   = be32toh( *(uint32_t*)&chunkstart[4] );
    chunk->chunk_data   = &chunkstart[8];
    chunk->CRC          = be32toh( *(uint32_t*)&chunkstart[chunk->chunk_length + 8] );    
    return PNG_FILE_OK;
}

static int32_t indexed_image_decode(uint8_t *dst, const png_t *png)
{
}

static int32_t sampled_image_decode(uint8_t *dst, const png_t *png)
{
    /*The Image Buffer */
    /*To be malloc'd once a IDAT chunk is found*/
    /*To be checked for value for return*/
    uint8_t *image_buffer = NULL;
    uint32_t index = 0l;

    //We need to iterate the png file for IDATA chunks
    png_chunk chunk;
    const uint8_t *chunkstart = png->data_start;
    getChunk(&chunk, chunkstart);

    while(chunk.chunk_type != PNG_CHUNK_TYPE_IEND)
    {
        unsigned long uncompressed_length;
        unsigned long dstlength = ( png->png_width * png->png_height * 4 );
        uint8_t lz_buffer[32768];
        uint8_t *next_out = lz_buffer;
        size_t avail_out = TINFL_LZ_DICT_SIZE;
        size_t out_bufsize = avail_out;
        tinfl_decompressor decompressor;
        tinfl_init(&decompressor);
        //check its type
        if(chunk.chunk_type == PNG_CHUNK_TYPE_IDAT)
        {
            switch(png->png_compression_method) {
            case PNG_COMPRESSION_DEFLATE:
            {
                int ret = tinfl_decompress(&decompressor,
                    chunk.chunk_data, (size_t*)&chunk.chunk_length,
                    lz_buffer, next_out, &out_bufsize,
                    TINFL_FLAG_PARSE_ZLIB_HEADER);
                if (ret < TINFL_STATUS_DONE)
                    return ret;
                
                next_out   += out_bufsize;
                avail_out  -= out_bufsize;
                
                if (ret == TINFL_STATUS_DONE || avail_out == 0) 
                {
                    
                    //image_buffer = lz_buffer;
                    uncompressed_length = TINFL_LZ_DICT_SIZE - avail_out;
                    
                    // Output buffer is full, or decompression is done, so write buffer to output file.
                    // XXX: This is the only chance to process the buffer.
                    image_buffer = malloc (dstlength);
                    memcpy (image_buffer, lz_buffer, uncompressed_length);

                    // pngle_on_data() usually returns n, otherwise -1 on error
                    //if (pngle_on_data(pngle, read_ptr, n) < 0) return -1;

                    // XXX: tinfl_decompress always requires (next_out - lz_buf + avail_out) == TINFL_LZ_DICT_SIZE
                    next_out = lz_buffer;
                    avail_out = TINFL_LZ_DICT_SIZE;
                }   
                
                break;
            }
            default:
                return PNG_FILE_CORRUPTED;
            }

            switch(png->png_filter_method) {
            case PNG_FILTER_METHOD_0:
            {
                sampled_image_filter_decode(&image_buffer, uncompressed_length, png, NULL);
                break;
            }
            default:
                return PNG_FILE_CORRUPTED;
            }

            switch(png->png_interlace_method) {
            case PNG_INTERLACE_TYPE_NONE:
                //no interlace? It's done, then.
                memcpy( dst, image_buffer, dstlength);
                return PNG_FILE_OK;
            case PNG_INTERLACE_TYPE_ADAM7:
                interlace_adam7(image_buffer, dstlength, png, dst);
                return PNG_FILE_OK;
            default:
                return PNG_FILE_CORRUPTED;
            }

        }

        //Go to the next chunk
        chunkstart+=
            (PNG_CHUNK_UTIL_SIZE + chunk.chunk_length);
        getChunk(&chunk, chunkstart);
    }

    if(image_buffer == NULL)
        return PNG_FILE_CORRUPTED;

    return PNG_FILE_OK;
}

static int8_t png_paeth_predictor(uint8_t *decoded, uint32_t current_len, uint16_t bytes_per_scanline, uint8_t bytes_per_pixel)
{
    int index_a =  current_len - bytes_per_pixel;
    int index_b =  current_len - bytes_per_scanline ;
    int index_c =  current_len - bytes_per_scanline  - bytes_per_pixel;
    
    uint8_t a = decoded[ index_a ];
    uint8_t b = (index_b >= 0)? decoded[ index_b ] : 0;
    uint8_t c = (index_c >= 0)? decoded[ index_c ] : 0;
    
    printf("indexa:%d indexb:%d indexc:%d \n", index_a,index_b,index_c);
    
    uint32_t p = a + b - c;
    uint32_t pa = abs(p - a);
    uint32_t pb = abs(p - b);
    uint32_t pc = abs(p - c);
    printf("a:%d b:%d c:%d \n", a,b,c);
    printf("p: %d pa:%d pb:%d pc:%d \n", p, pa,pb,pc);
    if (pa <= pb && pa <= pc)
        return a;
    if (pb <= pc)
        return b;

    return c;
}

static int32_t sampled_image_filter_decode(uint8_t **src, uint32_t src_length, const png_t *png, uint8_t **dst )
{
    uint8_t *dst_buffer;

    //if destination is null, then the dst buffer needs to be made, and then switched with the src buffer
    uint8_t channels_per_pixel;
    switch (png->png_color_type)
    {
        case PNG_COLOR_TYPE_GRAYSCALE:
        case PNG_COLOR_TYPE_INDEXED:
            channels_per_pixel = 1;
            break;
        case PNG_COLOR_TYPE_GRAYSCALE_ALPHA:
            channels_per_pixel = 2;
            break;
        case PNG_COLOR_TYPE_RGB:
            channels_per_pixel = 3;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            channels_per_pixel = 4;
            break;
        default:
            return PNG_FILE_CORRUPTED;
    }

    uint8_t bytes_per_pixel = channels_per_pixel * ( (png->png_bpp + 7) >> 3);
    uint32_t bytes_per_scanline = ( png->png_width * bytes_per_pixel  );
    uint32_t bytes_per_row = bytes_per_scanline;
    
    if (dst == NULL || *dst == NULL || *src == *dst ) // dst should be null if the source is the destination
    {
        dst_buffer = malloc(png->png_height * bytes_per_scanline);
    }

    const uint8_t *filter_data  = *src;
    uint8_t *dstStart = dst_buffer;
    
    
    for (int row = 0; row < png->png_height; row++)
    {
        printf("Column %d\n", row);
        printf("---------------------------------------------------\n");
        uint8_t filter_type = filter_data[0];
        for (int column = 1; column <= bytes_per_scanline; column++)
        {
            switch(filter_type)
            {
            case PNG_FILTER_TYPE_NONE:
                *dst_buffer++ = filter_data[column];
                break;
            case PNG_FILTER_TYPE_SUB:
                *dst_buffer++ = filter_data[column] + ( (column <= bytes_per_pixel) ? 0 : dstStart[ (row * bytes_per_row) + (column - 1) - bytes_per_pixel ] );
                break;
            case PNG_FILTER_TYPE_UP:
                *dst_buffer++ = filter_data[column] + ( (row > 1) ? dstStart[ ( ( row - 1) * bytes_per_row) + (column - 1) ] : 0 );
                break;
            case PNG_FILTER_TYPE_AVG:
                *dst_buffer++ = filter_data[column] +
                         floor(
                                ( ( dstStart[  (row * png->png_width) + (column) ] ) +
                                  ( dstStart[  (row * png->png_width - 1) + (column - 1)] )
                                )/ 2
                         );
            break;
            case PNG_FILTER_TYPE_PAETH:
            {   
                printf("index %2d %1d\n\t", filter_data[column], ((row * bytes_per_row) + (column - 1)) );
                int8_t test = png_paeth_predictor(dstStart, ((row * bytes_per_row) + (column - 1)), bytes_per_scanline, bytes_per_pixel );
                *dst_buffer = filter_data[column] + test;
                dst_buffer++;
            }
            break;
            default:
                break;
            }
        }

        filter_data += bytes_per_scanline + 1;
    }
    
    //TODO: Reallocate Properly
    *src = dstStart;
}

static int32_t interlace_adam7 (const uint8_t *src, uint32_t src_length, const png_t *png, uint8_t *dst )
{
    uint32_t starting_row[7]  = { 0, 0, 4, 0, 2, 0, 1 };
    uint32_t starting_col[7]  = { 0, 4, 0, 2, 0, 1, 0 };
    uint32_t row_increment[7] = { 8, 8, 8, 4, 4, 2, 2 };
    uint32_t col_increment[7] = { 8, 8, 4, 4, 2, 2, 1 };
    uint32_t block_height[7]  = { 8, 8, 4, 4, 2, 2, 1 };
    uint32_t block_width[7]   = { 8, 4, 4, 2, 2, 1, 1 };

    uint32_t pass;
    uint32_t row, col, index;

    pass = 0;
    index = 0;
    while (pass < 7)
    {
        row = starting_row[pass];
        while (row < png->png_height)
        {
            col = starting_col[pass];
            while (col < png->png_width)
            {
                // Technically it takes the current source position and places it where it should go in the destination.
                // Because we are not streaming this, but instead saving this, it's best to not go for the waterfall effect.
                
                dst[ (row * png->png_width) + col ] = src[index];
                
                col = col + col_increment[pass];
                index++;
            }
            row = row + row_increment[pass];
        }
        pass = pass + 1;
    }
}
