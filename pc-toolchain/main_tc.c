#include "core.h"

#define APP_NAME "EEPROM IMAGE BUILDER"
#define APP_VERSION 1000

BOOL ProcessCoreRegions(void)
{
    FILE *fp;
	BYTE cur_byte, string_pos, string_id, string_buff[CORE_STRING_COUNT][CORE_STRING_LEN];
	BOOL string_begin;
	char cur_char;
	int i;

	printf("PROCESS CORE STRINGS:\n");
	fp = fopen(CORE_STRINGS_FILENAME, "r");
	if (fp == NULL) {
		printf("UNABLE TO OPEN CORE STRINGS FILE: " CORE_STRINGS_FILENAME "\n");
		return FALSE;
	}

	string_begin = FALSE;
	string_pos = string_id = 0;
	do {
		cur_char = fgetc(fp);
		cur_byte = (BYTE)cur_char;
		if ( (cur_byte == '{') || (cur_byte == '}') ) {
			if ( (string_pos > 0) && (string_pos != CORE_STRING_LEN) ) {
				printf("UNEXPECTED END OF STRING#%u\n", string_id);
				return FALSE;
			} 
			if (cur_byte == '}') {
				string_begin = FALSE;
				++string_id;
				printf("}\n", cur_char);
			} else {
				string_begin = TRUE;
				printf("STR_ID#%u:\t{", string_id);
			}
			string_pos = 0;
		} else if (string_begin) {
			if (string_pos >= CORE_STRING_LEN) {
				printf("STRING#%u BUFFER OUT OF BOUNDS\n", string_id);
				return FALSE;
			} else if (string_id >= CORE_STRING_COUNT) {
				printf("TOO MANY STRINGS\n");
				return FALSE;
			}
			string_buff[string_id][string_pos++] = cur_byte;
			printf("%c", cur_char);
		}
	} while (cur_char != EOF);
	fclose(fp);

	if (CopyFile(BIN_FONT_FILENAME, BIN_EEPROM_FILENAME, FALSE) == 0) {
		printf("UNABLE TO CREATE " BIN_EEPROM_FILENAME " FILE\n");
		return FALSE;
	}
	printf("CORE STRINGS PROCESSING COMPLETED\n");

	fp = fopen(BIN_EEPROM_FILENAME, "ab");
	if (fp == NULL) {
		printf("UNABLE TO OPEN " BIN_EEPROM_FILENAME " FILE\n");
		return FALSE;
	}
	printf("\nADDING CORE REGIONS:\n -ADDED FONT REGION (%lu BYTES)\n", CORE_GetFileSz(fp));

	BYTE rsv_buff[BIN_RESERVED_SPACE_SZ];
	for (i = 0; i < BIN_RESERVED_SPACE_SZ; ++i)
		rsv_buff[i] = 0x00;
	rsv_buff[0] = BYTE_H(EEPROM_CUSTOM_DATA_ADDR);
	rsv_buff[1] = BYTE_L(EEPROM_CUSTOM_DATA_ADDR);
	fwrite(rsv_buff, sizeof(BYTE), BIN_RESERVED_SPACE_SZ, fp);
	printf(" -ADDED RESERVED SPACE REGION (%u BYTES)\n", sizeof(rsv_buff));

	fwrite(string_buff, sizeof(BYTE), CORE_STRING_COUNT*CORE_STRING_LEN, fp);
	printf(" -ADDED STRINGS REGION (%u BYTES)\n", sizeof(string_buff));

	FILE *fp2 = fopen(BIN_LOGO_FILENAME, "rb");
	if (fp2 == NULL) {
		printf("UNABLE TO OPEN LOGO FILE: " BIN_LOGO_FILENAME "\n");
		return FALSE;
	}
	long logo_file_size = CORE_GetFileSz(fp2);
	if (logo_file_size != ((IMG_WIDTH*IMG_HEIGHT)/8)) {
		printf("UNEXPECTED LOGO FILE SIZE - %lu BYTES\n", logo_file_size);
		return FALSE;
	}
	BYTE bin_logo_buff[logo_file_size];
	fread(bin_logo_buff, sizeof(BYTE), logo_file_size, fp2);
	fclose(fp2);
	fwrite(bin_logo_buff, sizeof(BYTE), logo_file_size, fp);
	fclose(fp);
	printf(" -ADDED LOGO REGION (%lu BYTES)\n", logo_file_size);

	printf("CORE REGIONS PROCESSING COMPLETED\n");
    return TRUE;
}


BOOL ProcessCustomData(void)
{
	const char *str_text_mode[] = {"STANDART","ADVANCED"};
	printf("\n");

	FILE *fp = fopen(CORE_TEXT_FILENAME, "rb");
	if (fp == NULL) {
		printf("UNABLE TO OPEN CUSTOM DATA FILE: " CORE_TEXT_FILENAME "\n");
		return FALSE;
	}

	long src_file_size = CORE_GetFileSz(fp);
	printf("CUSTOM DATA FILE SIZE: %u BYTES\n", src_file_size);
	if (src_file_size == 0) {
		printf("CUSTOM DATA FILE IS EMPTY\n");
		return FALSE;
	}

	FILE *fp2 = fopen(BIN_EEPROM_FILENAME, "ab");
	BYTE cur_code, img_pixel_data = 0, img_bit_index = 0;
	BOOL img_begin = FALSE, first_line_flag = TRUE, advanced_mode = FALSE;
	int img_index = 0, img_pixel_index = 0, img_list_buff_pos = 0, chars_after_img = -1;
	long byte_count = 0;
	BYTE img_list[IMG_LIST_SIZE][IMG_SIZE_BYTES], img_list_pos = 0;
	for (int i = 0; i < src_file_size; ++i) {
		fread(&cur_code, sizeof(BYTE), 1, fp);

		if (cur_code == 0x0D) {
			continue; //skip CR chars
		}

		else if (first_line_flag) { //check extra text mode (ETM)
			if (cur_code == '\n') {
				first_line_flag = FALSE;
				printf("TEXT MODE: %s\n", str_text_mode[(advanced_mode ? 1 : 0)]);
			} else if (cur_code == '1') {
				advanced_mode = TRUE;
			}
			continue;
		} else if ( (advanced_mode) && (cur_code == 0x0A) ) {
			continue; //skip enter symbol for ETM
		} else if ( (advanced_mode) && (cur_code == '_') ) {
			cur_code = ' '; //replace underscores with spaces for ETM
		}
		
		else if (cur_code == IMG_BEGIN_END_CODE) { 
			if (img_begin) {
				img_begin = FALSE;
				++img_list_pos;
				if (img_pixel_index != (IMG_WIDTH*IMG_HEIGHT))
					printf(" WARNING: IMAGE#%i IS TOO SMALL\n", img_index);
				printf(" INFO: IMAGE#%02i PROCESSING END\n", img_index);
				++img_index;
				chars_after_img = 0;
			} else if ( (src_file_size - i) < (IMG_WIDTH*IMG_HEIGHT + 1) ) {
				printf(" WARNING: INSUFFICIENT SPACE FOR IMAGE#%i\n", img_index);
			} else {
				img_begin = TRUE;
				img_pixel_index = 0;
				img_pixel_data = 0;
				img_bit_index = 0;
				img_list_buff_pos = 0;
				cur_code = IMG_FIRST_ID_CHAR + img_list_pos;
				if (cur_code > IMG_LAST_ID_CHAR) {
					printf(" ERROR: TOO MANY IMAGES! MAX:%u\n", IMG_LIST_SIZE);
					return FALSE;
				}
				fwrite(&cur_code, sizeof(BYTE), 1, fp2);
				++byte_count;
				if ( (chars_after_img >= 0) && (chars_after_img < (CORE_STRING_LIM*2)) )
					printf(" WARNING: TOO SHORT DISTANCE BETWEEN TWO IMAGES\n");
				printf(" INFO: IMAGE#%02i PROCESSING BEGIN\n", img_index);
			}
			continue;
		} else if ( (img_begin) && (cur_code != IMG_BLACK_BIT_CODE) && (cur_code != IMG_WHITE_BIT_CODE) ) {
			printf(" WARNING: DETECTED DEFECTIVE CODE FOR IMAGE#%i\n", img_index);
			img_begin = FALSE;
		} else if (img_begin) {
			if (img_pixel_index > (IMG_WIDTH*IMG_HEIGHT)) {
				img_begin = FALSE;
				printf(" WARNING: IMAGE#%i SIZE IS TOO BIG\n", img_index);
			}

			img_pixel_data <<= 1;
			if (cur_code == IMG_BLACK_BIT_CODE)
				img_pixel_data |= 0x01;

			if ((++img_bit_index) == 8) {
				//cur_code = img_pixel_data;
				if ( (img_list_pos >= IMG_LIST_SIZE) || (img_list_buff_pos >= IMG_SIZE_BYTES) ) {
					printf(" ERROR: IMAGE LIST BUFFER OVERFLOW\n");
					return FALSE;
				}
				img_list[img_list_pos][img_list_buff_pos++] = img_pixel_data;
				img_pixel_data = 0;
				img_bit_index = 0;
			}
			++img_pixel_index;
			continue;
		}
		fwrite(&cur_code, sizeof(BYTE), 1, fp2);
		++byte_count;
		++chars_after_img;
	}
	cur_code = '\0'; fwrite(&cur_code, sizeof(BYTE), 1, fp2); //EOF
	++byte_count;

	if (img_list_pos > 0) {
		long empty_space_sz = EEPROM_CUSTOM_DATA_SIZE - (img_list_pos * IMG_SIZE_BYTES) - byte_count;
		if (empty_space_sz < 0) {
			printf("ERROR: NOT ENOUGH SPACE FOR IMAGES\n");
			return FALSE;
		}
		printf("FILLING EEPROM EMPTY SPACE (%lu BYTES)...\n", empty_space_sz);
		while ((empty_space_sz--) > 0) {
			if (empty_space_sz == 0)
				cur_code = 0xFF; //add this code to prevent image part skipping in flashing process
			fwrite(&cur_code, sizeof(BYTE), 1, fp2);
		}

		printf("ADDING IMAGES (%lu BYTES) AT THE END OF FILE...\n", (long)((long)img_list_pos)*IMG_SIZE_BYTES);
		for (int i = 0; i < img_list_pos; ++i)
			fwrite(&img_list[i][0], sizeof(BYTE), IMG_SIZE_BYTES, fp2);
	}

	fclose(fp);
	printf("CUSTOM DATA REGION SIZE: %lu BYTES\nTEXT PROCESSING COMPLETED\nTOTAL FILE SIZE: %lu BYTES\n", (long)(byte_count + (((long)img_list_pos) * IMG_SIZE_BYTES)), CORE_GetFileSz(fp2));
	fclose(fp2);

	return TRUE;
}


int main(int argc, char *argv[])
{
	CORE_PrintAppCaption(APP_NAME, APP_VERSION);

	if (!ProcessCoreRegions())
		printf("ERROR: CORE STRINGS PROCESSING FAILED\n");
	else if (!ProcessCustomData())
		printf("ERROR: CUSTOM DATA PROCESSING FAILED\n");
	else
		printf("\nBINARY FILE SUCCESSFULLY GENERATED\n");

	printf("\n");
	system("PAUSE");
    return 0;
}

