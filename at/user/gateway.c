#include "osapi.h"
#include "at_custom.h"

#include "user_interface.h"
#include "mqtt.h"
#include "config.h"
#include "mem.h"

#include "driver/i2c_master.h"
#include "driver/ak09911.h"


//#include "json.h"
#include "user_json.h"

#include "gateway.h"

versionInfo g_versionInfo;
uint8 g_version = 218;


LOCAL struct jsontree_callback jsonUpdateVersionCallback =  
    JSONTREE_CALLBACK(NULL, gw_updateVersion_parse);  //ONLY SET PRASE DATA  


  
JSONTREE_OBJECT(jsonUpdateVersionTrees,  
                JSONTREE_PAIR("esn", &jsonUpdateVersionCallback),  
                JSONTREE_PAIR("version", &jsonUpdateVersionCallback),  
                JSONTREE_PAIR("path", &jsonUpdateVersionCallback));  
JSONTREE_OBJECT(jsonUpdateVersionTree,  
                JSONTREE_PAIR("updateVersion", &jsonUpdateVersionTrees)); 

int ICACHE_FLASH_ATTR  
gw_updateVersion_parse(struct jsontree_context *js_ctx, struct jsonparse_state *parser)  
{  
    int type;  
	versionInfo *pInfo = &g_versionInfo;

	os_bzero(pInfo,sizeof(versionInfo));
  
    while ((type = jsonparse_next(parser)) != 0) {  
        //如果是KEY类型  
        if (type == JSON_TYPE_PAIR_NAME) {  
            char buffer[64];  
            os_bzero(buffer, 64);  
  
            if (jsonparse_strcmp_value(parser, "esn") == 0) {  
                jsonparse_next(parser); //返回的是冒号字符  
                type = jsonparse_next(parser);  //返回的是双引号字符  
  
                //如果Value是字符串类型，则读取数据到buffer  
                if (JSON_TYPE_STRING == type){  //#define JSON_TYPE_STRING '"'  
                    jsonparse_copy_value(parser, pInfo->esn, sizeof( pInfo->esn));  
                    os_printf("esn = %s\n", pInfo->esn);  
                }  
            }
            if (jsonparse_strcmp_value(parser, "version") == 0) {  
                jsonparse_next(parser); //返回的是冒号字符  
                type = jsonparse_next(parser);  //返回的是双引号字符  
  
                //如果Value是字符串类型，则读取数据到buffer  
                if (JSON_TYPE_NUMBER  == type){  //#define JSON_TYPE_STRING '"'  
                    pInfo->version = jsonparse_get_value_as_int(parser);  
                    os_printf("version = %d\n", pInfo->version);  
                }  
            }
            if (jsonparse_strcmp_value(parser, "path") == 0) {  
                jsonparse_next(parser); //返回的是冒号字符  
                type = jsonparse_next(parser);  //返回的是双引号字符  
  
                //如果Value是字符串类型，则读取数据到buffer  
                if (JSON_TYPE_STRING == type){  //#define JSON_TYPE_STRING '"'  
                    jsonparse_copy_value(parser, pInfo->path, sizeof(pInfo->path));  
                    os_printf("path = %s\n", pInfo->path);  
                }  
            }			
        }
    }
	return true;
}


void ICACHE_FLASH_ATTR  
gw_Json_parse(char *json)  
{  
    struct jsontree_context js;  
  
    jsontree_setup(&js, (struct jsontree_value *)&jsonUpdateVersionTree, json_putchar);  
    json_parse(&js, json);  
}  



void gw_mqtt_reportCb(MQTT_Client* client)
{
	uint8 source[6];
	uint8 length = 0;
	char buf[64] = {0};
	uint8* pAddr;
	struct ip_info ipConfig;
	wifi_get_ip_info(STATION_IF, &ipConfig);


	
	wifi_get_macaddr(STATION_IF, source);
	wifi_get_ip_info(STATION_IF, &ipConfig);
	pAddr =(uint8*) &(ipConfig.ip.addr);

	os_sprintf(buf+length,"{esn:'ESP%08X',",system_get_chip_id());
	length = os_strlen(buf);
	
	os_sprintf(buf+length,"mac:'"MACSTR"',",MAC2STR(source));
	length = os_strlen(buf);

	os_sprintf(buf+length,"ip:'"IPSTR"',",pAddr[0],pAddr[1],pAddr[2],pAddr[3]);
	length = os_strlen(buf);

	os_sprintf(buf+length,"version:%d}",g_version);
	length = os_strlen(buf);	

	if(g_mqttConnected == 0)
	{
		MQTT_Connect(&mqttClient);
		return;
		
	}

	if(client->connState == MQTT_DATA)
	{
		MQTT_Publish(client, "gateway_report", buf,length, 0, 0);
		INFO("MQTT: report published\r\n");
	}
	else
	{
		
		MQTT_Disconnect(&mqttClient);
		INFO("MQTT: restart\r\n");
		g_mqttConnected = 0;
		
	}


	
//	MQTT_Publish(client, "/mqtt/topic/1", "hello1", 6, 1, 0);
//	MQTT_Publish(client, "/mqtt/topic/2", "hello2", 6, 2, 0);

}



void gw_mqtt_sendHallData(MQTT_Client* client)
{
	uint8 source[6];
	uint8 length = 0;
	char buf[64] = {0};
	uint8 data[16] =  {0};
	sint16 x=0,y=0,z=0;
	int result = false;
	
	result = ak_readSingleMagenticData(data);

	if(result == true){
		x=data[1]*256+data[0];
		y=data[3]*256+data[2];
		z=data[5]*256+data[4];
	}
	else
	{
		os_printf("read Magentic data error!");
		return;
	}
		
	wifi_get_macaddr(STATION_IF, source);

	os_sprintf(buf+length,"{mac:'"MACSTR"',",MAC2STR(source));
	length = os_strlen(buf);



	os_sprintf(buf+length,"hall_X:%d,hall_Y:%d,hall_Z:%d}",x,y,z);
	length = os_strlen(buf);

	if(g_mqttConnected == 0)
	{
		MQTT_Connect(&mqttClient);
		return;
		
	}

	if(client->connState == MQTT_DATA)
	{
		MQTT_Publish(client, "sensorData", buf,length, 0, 0);
		INFO("MQTT:  sensorData published\r\n");
	}
	else
	{
		
		MQTT_Disconnect(&mqttClient);
		INFO("MQTT: restart\r\n");
		g_mqttConnected = 0;
		
	}

}





void ICACHE_FLASH_ATTR 
gw_report_status()
{
	os_printf("Start report....\r\n");


	//if(mqttClient.connState )
	if(g_mqttConnected ==1)
	{
		gw_mqtt_reportCb(&mqttClient);
		gw_mqtt_sendHallData(&mqttClient);
	}
}

void gw_recieveMqttData(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);
//	char ip[4]={0xc0,0xa8,0x0,0x6c};
	char ip[4]={139,224,232,63};

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	//这儿没有用topic，后续根据topic 进行解析
	gw_Json_parse(dataBuf);
	ota_start_Upgrade(g_versionInfo.version,ip,8080,g_versionInfo.path);
	
	os_free(topicBuf);
	os_free(dataBuf);
}


