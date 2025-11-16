#include "oled.h"
#include "i2c_soft.h"
#include "eeprom.h"

@near static uint8_t oled_buffer[OLED_SIZE_Y][OLED_SIZE_X];
struct oled_struct oled;


void OLED_SetChar(uint8_t char_code, uint8_t x, uint8_t y)
{
	assert_param(IS_OLED_BUFF_X_OK(x));
	assert_param(IS_OLED_BUFF_Y_OK(y));
	
	if (oled.rotation) {
		x = (OLED_SIZE_X - 1) - x;
		y = (OLED_SIZE_Y - 1) - y;
	}
	oled_buffer[y][x] = char_code;
}


uint8_t OLED_GetChar(uint8_t x, uint8_t y)
{
	assert_param(IS_OLED_BUFF_X_OK(x));
	assert_param(IS_OLED_BUFF_Y_OK(y));
	
	if (oled.rotation) {
		x = (OLED_SIZE_X - 1) - x;
		y = (OLED_SIZE_Y - 1) - y;
	}
	return oled_buffer[y][x];
}


void OLED_PrintNumber(int32_t number, uint8_t reverse_x, uint8_t y)
{
	uint8_t v, nf;
	
	nf = 1;
	if (number < 0) {
		number = -number;
		nf = 2; //negative flag
	}
	
	do {
		v = DIGIT_TO_ASCII((uint8_t)(number % 10));
		number /= 10;
opn_set_char:
		OLED_SetChar(v, reverse_x--, y);
	} while (number > 0);
		
	if (--nf) { //shitty style to reduce code size
		v = '-';
		goto opn_set_char;
	}
}


void OLED_PrintString(OLED_STR_TypeDef id, uint8_t y)
{
	uint8_t str_buff[OLED_STRING_BUFF_SZ], i, x;
	
	assert_param(IS_OLED_STR_ID_OK(id));
	assert_param(IS_OLED_BUFF_Y_OK(y));
	
	assert_function((EEPROM_Read(OLED_CORE_STRINGS_ADDR + id*OLED_STRING_BUFF_SZ, OLED_STRING_BUFF_SZ, str_buff) == I2CSS_ACK));
	x = 0;
	for (i = 0; i < OLED_STRING_BUFF_SZ; ++i) {
		if ( (str_buff[i] == OLED_ECHAR_STR_EOL) && (i > 0) )
			return;
		OLED_SetChar(str_buff[i], x++, y);
	}
}


void OLED_DrawImage(uint16_t addr)
{
	oled.img_adddr = addr;
	//OLED_Redraw();
}


void OLED_Redraw(void)
{
	oled.redraw_flag = TRUE;
}


void OLED_StartLine(void)
{
	I2CS_Start();
	I2CS_WriteByte(OLED_I2C_ADDR);
	I2CS_WriteByte(OLED_SET_START_LINE);
}


void OLED_InverseArea(struct pos2d_struct *pos_ptr, uint8_t size)
{
	oled.inv_pos.x = pos_ptr->x;
	oled.inv_pos.y = pos_ptr->y;
	if (oled.rotation) {
		oled.inv_pos.x = (OLED_SIZE_X - 1) - oled.inv_pos.x - size + 1;
		oled.inv_pos.y = (OLED_SIZE_Y - 1) - oled.inv_pos.y;
	}
	oled.inv_area_sz = size;
}


void OLED_PrintStringList(uint8_t start_y, OLED_STR_TypeDef str_begin, OLED_STR_TypeDef str_end)
{
	uint8_t i, n;
	
	n = start_y;
	for (i = str_begin; i < str_end; ++i)
		OLED_PrintString((OLED_STR_TypeDef)i, n++);
}


void OLED_Update(void)
{
	uint8_t block_no, x, y, j, char_buff[OLED_CHAR_WIDTH], img_buff[OLED_IMAGE_BUFF_SZ], inv_bytes, cur_byte, cur_index;
	uint16_t i, cur_addr;
	
	if (!oled.redraw_flag)
		return;
	oled.redraw_flag = FALSE;	//reset flag at start if somefuck it changed by another thread while rendering process

	OLED_Command(OLED_COLUMN_ADDR);
	OLED_Command(0x00);
	OLED_Command(OLED_WIDTH_PX - 1);
	OLED_Command(OLED_PAGE_ADDR);
	OLED_Command(0x00);
	OLED_Command(0x03);
	
	block_no = x = y = inv_bytes = 0;

	if (oled.img_adddr != 0xFFFF) {
		for (i=0; i < OLED_BYTE_COUNT; i += OLED_IMAGE_BUFF_SZ) {
			cur_addr = (oled.rotation) ? (oled.img_adddr + OLED_BYTE_COUNT - OLED_IMAGE_BUFF_SZ - i) : (oled.img_adddr + i);
			assert_function((EEPROM_Read(cur_addr, OLED_IMAGE_BUFF_SZ, img_buff) == I2CSS_ACK));
			OLED_StartLine();
			for (j = 0; j < OLED_IMAGE_BUFF_SZ; ++j) {
				cur_index = (oled.rotation) ? ((OLED_IMAGE_BUFF_SZ - 1) - j) : j;
				cur_byte = (oled.img_adddr == OLED_FHASHLIGHT_KEYWORD) ? 0xFF : img_buff[cur_index];
				if (oled.rotation)
					cur_byte = ReverseBitOrder(cur_byte);
				assert_function((I2CS_WriteByte(cur_byte) == I2CSS_ACK));
			}
			I2CS_Stop();
		}
	}
	
	else {
		for (i=0; i < OLED_BYTE_COUNT; ++i) {
			assert_function((EEPROM_Read(OLED_EEPROM_FONT_ADDR + oled_buffer[y][x]*OLED_CHAR_WIDTH, OLED_CHAR_WIDTH, char_buff) == I2CSS_ACK));
			if ( (oled.inv_area_sz > 0) && (y == oled.inv_pos.y) && (x == oled.inv_pos.x) )
				inv_bytes = oled.inv_area_sz;
			else if (inv_bytes > 0)
				--inv_bytes;
			OLED_StartLine();
			for (j = 0; j < OLED_CHAR_WIDTH; ++j) {
				cur_index = (oled.rotation) ? ((OLED_CHAR_WIDTH - 1) - j) : j;
				cur_byte = char_buff[cur_index]; //reduce code size by 7 bytes!
				if (oled.rotation)
					cur_byte = ReverseBitOrder(cur_byte);
				if (inv_bytes > 0)
					cur_byte = ~cur_byte;
				assert_function((I2CS_WriteByte(cur_byte) == I2CSS_ACK));
			}
			++x;
			i += (OLED_CHAR_WIDTH - 1);
			block_no += 1;
			
			if (block_no > (OLED_SIZE_X - 1)) {
				for (j = 0; j < (OLED_WIDTH_PX - (OLED_SIZE_X * OLED_CHAR_WIDTH)); ++j) {
					I2CS_WriteByte(0x00);
					++i;
				}
				block_no = 0;
				++y;
				x = 0;
			}
			I2CS_Stop();
		}
	}
}
	
	
void OLED_Clear(void)
{
	uint8_t x, y;
	
	oled.redraw_flag = FALSE;
	oled.img_adddr = 0xFFFF;
	oled.inv_area_sz = 0;
	
	for (y=0; y < OLED_SIZE_Y; ++y)
		for (x=0; x < OLED_SIZE_X; ++x)
			oled_buffer[y][x] = OLED_EEPROM_FONT_ADDR + ' '; //fill with spaces
}


void OLED_Init(OLED_CONTRAST_TypeDef lvl, bool rot)
{
	OLED_Command(OLED_DISPLAY_OFF);
	OLED_Command(OLED_SET_DISPLAY_CLOCK_DIV);
	OLED_Command(0xF0);
	OLED_Command(OLED_SET_MULTIPLEX);
	OLED_Command(OLED_HEIGHT_PX - 1);
	OLED_Command(OLED_SET_DISPLAY_OFFSET);
	OLED_Command(0x00);
	OLED_Command(OLED_SET_START_LINE);
	OLED_Command(OLED_CHARGE_PUMP);
	OLED_Command(0x14);
	OLED_Command(OLED_MEMORY_MODE);
	OLED_Command(0x00);
	OLED_Command(OLED_SEG_REMAP);
	OLED_Command(OLED_COM_SCAN_INC);
	OLED_Command(OLED_SET_COMPINS);
	OLED_Command(0x02);
	
	OLED_Command(OLED_SET_CONTRAST);
	OLED_Command((uint8_t)lvl); //560k, ~17uA, 188 = MAX
	
	OLED_Command(OLED_SET_PRECHARGE);
	OLED_Command(0xF1);
	if (lvl == OLEDCT_LOW) { //reduce brightness more
		OLED_Command(OLED_SET_VCOM_DETECT); //0xDB, (additionally needed to lower the contrast)
		OLED_Command(0x00);	        //0x20 def, 0x40 prev, to lower the contrast, put 0
	}
	OLED_Command(OLED_DISPLAY_ALL_ON_RESUME);
	OLED_Command(OLED_NORMAL_DISPLAY);
	OLED_Command(0x2e);            // stop scroll
	OLED_Command(OLED_DISPLAY_ON);
	
	//OLED_Clear();
	//OLED_Redraw();
	oled.rotation = rot;
	oled.redraw_flag = FALSE;
	}	


void OLED_Command(uint8_t cmd)
{
	I2CS_Start();
	
	I2CS_WriteByte((uint8_t)(OLED_I2C_ADDR | 0x00));
	I2CS_WriteByte(0x80);
	assert_function((I2CS_WriteByte(cmd) == I2CSS_ACK));
	
	I2CS_Stop();
}


uint8_t OLED_ReadByte(uint8_t addr)
{   
	uint8_t data;

	I2CS_Start();
	I2CS_WriteByte((uint8_t)(OLED_I2C_ADDR | 0x00));
	I2CS_WriteByte(addr);
	
	I2CS_Start();
	assert_function((I2CS_WriteByte((uint8_t)(OLED_I2C_ADDR | 0x01)) == I2CSS_ACK));

	data = I2CS_ReadByte(I2CSS_NO_ACK);
	I2CS_Stop();

	return data;
}