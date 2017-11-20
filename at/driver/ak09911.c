#include "osapi.h"
#include "at_custom.h"
#include "user_interface.h"
#include "driver/i2c_master.h"
#include "driver/ak09911.h"

/*
function: read an internal regester
 addr: resgiter address
 pdata: recive data
 len: data len
*/

bool ICACHE_FLASH_ATTR
ak_readRegister(uint8 addr, uint8 *pData, uint16 len)
{
	uint8 ack;
	uint16 i;
/*
1.	  Master����I2C addr��7bit����w����0��1bit�����ȴ�ACK
2.	  Slave����ACK
3.	  Master����reg addr��8bit�����ȴ�ACK
4.	  Slave����ACK
5.	  Master����START
6.	  Master����I2C addr��7bit����r����1��1bit�����ȴ�ACK
7.	  Slave����ACK
8.	  Slave����data��8bit�������Ĵ������ֵ
9.	  Master����ACK
*/
	i2c_master_start();
	i2c_master_writeByte(AK_SLAVE_ADDR);
	ack = i2c_master_getAck();

	if (ack) {
		at_port_print("AK9911 slave addr not ack when tx write cmd\n");
		i2c_master_stop();
		return false;
	}
	i2c_master_writeByte(addr);
	ack = i2c_master_getAck();

	if (ack) {
		at_port_print("regist addr not ack when tx write cmd\n");
		i2c_master_stop();
		return false;
	}
	i2c_master_start();
	i2c_master_writeByte(AK_SLAVE_ADDR+1);
	ack = i2c_master_getAck();

	if (ack) {
		at_port_print("AK9911 read addr not ack when tx write cmd\n");
		i2c_master_stop();
		return false;
	}		

	for (i = 0; i < len; i++) {
		pData[i] = i2c_master_readByte();

		i2c_master_setAck((i == (len - 1)) ? 1 : 0);
	}

	i2c_master_stop();

	return true;
}

/*
function: write an internal regester
 addr: resgiter address
 pdata: write data
 len: data len
*/

bool ICACHE_FLASH_ATTR
ak_writeRegister(uint8 addr, uint8 *pData, uint16 len)
{
	uint8 ack;
	uint16 i;
/*
1.    Master����START
2.    Master����I2C addr��7bit����w����0��1bit�����ȴ�ACK
3.    Slave����ACK
4.    Master����reg addr��8bit�����ȴ�ACK
5.    Slave����ACK
6.    Master����data��8bit������Ҫд��Ĵ����е����ݣ��ȴ�ACK
7.    Slave����ACK
8.    ��6���͵�7�������ظ���Σ���˳��д����Ĵ���
9.    Master����STOP

*/
	i2c_master_start();
	i2c_master_writeByte(AK_SLAVE_ADDR);
	ack = i2c_master_getAck();

	if (ack) {
		at_port_print("AK9911 slave addr not ack when tx write cmd\n");
		i2c_master_stop();
		return false;
	}
	i2c_master_writeByte(addr);
	ack = i2c_master_getAck();

	if (ack) {
		at_port_print("regist addr not ack when tx write cmd\n");
		i2c_master_stop();
		return false;
	}
	
	for (i = 0; i < len; i++) {
		i2c_master_writeByte(pData[i]);
//		pData[i] = i2c_master_readByte();
		ack = i2c_master_getAck();

		if (ack) {
			at_port_print("witre data  ack  timeout\n");
			i2c_master_stop();
			return false;
		}		
	}

	i2c_master_stop();

	return true;
}


bool ICACHE_FLASH_ATTR
ak_readSingleMagenticData(uint8 *pData)
{

/*
1.	 ���ÿ���ģʽΪSINGLE����������
2.	��DRDY �Ĵ���λ�����Ϊ1��������READY
3.	��ȡ����
4.	  ��ȡ��������
5.	  ��ȡST2�Ĵ����������ǲ����б仯������Ĵ���

*/

	uint8 mode = AK_SINGLEREAD_MODE;
	uint8 data_state = 0;
	uint8 data_ready = 0,count =0;

	ak_writeRegister(AK_READMODE_ADDR,&mode,1);
	i2c_master_wait(5);	

	while(data_ready == 0){
		ak_readRegister(AK_ST1_ADDR,&data_state,1);
		data_ready = data_state&AK_DRDY_MASK;
		count++;
		if(count >10){
			os_printf("read data not ready\n");
			return false;			
		}
	}
	
	//6bit x_l,x_h,y_l,y_h,z_l,z_h 
	ak_readRegister(AK_MEASURE_DATA_BEGIN_ADDR,pData,6);

	data_state = 0;
	ak_readRegister(AK_ST2_ADDR,&data_state,1);	
	if(data_state & AK_DOFL_MASK != 0 ){
			os_printf("read data overflow\n");
			return false;				
	}
	return true;
}



