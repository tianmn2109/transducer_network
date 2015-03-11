																			   																												 #ifndef __I2C_FRAM_H
#define __I2C_FRAM_H

//#include "stm32f10x_lib.h"
#include "ssnp/opt.h"
#include "stm32f10x.h"

u8t I2C_FRAM_BufferWrite(u8t* pBuffer, u16t WriteAddr, u16t NumByteToWrite);
u8t I2C_FRAM_BufferRead(u8t* pBuffer, u16t ReadAddr, u16t NumByteToRead);
void I2C_BufferWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint16_t NumByteToWrite);

#endif /* __I2C_FRAM_H */
