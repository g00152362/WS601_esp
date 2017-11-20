#ifndef __AK09911_H__
#define __AK09911_H__

#define AK_SLAVE_ADDR 0x18
#define AK_CNTL2_ADDR 0x31
#define AK_ST1_ADDR 0x10
#define AK_ST2_ADDR 0x18

#define AK_DRDY_MASK 0x01
#define AK_DOFL_MASK 0x08

#define AK_MEASURE_DATA_BEGIN_ADDR 0x11


#define AK_READMODE_ADDR AK_CNTL2_ADDR


#define AK_SINGLEREAD_MODE 0x01


bool ak_readRegister(uint8 addr, uint8 *pData, uint16 len);
bool ak_writeRegister(uint8 addr, uint8 *pData, uint16 len);
bool ak_readSingleMagenticData(uint8 *pData);




#endif

