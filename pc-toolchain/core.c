#include "core.h"


void CORE_PrintAppCaption(const char *app_name, unsigned int app_ver)
{
	char name_buff[MAX_LINE], name_buff2[MAX_LINE];
	int i, name_len;

	snprintf(name_buff, MAX_LINE, "* %s TOOL FOR SSD%ux%u OLED v%u *\n", app_name, IMG_WIDTH, IMG_HEIGHT, app_ver);
	name_len = strlen(name_buff) - 1;
	if (name_len > MAX_LINE - 3)
		name_len = MAX_LINE - 3;
	for (i = 0; i < name_len; ++i)
		name_buff2[i] = '*';
	name_buff2[name_len] = '\n';
	name_buff2[name_len+1] = '\0';
	printf(name_buff2); printf(name_buff); printf("%s\n", name_buff2);
}


BOOL CORE_FileExists(const char *filename)
{
	BOOL state;

	state = (access(filename, F_OK) == 0) ? TRUE : FALSE;
	return state;
}


void CORE_ClipboardSetText(const void *buff, size_t buff_sz)
{
	HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, buff_sz);
	memcpy(GlobalLock(hMem), buff, buff_sz);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
	GlobalFree(hMem);
}


long CORE_GetFileSz(FILE *fp)
{
	fseek(fp, 0, SEEK_END);
	long file_sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	return file_sz;
}


void CORE_CRC16(UINT16 *crc, BYTE data)
{
	BYTE i;
	
	*crc ^= data;
	for (i = 0; i < 8; ++i)
		*crc = (*crc & 1) ? ((*crc >> 1) ^ 0xA001) : (*crc >> 1);
}
