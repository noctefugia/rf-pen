#ifndef __CORE_H
#define __CORE_H

#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <io.h>
#define F_OK 0
#define access _access

#define MAX_LINE 255
#define LINE_FORMAT "%255s"

#define IMG_WIDTH 128
#define IMG_HEIGHT 32
#define IMG_SIZE_BYTES ((IMG_WIDTH*IMG_HEIGHT)/8)
#define IMG_BEGIN_END_CODE 0xB6 //pilcrow
#define IMG_BLACK_BIT_CODE 0x30 //0
#define IMG_WHITE_BIT_CODE 0x31 //1
#define IMG_FIRST_ID_CHAR 0x0B
#define IMG_LAST_ID_CHAR 0x1F
#define IMG_LIST_SIZE (IMG_LAST_ID_CHAR - IMG_FIRST_ID_CHAR + 1)

#define FONT_IMG_WIDTH 225
#define FONT_IMG_HEIGHT 73
#define FONT_CHAR_W 6
#define FONT_CHAR_H 8
#define FONT_OUTPUT_BUFF_SZ (((UINT32)(0x100*FONT_CHAR_W*FONT_CHAR_H)) / (UINT32)8)

#define CORE_STRING_COUNT 52
#define CORE_STRING_LEN 21
#define CORE_STRING_LIM (CORE_STRING_LEN * 4)

#define CORE_LOGO_FILENAME "core_logo.bmp"
#define CORE_FONT_FILENAME "core_font.bmp"
#define CORE_STRINGS_FILENAME "core_strings.ini"
#define CORE_TEXT_FILENAME "custom_data.txt"
#define CORE_NOTES_FILENAME "custom_notes.txt"
#define BIN_FONT_FILENAME "core_font.bin"
#define BIN_LOGO_FILENAME "core_logo.bin"
#define BIN_EEPROM_FILENAME "eeprom.bin"
#define BIN_EEPROM_DUMP_FILENAME "eeprom_dump.bin"
#define BIN_RESERVED_SPACE_SZ 32

#define SERIAL_BUFFER_SIZE 150
#define EEPROM_MEM_SZ 0xFFFF
#define EEPROM_NOTES_REGION_SZ 4096
#define EEPROM_PAGE_SZ 128
#define EEPROM_NOTES_REGION_ADDR ((EEPROM_MEM_SZ + 1) - EEPROM_NOTES_REGION_SZ)
#define EEPROM_CUSTOM_DATA_ADDR 0x0C64
#define EEPROM_CUSTOM_DATA_SIZE (EEPROM_NOTES_REGION_ADDR - EEPROM_CUSTOM_DATA_ADDR)

/* DEVICE CONFIG DEFAULTS */
#define DCD_TIMER_MIN 5
#define DCD_VIBRO_EN 1
#define DCD_EFFECTS_EN 1
#define DCD_OLED_CONTRAST 188
#define DCD_OLED_AUTOSLP 30
#define DCD_OLED_ROTATION 0
#define DCD_RF_CHANNEL 100
#define DCD_RF_ADDRESS 250
#define DCD_RF_EN 0
#define DCD_NOTES_DM_PAGE 0


/* UTILS */
#define BYTE_L(val) ((BYTE)((val) & (BYTE)0xFF))
#define BYTE_H(val) ((BYTE)(BYTE_L((val) >> (BYTE)8)))
#define WORD_HL(value_h, value_l) ((UINT16)((((UINT16)value_h) << 8) | value_l))


void CORE_PrintAppCaption(const char *app_name, unsigned int app_ver);
BOOL CORE_FileExists(const char *filename);
void CORE_ClipboardSetText(const void *buff, size_t buff_sz);
long CORE_GetFileSz(FILE *fp);
void CORE_CRC16(UINT16 *crc, BYTE data);

#endif /* __CORE_H */
