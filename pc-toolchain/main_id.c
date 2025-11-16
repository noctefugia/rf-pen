#include "core.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP
#include "stb_image.h" //https://github.com/nothings/stb

#define APP_NAME "BITMAP CONVERTER"
#define APP_VERSION 1000

BOOL font_mode = FALSE, logo_mode = FALSE;


BYTE PXC(unsigned char *buff, int x, int y)
{
	int img_w, img_h;
	if (font_mode) {
		img_w = FONT_IMG_WIDTH;
		img_h = FONT_IMG_HEIGHT;
	} else {
		img_w = IMG_WIDTH;
		img_h = IMG_HEIGHT;
	}

	int buff_index = img_w * y + x;
	if (buff_index >= (img_w * img_h)) {
		printf("ERROR: FONT IMAGE BUFFER OUT OF BOUNDS\n");
		return 0;
	}
	return ( (BYTE)((buff[buff_index] >= 128) ? 0 : 1) );
}


void SortImgBuffer(BYTE *buff, int buff_sz)
{
	int i, x, y, y2;
	BYTE temp_buff[buff_sz];

	x = y = y2 = 0;
	for (i = 0; i < buff_sz; ++i) {
		temp_buff[i] = PXC(buff, x, y2 + 7 - y);
		if ((++y) == 8) {
			y = 0;
			if ((++x) == IMG_WIDTH) {
				x = 0;
				y2 += 8;
			}
		}
	}
	memcpy(buff, temp_buff, buff_sz);
}


BOOL ProcessImage(const char *filename)
{
	if (!CORE_FileExists(filename)) {
		printf("UNABLE TO OPEN FILE %s\n", filename);
		return FALSE;
	}

	int x, y, n, ok, i;
	ok = stbi_info(filename, &x, &y, &n);
	if (!ok)
		return FALSE;
	printf("\nSOURCE IMAGE PARAMS:\nwidth = %i\nheight = %i\nchannels = %i\n\n", x, y, n);

	int ok_x = IMG_WIDTH, ok_y = IMG_HEIGHT;
	if (font_mode) {
		ok_x = FONT_IMG_WIDTH;
		ok_y = FONT_IMG_HEIGHT;
	}
	if ( (ok_x != ok_x) || (ok_y != ok_y) ) {
		printf("IMAGE SIZE IS NOT SUPPORTED\nSUPPORTED SIZE: %ix%i\n", ok_x, ok_y);
		return FALSE;
	}

	BYTE *buff = stbi_load(filename, &x, &y, &n, 1);
	if (!buff)
		return FALSE;
 	
	if (font_mode) {
		int j, k, bbuff_pos;
		BYTE bin_buff[FONT_OUTPUT_BUFF_SZ];

		bbuff_pos = 0;
		for (i=0; i < (int)FONT_OUTPUT_BUFF_SZ; ++i)
			bin_buff[i] = 0;

		for (i=0; i < 0x100; ++i) {
			x = 1 + (i%IMG_HEIGHT)*(FONT_CHAR_W+1);
			y = 1 + (i/IMG_HEIGHT)*(FONT_CHAR_H+1);
			for (k = 0; k < FONT_CHAR_W; ++k) {
				for (j = 0; j < FONT_CHAR_H; ++j) {
					if (bbuff_pos >= (int)FONT_OUTPUT_BUFF_SZ) {
						printf("OUTPUT FONT BUFFER OUT OF BOUNDS\n");
						return FALSE;
					}
					bin_buff[bbuff_pos] <<= 1;
					bin_buff[bbuff_pos] |= PXC(buff, x+k, y+(FONT_CHAR_H-1)-j);
				}
				++bbuff_pos;
			}
		}
		remove(BIN_FONT_FILENAME);
		FILE *fp;
		fp = fopen(BIN_FONT_FILENAME,"wb");
		fwrite(bin_buff, sizeof(BYTE), FONT_OUTPUT_BUFF_SZ, fp);
		fclose(fp);
		printf("BINARY FONT FILE GENERATED\n");
	}

	else {
		int buff_sz = x*y;
		SortImgBuffer(buff, buff_sz);

		if (logo_mode) {
			int bin_buff_sz = (x*y)/8, bin_buff_index = 0;
			BYTE bin_buff[bin_buff_sz], bin_buff_byte = 0, bin_buff_bit = 0;
			for (i=0; i < bin_buff_sz; ++i)
				bin_buff[i] = 0x00;
			for (i=0; i < buff_sz; ++i) {
				bin_buff_byte <<= 1;
				if (buff[i] == 1)
					bin_buff_byte |= 0x01;
				if ((++bin_buff_bit) == 8) {
					if (bin_buff_index >= bin_buff_sz) {
						printf("BINARY LOGO BUFFER OUT OF BOUNDS\n");
						return FALSE;
					}
					bin_buff[bin_buff_index++] = bin_buff_byte;
					bin_buff_byte = 0;
					bin_buff_bit = 0;
				}
			}
			remove(BIN_LOGO_FILENAME);
			FILE *fp = fopen(BIN_LOGO_FILENAME,"wb");
			fwrite(bin_buff, sizeof(BYTE), bin_buff_sz, fp);
			fclose(fp);
			printf("BINARY LOGO FILE GENERATED\n");
		} 
		
		else {
			for (i=0; i < buff_sz; ++i)
				buff[i] = (buff[i] == 1) ? IMG_BLACK_BIT_CODE : IMG_WHITE_BIT_CODE;
			
			BYTE clip_buff[buff_sz+3];
			n = 0;
			clip_buff[n++] = IMG_BEGIN_END_CODE;
			for (i = 0; i < buff_sz; ++i)
				clip_buff[n++] = buff[i];
			clip_buff[n++] = IMG_BEGIN_END_CODE;
			clip_buff[n] = '\0';

			CORE_ClipboardSetText(clip_buff, buff_sz+3);
			printf("RAW DATA COPIED TO CLIPBOARD\n");
		}
	}

	stbi_image_free(buff);

	printf("IMAGE PROCESSING COMPLETED\n\n");

	return TRUE;
}





int main(int argc, char *argv[])
{
	errno_t err;

	printf("\n");
	CORE_PrintAppCaption(APP_NAME, APP_VERSION);
	
	char filename[MAX_LINE];
	if ( (argc == 2) && (strcmp(argv[1], CORE_FONT_FILENAME) == 0) ) {
		font_mode = TRUE;
		err = strcpy_s(filename, MAX_LINE, CORE_FONT_FILENAME);
		printf("FONT GENERATION MODE\n");
	} else if ( (argc == 2) && (strcmp(argv[1], CORE_LOGO_FILENAME) == 0) ) {
		logo_mode = TRUE;
		err = strcpy_s(filename, MAX_LINE, CORE_LOGO_FILENAME);
		printf("LOGO GENERATION MODE\n");
	} else {
		printf("ENTER BITMAP FILENAME:\n");
		scanf(LINE_FORMAT, filename);
	}

	if (!ProcessImage(filename))
		printf("ERROR: IMAGE PROCESSING FAILED\n\n");

	system("PAUSE");
    return 0;
}


