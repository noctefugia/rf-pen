#include "core.h"

#define APP_NAME "EEPROM FLASHER"
#define APP_VERSION 1000


enum {
	CMDI_VERSION = 1,			//get firmware ver
	CMDI_WRITE_PAGE,			//write single page to eeprom
	CMDI_RESET,					//reset device
	CMDI_EEPROM_BYTE,			//read byte from EEPROM
	CMDI_USB_MODE,				//switch device to USB mode
	CMDI_WRITE_CONFIG,			//write to internal device EEPROM
	CMDI_UNDEFINED = 0xFF 		//place before this flag
};

enum {
	CMDO_VERSION = 1,			//return firmware ver
	CMDO_PAGE_CRC,				//last written page CRC
	CMDO_EEPROM_BUSY,			//cannot access eeprom now
	CMDO_EEPROM_BYTE,			//byte read from eeprom
	CMDO_OK,					//command ok
	CMDO_UNDEFINED = 0xFF	//place before this flag
};

HANDLE g_hCom = NULL;
BOOL reset_mode = FALSE, dnotes_erase_mode = FALSE, dnotes_read_mode = FALSE, config_reset_mode = FALSE, eeprom_dump_mode = FALSE;


void PrintCommState(DCB dcb, LPCTSTR name)
{
	printf_s("%s\r\nBaudRate = %d\r\nByteSize = %d\r\nParity = %d\r\nStopBits = %d\r\n",
		name,
		dcb.BaudRate, 
		dcb.ByteSize, 
		dcb.Parity,
		dcb.StopBits);
}


void ResetComState(void)
{
	if ( (g_hCom == NULL) || (!PurgeComm(g_hCom, PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR)) )
		printf("FAILED TO RESET COM BUFFER\n");
}


BOOL OpenComPort(HANDLE *hCom, LPCSTR port_name, DWORD bdr, BYTE byte_sz, BYTE parity, BYTE stop_bits)
{
	DCB dcb;
	BOOL fSuccess;

	//  Open a handle to the specified com port.
	*hCom = CreateFile(port_name,
		GENERIC_READ | GENERIC_WRITE,
		0,      //  must be opened with exclusive-access
		NULL,   //  default security attributes
		OPEN_EXISTING, //  must use OPEN_EXISTING
		0,      //  not overlapped I/O
		NULL ); //  hTemplate must be NULL for comm devices

	if (*hCom == INVALID_HANDLE_VALUE) {
		printf("CreateFile failed, err=%u", GetLastError());
		return FALSE;
	}

	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	fSuccess = GetCommState(*hCom, &dcb);
	if (!fSuccess) {
		printf("GetCommState failed, err=%u", GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}
	//PrintCommState(dcb, TEXT("Current config:"));

	/* NEW */
	fSuccess = SetupComm(*hCom, SERIAL_BUFFER_SIZE, SERIAL_BUFFER_SIZE);
	if (!fSuccess) {
		printf("SetupComm failed, err=%u", GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}

	COMMTIMEOUTS comt;
	comt.ReadIntervalTimeout=2; 
	comt.ReadTotalTimeoutMultiplier=1; 
	comt.ReadTotalTimeoutConstant=10; 
	comt.WriteTotalTimeoutMultiplier=1; 
	comt.WriteTotalTimeoutConstant=10; 
	fSuccess = SetCommTimeouts(*hCom, &comt);
	if (!fSuccess) {
		printf("SetCommTimeouts failed, err=%u", GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}
	/* NEW */

	dcb.BaudRate = bdr;
	dcb.ByteSize = byte_sz;
	dcb.Parity   = parity;
	dcb.StopBits = stop_bits;

	fSuccess = SetCommState(*hCom, &dcb);
	if (!fSuccess) {
		printf("SetCommState failed, err=%u", GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}

	fSuccess = GetCommState(*hCom, &dcb);
	if (!fSuccess) {
		printf("GetCommState failed, err=%u", GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}
	//PrintCommState(dcb, TEXT("New config:")); 

	/* NEW */
	ResetComState();
	/* NEW */

	return TRUE;
}


BOOL WriteComMessage(BYTE msg_no, BYTE *buff, BYTE buff_sz)
{
	BYTE i;

	DWORD out_buff_sz = buff_sz + 4;
	BYTE out_buff[out_buff_sz];
	out_buff[0] = buff_sz + 1; //msg size
	out_buff[1] = msg_no; //msg no
	if (buff_sz > 0) {
		for (i = 0; i < buff_sz; ++i)
			out_buff[2+i] = buff[i];
	}

	UINT16 crc = 0;
	for (i = 0; i < (out_buff_sz - 2); ++i)
		CORE_CRC16(&crc, out_buff[i]);
	out_buff[out_buff_sz-2] = BYTE_H(crc);
	out_buff[out_buff_sz-1] = BYTE_L(crc);

	DWORD error_flags; COMSTAT coms;
	ClearCommError(g_hCom, &error_flags, &coms); 
	ResetComState();

	BOOL fSuccess = WriteFile(g_hCom, out_buff, out_buff_sz, &out_buff_sz, NULL); 
	if (!fSuccess)
		printf("ERROR: SERIAL PORT WRITE FAIL; ERR=%u\n", GetLastError());

	return fSuccess;
}


BOOL ReadComMessage(BYTE *buff, BYTE *buff_sz)
{
	BYTE i;

	DWORD error_flags, nbytes_read; COMSTAT coms;
	ClearCommError(g_hCom, &error_flags, &coms); 

	*buff_sz = 0;
	BYTE in_buff_sz = SERIAL_BUFFER_SIZE, in_buff[in_buff_sz];
	BOOL fSuccess = ReadFile(g_hCom, in_buff, in_buff_sz, &nbytes_read, NULL); 
	if (!fSuccess) { 
		printf("ERROR: SERIAL PORT READ FAIL; ERR=%u", GetLastError());
		return FALSE;
	}

	UINT16 crc = 0, crc2;
	if ( (nbytes_read > 3) && (nbytes_read <= SERIAL_BUFFER_SIZE) ) {
		for (i = 1; i < nbytes_read - 2; ++i)
			buff[i-1] = in_buff[i];
		*buff_sz = nbytes_read - 3;

		crc = 0;
		for (i = 0; i < (nbytes_read - 2); ++i)
			CORE_CRC16(&crc, in_buff[i]);
		crc2 = (UINT16)(((UINT16)(in_buff[nbytes_read - 2] << 8)) | ((UINT16)in_buff[nbytes_read - 1]));
		if (crc == crc2)
			return TRUE;
		else
			printf("ERROR: WRONG COM-MSG CRC\n");
	} 
	
	else if (nbytes_read != 0) {
		printf("ERROR: UNEXPECTED SERIAL INPUT BEFFER SIZE\n");
	}

	return FALSE;
}


BOOL TestDevicePort(void)
{
	if (!WriteComMessage(CMDI_VERSION, NULL, 0))
		return FALSE;
	Sleep(200);

	BYTE buff_sz = SERIAL_BUFFER_SIZE, buff[buff_sz];
	if ( (ReadComMessage(buff, &buff_sz)) && (buff_sz == 3) && (buff[0] == CMDO_VERSION) ) {
		printf("DEVICE VERSION: %u.%02u\n", buff[1], buff[2]);
		return TRUE;
	}

	return FALSE;
}


BOOL SearchDevicePort(UINT max_port)
{
	char cur_port_name[8];
	COMMCONFIG cc;
	DWORD dwSize = sizeof(COMMCONFIG);

	for (UINT i=1; i<max_port; i++) {
		sprintf_s(cur_port_name, 8, "COM%u", i);
		
		if (GetDefaultCommConfig(cur_port_name, &cc, &dwSize)) {
			printf("DETECTED PORT: %s\n", cur_port_name);
			if (OpenComPort(&g_hCom, cur_port_name, CBR_115200, 8, NOPARITY, ONESTOPBIT)) {
				printf("CONNECTED TO: %s\n", cur_port_name);
				if (TestDevicePort()) {
					printf("DEVICE CONNECTED TO PORT %s\n", cur_port_name);
					return TRUE;
				} else {
					CloseHandle(g_hCom);
					printf("NO VALID RESPONSE FROM PORT %s\n", cur_port_name);
				}
			}
		}
	}

	return FALSE;
}


BOOL DeviceReset(BOOL usb_mode)
{
	BOOL state = WriteComMessage(CMDI_RESET, NULL, 0);
	if (usb_mode) {
		printf("RESTARTING DEVICE IN USB MODE...\n");
		Sleep(2500);
		WriteComMessage(CMDI_USB_MODE, NULL, 0);
		Sleep(200);
	}

	return state;
}


BOOL DeviceWritePage(UINT32 *addr, BYTE *page_buff)
{
	int i;
	BYTE buff_sz = SERIAL_BUFFER_SIZE, buff[SERIAL_BUFFER_SIZE];
	UINT16 crc_page, crc2_page;
	static unsigned int page_no = 0, empty_pages = 0;
	BOOL zero_flag;

	buff[0] = BYTE_H(*addr); //addr_h
	buff[1] = BYTE_L(*addr); //addr_l
	crc_page = 0;
	zero_flag = TRUE;
	for (i = 0; i < EEPROM_PAGE_SZ; ++i) {
		buff[2+i] = page_buff[i];
		CORE_CRC16(&crc_page, buff[2+i]);
		if ( (zero_flag) && (buff[2+i] != 0x00) )
			zero_flag = FALSE;
	}

	/* flashing process optimization begin */
	if (zero_flag) {
		++empty_pages;
		if (empty_pages > (IMG_SIZE_BYTES / EEPROM_PAGE_SZ)) { 
			printf("PAGE#%03u: ADDR:%04X EMPTY:1 SKIPPING...\n", page_no, *addr);
			*addr += EEPROM_PAGE_SZ;
			++page_no;
			return TRUE;
		} 
	} else {
		empty_pages = 0;
	}
	/* flashing process optimization end */

	if (!WriteComMessage(CMDI_WRITE_PAGE, buff, 134))
		return FALSE;
	Sleep(50);
	if (!ReadComMessage(buff, &buff_sz)) 
		return FALSE;

	if (buff_sz == 0) {
		printf("NO RESPONSE FROM DEVICE\n");
		return FALSE;
	} else if (buff[0] == CMDO_PAGE_CRC) {
		crc2_page = WORD_HL(buff[1], buff[2]);
		printf("PAGE#%03u: ADDR:%04X EMPTY:%u CRC1=0x%04X CRC2=0x%04X", page_no, *addr, (BYTE)(zero_flag ? 1 : 0), crc_page, crc2_page);
		if (crc_page == crc2_page) {
			printf(" - CRC OK\n");
			if (i < (EEPROM_MEM_SZ - EEPROM_NOTES_REGION_SZ - EEPROM_PAGE_SZ)) {
				*addr += EEPROM_PAGE_SZ;
				++page_no;
				return TRUE;
			} else {
				printf("WARNING: CUSTOM DATA FILE IS TOO BIG\n");
				return FALSE;
			}
		} else {
			printf(" - WRONG CRC\n");
			return FALSE;
		}
	} else {
		printf("INVALID RESPONSE FROM DEVICE\n");
		return FALSE;
	}
}


BOOL DeviceOperationEEPROM(void)
{
	int i;
	BYTE page_buff[EEPROM_PAGE_SZ];
	UINT32 page_addr;

	if (dnotes_read_mode || eeprom_dump_mode) {
		if (!DeviceReset(TRUE))
			return FALSE;
		
		FILE *fp;
		char read_fn[MAX_LINE];
		if (dnotes_read_mode)
			i = strcpy_s(read_fn, MAX_LINE, CORE_NOTES_FILENAME);
		else
			i = strcpy_s(read_fn, MAX_LINE, BIN_EEPROM_DUMP_FILENAME);
		remove(read_fn);
		fp = fopen(read_fn,"wb");

		BYTE read_buf_sz;
		UINT16 read_progress = 0;
		UINT32 target_region_sz = dnotes_read_mode ? EEPROM_NOTES_REGION_SZ : EEPROM_MEM_SZ;
		UINT32 read_max_addr = dnotes_read_mode ? EEPROM_MEM_SZ : (EEPROM_MEM_SZ + 1);
		page_addr = dnotes_read_mode ? EEPROM_NOTES_REGION_ADDR : 0;
		i = 0;
		while (page_addr < read_max_addr) {
			page_buff[0] = BYTE_H(page_addr); //addr_h
			page_buff[1] = BYTE_L(page_addr); //addr_l
			++page_addr;

			if (!WriteComMessage(CMDI_EEPROM_BYTE, page_buff, 2))
				return FALSE;
			Sleep(5);
			if (!ReadComMessage(page_buff, &read_buf_sz))
				return FALSE;

			if ( (read_buf_sz == 2) && (page_buff[0] == CMDO_EEPROM_BYTE) ) {
				fwrite(&page_buff[1], sizeof(BYTE), 1, fp);
			} else {
				printf("ERROR: UNABLE TO READ DATA AT ADDR=%04X\n", page_addr);
				return FALSE;
			}
			if ((++i) % CORE_STRING_LEN == 0) {
				if (dnotes_read_mode) {
					page_buff[1] = '\n';
					fwrite(&page_buff[1], sizeof(BYTE), 1, fp);
				}
				if ((i % (CORE_STRING_LEN*3)) == 0)
					printf("\tprogress: %u\\%u\n", ++read_progress, (target_region_sz/(CORE_STRING_LEN*3)));
			}
		}
		fclose(fp);
		printf("READ PROCESS FINISHED\n");
	}

	else if (dnotes_erase_mode) {
		if (!DeviceReset(TRUE))
			return FALSE;
		
		for (i = 0; i < EEPROM_PAGE_SZ; ++i)
			page_buff[i] = ' ';

		page_addr = EEPROM_NOTES_REGION_ADDR;
		while (page_addr < EEPROM_MEM_SZ) {
			if (!DeviceWritePage(&page_addr, page_buff))
				return FALSE;
		}
		printf("ERASE PROCESS FINISHED\n");
	} 
	
	else if (config_reset_mode) {
		if (!DeviceReset(TRUE))
			return FALSE;
		
		BYTE read_buf_sz;
		BYTE config_buff[10] = {
			DCD_TIMER_MIN,		DCD_VIBRO_EN,		DCD_EFFECTS_EN,
			DCD_OLED_CONTRAST,	DCD_OLED_AUTOSLP,	DCD_OLED_ROTATION,
			DCD_RF_CHANNEL,		DCD_RF_ADDRESS,		DCD_RF_EN, DCD_NOTES_DM_PAGE
		};
		page_buff[0] = 0;
		for (i = 0; i < 10; ++i) {
			page_buff[0] = i;
			page_buff[1] = config_buff[i];
			if (!WriteComMessage(CMDI_WRITE_CONFIG, page_buff, 2))
				return FALSE;
			Sleep(5);
			if (!ReadComMessage(page_buff, &read_buf_sz))
				return FALSE;
			if ( (read_buf_sz == 1) && (page_buff[0] == CMDO_OK) ) {
				printf("BYTE#%02u VALUE:%u\n", i, config_buff[i]);
			} else {
				printf("ERROR: NO RESPONSE FROM DEVICE\n");
				return FALSE;
			}
		}
		printf("CONFIG RESET PROCESS FINISHED\n");
	} 

	else if (!reset_mode) {
		FILE *fp = fopen(BIN_EEPROM_FILENAME, "rb");
		if (fp == NULL) {
			printf("UNABLE TO OPEN EEPROM FILE: " BIN_EEPROM_FILENAME "\n");
			return FALSE;
		}

		long eeprom_file_size = CORE_GetFileSz(fp);
		if (eeprom_file_size < ( ((IMG_WIDTH*IMG_HEIGHT)/8) + ((FONT_IMG_WIDTH*FONT_IMG_HEIGHT)/8) ) ) {
			printf("UNEXPECTED EEPROM FILE SIZE - %i BYTES\n", eeprom_file_size);
			return FALSE;
		}
		printf("EEPROM FILE SIZE: %i BYTES\n", eeprom_file_size);
		BYTE eeprom_buff[eeprom_file_size];
		fread(eeprom_buff, sizeof(BYTE), eeprom_file_size, fp);
		fclose(fp);
		printf("EEPROM FILE LOADED\nSTARTING FLASHING PROCESS...\n");

		if (!DeviceReset(TRUE))
			return FALSE;

		page_addr = 0x0000;
		long eeprom_buff_pos = 0;
		BOOL eeprom_buff_end = FALSE;
		while (!eeprom_buff_end) {
			for (i = 0; i < EEPROM_PAGE_SZ; ++i) {
				if (eeprom_buff_pos < eeprom_file_size) {
					page_buff[i] = eeprom_buff[eeprom_buff_pos++];
				} else if (i == 0) {
					goto flash_exit;
				} else {
					page_buff[i] = 0x00;
					eeprom_buff_end = TRUE;
				}
			}
			if (!DeviceWritePage(&page_addr, page_buff))
				return FALSE;
		}
flash_exit:
		printf("FLASHING PROCESS FINISHED\n");
	}

	printf("RESETTING DEVICE...\n");
	Sleep(100);
	if (!DeviceReset(FALSE))
		return FALSE;

	return TRUE;
}


int main(int argc, char *argv[])
{
	CORE_PrintAppCaption(APP_NAME, APP_VERSION);

	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset_mode = TRUE;
			printf("RESET DEVICE ROUTINE\n");
		} else if (strcmp(argv[1], "erase_dnotes") == 0) {
			dnotes_erase_mode = TRUE;
			printf("ERASE DYNAMIC NOTES ROUTINE\n");
		} else if (strcmp(argv[1], "read_dnotes") == 0) {
			dnotes_read_mode = TRUE;
			printf("READ DYNAMIC NOTES ROUTINE\n");
		} else if (strcmp(argv[1], "reset_config") == 0) {
			config_reset_mode = TRUE;
			printf("CONFIG RESET ROUTINE\n");
		} else if (strcmp(argv[1], "eeprom_dump") == 0) {
			eeprom_dump_mode = TRUE;
			printf("EEPROM DUMP ROUTINE\n");
		}
	}

	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	UINT tim_res = (tc.wPeriodMin < 10) ? 10 : tc.wPeriodMin;
	timeBeginPeriod(tim_res);
	printf("SET SYSTEM TIMER RESOLUTION TO: %uMS\n", tim_res);

	printf("SEARCHING FOR AVAILABLE SERIAL PORTS...\n");
    if (SearchDevicePort(0xFF)) {
		printf("SEARCHING COMPLETED\n\n");
		if (DeviceOperationEEPROM())
			printf("\nOPERATION SUCCESSFULLY COMPLETED\n");
		else
			printf("ERROR: OPERATION FAILED\n");
	} else {
		printf("ERROR: DEVICE NOT DETECTED\n");
	}

	if (g_hCom)
		CloseHandle(g_hCom);
	timeEndPeriod(tim_res);

	printf("\n");
	system("PAUSE");
    return 0;
}

