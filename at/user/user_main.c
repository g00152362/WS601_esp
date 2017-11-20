/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "osapi.h"
#include "at_custom.h"
#include "user_interface.h"
#include "driver/i2c_master.h"
#include "driver/ak09911.h"

#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "mem.h"
#include "gateway.h"


#define GLOBAL_DEBUG_ON 1





MQTT_Client mqttClient;
os_timer_t g_report_timer;
uint8 g_mqttConnected;





// test :AT+SETIO=1,1 (GPIO , value)

void ICACHE_FLASH_ATTR
at_setupCmdTest(uint8_t id, char *pPara)
{
    int gpio = 0, err = 0, flag = 0,value = 0;
    uint8 buffer[32] = {0};
    pPara++; // skip '='


    //get the first parameter
    // digit
    flag = at_get_next_int_dec(&pPara, &gpio, &err);

    // flag must be ture because there are more parameter
    if (flag == FALSE) {
        at_response_error();
        return;
    }

	os_sprintf(buffer, "the gpio:%d\r\n", gpio);
    at_port_print(buffer);

    if (*pPara == ',') {
        pPara++; // skip ','
        value = 0;
        //there is the second parameter
        // digit
        flag = at_get_next_int_dec(&pPara, &value, &err);
    }
	os_sprintf(buffer, "the value parameter:%d\r\n", value);
    at_port_print(buffer);

	

    if (*pPara != '\r') {
        at_response_error();
        return;
    }
    if(gpio >31 ){
        at_response_error();
        return;		
    }

	switch(gpio){
		case 2: 
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U , FUNC_GPIO2);
			break;

		case 12: 
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
			break;	

		case 14: 
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
			break;	
		default:
			at_port_print("port is not defined\r\n");
			 at_response_error();
			 return;
		};

	GPIO_OUTPUT_SET(gpio, value); 

    at_response_ok();
}



void ICACHE_FLASH_ATTR
at_testCmdTest(uint8_t id)
{
    uint8 buffer[32] = {0};

    os_sprintf(buffer, "%s\r\n", "at_testCmdTest");
    at_port_print(buffer);
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_queryCmdTest(uint8_t id)
{    
	uint8 buffer[32] = {0};    
	os_sprintf(buffer, "%s\r\n", "at_queryCmdTest");    
	at_port_print(buffer);    
	at_response_ok();
}

// test :AT+QUERYIO=1 (GPIO )

void ICACHE_FLASH_ATTR
at_queryGPIOCmdTest(uint8_t id, char *pPara)
{
    uint8 buffer[32] = {0};
    int gpio = 0, err = 0,value = 0;
	uint8_t state = 0;

	pPara++; // skip '=
	at_get_next_int_dec(&pPara, &gpio, &err);
	
	state = 0x1 & GPIO_INPUT_GET(gpio);
	if(state) {
		at_response("ON");
	} else {
		at_response("OFF");
	}
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_exeCmdTest(uint8_t id)
{
    uint8 buffer[32] = {0};

    os_sprintf(buffer, "%s\r\n", "at_exeCmdTest");
    at_port_print(buffer);
    at_response_ok();
}


void ICACHE_FLASH_ATTR
at_readICCmdTest(uint8_t id, char *pPara)
{
    uint8 buffer[64] = {0};
	uint8 data[16] =  {0};
    int ic_address = 0, err = 0,length = 0,i=0,count=0;
	uint8_t state = 0;

	pPara++; // skip '=
	at_get_next_int_dec(&pPara, &ic_address, &err);
	pPara++; // skip ',
	at_get_next_int_dec(&pPara, &count, &err);
	pPara++; // skip ',
	at_get_next_int_dec(&pPara, &length, &err);

	for(i=0; i<count; i++)
	{
		ak_readRegister((uint8)ic_address,data,length);
	}
	at_port_print("\r\ndata: ");
	for(i=0;i<length;i++)
	{
		os_sprintf(buffer, "%2X ", data[i]);
		at_port_print(buffer);
	}
	at_port_print("\r\n");
	


    at_response_ok();
}

//AT+WRITEIC=1,2(register address,data(1 BYTE))
void ICACHE_FLASH_ATTR
at_writeICCmdTest(uint8_t id, char *pPara)
{

    int err = 0,ic_address = 0,data = 0;

	uint8 d=0 ;

	pPara++; // skip '=
	at_get_next_int_dec(&pPara, &ic_address, &err);
	pPara++; // skip ',
	at_get_next_int_dec(&pPara, &data, &err);

	d=(uint8)data;
	ak_writeRegister((uint8)ic_address,&d,1);

    at_response_ok();
}

//AT+READHALL=0(0: once,1 multi times)
void ICACHE_FLASH_ATTR
at_readHallCmdTest(uint8_t id, char *pPara)
{
    uint8 buffer[64] = {0};
	uint8 data[16] =  {0};
    int mode = 0, result = true,length = 0,i=0,count=0;
	sint16 x=0,y=0,z=0;


	pPara++; // skip '=
	at_get_next_int_dec(&pPara, &mode, &result);

	if(mode == 1){
		result = ak_readSingleMagenticData(data);
	}

	if(result == true){
		x=data[1]*256+data[0];
		y=data[3]*256+data[2];
		z=data[5]*256+data[4];
	}
	
	os_sprintf(buffer, "\r\nMagentic: x:%d y:%d z:%d\r\n", x,y,z);
	at_port_print(buffer);
    at_response_ok();
}



// test :AT+TESTIC=1,0 SDA,SCL )
/*
void ICACHE_FLASH_ATTR
at_testICCmdTest(uint8_t id, char *pPara)
{
    uint8 buffer[64] = {0};
	uint8 data[16] =  {0};
    int sda = 0, err = 0,scl = 0,i=0;
	uint8_t state = 0;

	pPara++; // skip '=
	at_get_next_int_dec(&pPara, &sda, &err);
	pPara++; // skip ',
	at_get_next_int_dec(&pPara, &scl, &err);
	
    i2c_master_setDC(sda, scl);
    i2c_master_wait(5);

	

    at_response_ok();

}
*/

void ICACHE_FLASH_ATTR
at_scanCmdTest(uint8_t id, char *pPara)
{
    uint8 buffer[64] = {0};
    int sda = 0, err = 0,scl = 0,i=0;
	uint8_t state = 0;

	pPara++; // skip '=
	at_get_next_int_dec(&pPara, &sda, &err);

	sniffer_init();
	
   	at_response_ok();

}



extern void at_exeCmdCiupdate(uint8_t id);
at_funcationType at_custom_cmd[] = {
    {"+SETIO", 6, at_testCmdTest, at_queryCmdTest, at_setupCmdTest, at_exeCmdTest},
	{"+QUERYIO", 8, NULL,  NULL,at_queryGPIOCmdTest, NULL},
	{"+READIC", 7, NULL,  NULL,at_readICCmdTest, NULL},
	{"+WRITEIC", 8, NULL,  NULL,at_writeICCmdTest, NULL},
	{"+READHALL", 9, NULL,  NULL,at_readHallCmdTest, NULL},	
	{"+SCAN", 5, NULL,  NULL,at_scanCmdTest, NULL},	
//	{"+TESTIC", 7, NULL,  NULL,at_testICCmdTest, NULL},		
#ifdef AT_UPGRADE_SUPPORT
    {"+CIUPDATE", 9,       NULL,            NULL,            NULL, at_exeCmdCiupdate}
#endif
};

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABBBCDDD
 *                A : rf cal
 *                B : at parameters
 *                C : rf init data
 *                D : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
    system_phy_freq_trace_enable(at_get_rf_auto_trace_from_flash());
}

void to_scan(void) {
	sniffer_init();}



void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	MQTT_Subscribe(&mqttClient,"versionUpdate",1);
	g_mqttConnected = 1;
	INFO("MQTT: Connected\r\n");
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	g_mqttConnected = 0;
	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}


void mqtt_wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
		
		//every one minits to report the state
		gw_report_status();
		os_timer_setfn(&g_report_timer, (os_timer_func_t *)gw_report_status, 0);
		os_timer_arm(&g_report_timer,REPORT_TIME, 1);	
	} else {
		MQTT_Disconnect(&mqttClient);
		g_mqttConnected =0;
	}
}


void 
mqtt_init()
{
	os_delay_us(60000);

	CFG_Load();
	os_printf("ssid:%s, pw:%s\r\n",sysCfg.sta_ssid,sysCfg.sta_pwd);

	wifi_set_opmode(1);
	// because the wifi send may send the duplication packet to casue tcp retransimit, set the max retran to re-establish TCP
	espconn_tcp_set_max_retran(3);
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, mqtt_wifiConnectCb);

	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnData(&mqttClient, gw_recieveMqttData);
	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);

}



void ICACHE_FLASH_ATTR
user_init(void)
{
    char buf[64] = {0};
    at_customLinkMax = 6;
	os_printf("new version 218@!@");
//    at_init();
    os_printf("compile time:%s %s",__DATE__,__TIME__);
 //   at_set_custom_info(buf);
//   at_port_print("\r\nready\r\n");
	// initial command
 //   at_cmd_array_regist(&at_custom_cmd[0], sizeof(at_custom_cmd)/sizeof(at_custom_cmd[0]));
	// initial GPIO
	i2c_master_gpio_init();
	//initial WIFI probe,set the station mode first,must be call probe init after system init 
	//system_init_done_cb(to_scan);

	//mqtt init
	mqtt_init();
	
}






