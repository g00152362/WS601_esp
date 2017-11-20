#ifndef __CARPORT_H__
#define __CARPORT_H__
#include "user_json.h"

#define REPORT_TIME 60*1000*10 //report time , one minits default
typedef struct{
	char esn[32];
	
	char path[64];
	uint8 version;
	//char filename[64];
} versionInfo;


extern MQTT_Client mqttClient;
extern SYSCFG sysCfg;
extern uint8 g_mqttConnected;

void gw_report_status();
int gw_updateVersion_parse(struct jsontree_context *js_ctx, struct jsonparse_state *parser) ;
void gw_recieveMqttData(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len);









#endif

