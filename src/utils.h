#ifndef __UTILS_H
#define __UTILS_H

#include "stm8s.h"

/* KEYWORD DEFINITIONS -------------------------------------------------------*/
#define LOW RESET
#define HIGH SET
#define NULL (0)

typedef enum {
	IO_High,
	IO_Low,
	IO_Reverse,
	IO_Invalid
} IO_MODE_TypeDef;


/* CORE DEFINITIONS -----------------------------------------------------------*/
#define CORE_VER_MAJOR 1
#define CORE_VER_MINOR 0
#define CORE_FCPU 16000000	//8MHz - cpu freq
#define CORE_PERIOD 50			//50ms - update period
#define PCB_REVISION 1

/* MACRO DEFINITIONS ---------------------------------------------------------*/
#define BYTE_L(val) ((uint8_t)((val) & (uint8_t)0xFF))
#define BYTE_H(val) ((uint8_t)(BYTE_L((val) >> (uint8_t)8)))
#define WORD_HL(value_h, value_l) ((uint16_t)((((uint16_t)value_h) << 8) | value_l))
#define DWORD_HML(value_h, value_m, value_l) ((uint32_t)((((uint32_t)value_h) << 16) | (((uint16_t)value_m) << 8) | value_l))
#define TICKS8(val) ((uint8_t)((uint16_t)(val) / (uint16_t)CORE_PERIOD))
#define TICKS16(val) ((uint16_t)((uint16_t)(val) / (uint16_t)CORE_PERIOD))
#define MS16(val) ((uint16_t)((uint16_t)(val) * (uint16_t)CORE_PERIOD))
#define DIGIT_TO_ASCII(val) ((uint8_t)(0x30 + (val)))
#define DIGIT_FROM_ASCII(val) ((uint8_t)((val) - 0x30))
#ifdef USE_FULL_ASSERT
	#define assert_function(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
	#define assert_function(expr) (expr)
#endif

/* PARAMETERS CHECKING --------------------------------------------------*/
#define IS_UTIL_IO_MODE_OK(io_mode) \
	((io_mode) < (IO_Invalid)) 

/* PIN DEFINITIONS -----------------------------------------------------------*/
#define PORT_LED_A GPIOA
#define PIN_LED_A GPIO_PIN_3
#define PORT_VIBRO_MOT GPIOA
#define PIN_VIBRO_MOT GPIO_PIN_3

#define PORT_BTN_UP GPIOA
#define PIN_BTN_UP GPIO_PIN_2
#define PORT_BTN_DOWN GPIOA
#define PIN_BTN_DOWN GPIO_PIN_1

#define PORT_I2C_SDA GPIOB
#define PIN_I2C_SDA GPIO_PIN_5
#define PORT_I2C_SCL GPIOB
#define PIN_I2C_SCL GPIO_PIN_4

#define PORT_UART_TX GPIOD
#define PIN_UART_TX GPIO_PIN_5
#define PORT_UART_RX GPIOD
#define PIN_UART_RX GPIO_PIN_6

#define PORT_VOLT_MON GPIOC
#define PIN_VOLT_MON GPIO_PIN_3

#define PORT_SPI_MISO GPIOD
#define PIN_SPI_MISO GPIO_PIN_2
#define PORT_SPI_MOSI GPIOC
#define PIN_SPI_MOSI GPIO_PIN_7
#define PORT_SPI_SCK GPIOC
#define PIN_SPI_SCK GPIO_PIN_6
#define PORT_SPI_CS GPIOC
#define PIN_SPI_CS GPIO_PIN_5

#define PORT_NRF24_CE_RXTX GPIOC
#define PIN_NRF24_CE_RXTX GPIO_PIN_4
#define PORT_NRF24_IRQ GPIOD
#define PIN_NRF24_IRQ GPIO_PIN_3

/* STRUCTURE PROTOTYPES ------------------------------------------------------*/
struct buffer_struct {
	uint8_t *array;
	uint8_t size;
};

struct io_struct {
	GPIO_TypeDef *port;
	GPIO_Pin_TypeDef pin;
};

struct pos2d_struct {
	uint8_t x, y;
};

struct pos3d_struct {
	uint8_t x, y, z;
};

struct size2d_struct {
	uint8_t width, height;
};

struct size3d_struct {
	uint8_t width, length, height;
};


/* FUNCTION PROTOTYPES -------------------------------------------------------*/
uint32_t GetCurClockFreq(void);
void Sleep(uint32_t t);
void AssignIO(struct io_struct *cur_io, GPIO_TypeDef *cur_port, GPIO_Pin_TypeDef cur_pin, bool init_io, GPIO_Mode_TypeDef cur_mode);
void WriteIO(struct io_struct *cur_io, IO_MODE_TypeDef io_mode);
void ReadIO(struct io_struct *cur_io, bool *result);
void ModeIO(struct io_struct *cur_io, GPIO_Mode_TypeDef cur_mode);
uint8_t DecFromBCD(uint8_t bcd_val);
uint8_t DecToBCD(uint8_t dec_val);
void CRC16(uint16_t *crc, uint8_t data);
int8_t Round8S(float x);
uint8_t Round8U(float x);
void DataMemory_Write(uint8_t byte_no, uint8_t data);
uint8_t DataMemory_Read(uint8_t byte_no);
uint8_t ReverseBitOrder(uint8_t n);

#endif /* __UTILS_H */


//.map file contents
//> .text, .const and .vector are ROM
//> .bsct, .ubsct, .data and .bss are all RAM
//> .info. and .debug are symbol tables that do not use target resources/memory.
