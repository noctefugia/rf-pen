#ifndef __SPI_SOFT_H
#define __SPI_SOFT_H

#include "utils.h"

#define SPIS_DEVICE_COUNT 1
#define SPIS_R_REGISTER    0x00 /* last 4 bits will indicate reg. address */
#define SPIS_W_REGISTER    0x20 /* last 4 bits will indicate reg. address */
#define SPIS_REGISTER_MASK 0x1F
#define IS_SPIS_DEVICE_ID_OK(id) \
	((id) < (SPIS_DEVICE_COUNT)) 

struct spiss_struct {
	struct io_struct io_miso, io_mosi, io_sck, io_cs;
};

void SPIS_Init(void);
uint8_t SPIS_Transfer(uint8_t tx);
void SPIS_TransferSync(uint8_t* dataout, uint8_t* datain,uint8_t len);
void SPIS_TransmitSync(uint8_t* dataout, uint8_t len);
void SPIS_ConfigRegister(uint8_t reg, uint8_t value);
void SPIS_ReadRegister(uint8_t reg, uint8_t* value, uint8_t len);
void SPIS_WriteRegister(uint8_t reg, uint8_t* value, uint8_t len);
void SPIS_HighCS(void);
void SPIS_LowCS(void);

#endif /* __SPI_SOFT_H */