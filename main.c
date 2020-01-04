#include <stdio.h>
#include <stdlib.h>

#include "libpng.h"


int main (int argc, char** argv)
{
	//first, lets open the file
    FILE* fp = fopen("test.png", "rb");
    if(!fp) {
        perror("File opening failed");
        return EXIT_FAILURE;
    }
	
	//now, let's add the contents in an array.
	fseek(fp, 0L, SEEK_END);
	long filesz = ftell(fp);
	rewind(fp);
	
	uint8_t *file_array = malloc(filesz);
	size_t ret = fread(file_array, sizeof(uint8_t), filesz, fp);
	if(ret != filesz)
	{
		perror("File loading failed");
		return -1;
	}
	
	//now, let's try to extract its contents
	
	png_t *png = malloc(sizeof(png_t));
	if(!png)
	{
		perror("Failure in allocating memory");
		return -2;			
	}
	
	int32_t ret2 = png_read(png, file_array);
	if(ret2 != PNG_FILE_OK)
	{
		perror("Failure in png_read");
		free(png);
		return ret2;		
	}
	
	//now lets try to decode its contents
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
	
	size_t raw_image_length = (png->png_width *  png->png_height * bytes_per_pixel);
		
	uint8_t *raw_image = malloc(raw_image_length);
	if (!raw_image)
	{
		perror ("raw data array not allocated");
		return -1;
	}
	
	ret2 = png_image_decode(png, raw_image);
	if(ret2 != PNG_FILE_OK)
	{
		perror("Failure in png_image_decode");
		free(png);
		return ret2;		
	}
	
	//lets output this to a file for final checking
	FILE* fp2 = fopen("output.bin", "wb");
	if (!fp2)
	{
		perror("Failure in opening output file");
		return -1;
	}
	
	ret = fwrite(raw_image, sizeof(uint8_t), raw_image_length,	fp2);
	if (ret < raw_image_length)
	{
		perror("Not all bytes were written");
	}
	
	
	
	//a properly allocated thing is a good thing!
	free (raw_image);
	free (png);
	free (file_array);
	fclose(fp);
	
	puts("A Winner Is You!");
	return 0;
}