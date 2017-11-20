//#include "esp_common.h"
#include "osapi.h"

#include "c_types.h"
#include "Os_type.h"
#include "Queue.h"
#include "Mem.h"

#include "user_interface.h"



#include "sniffer.h"

os_timer_t channel_timer;
uint8 current_channel = 0;

/*In Sniffer mode ESP8266 cannot connect to the AP. You need to disable the Sniffer mode and then connect to AP*/
#define ONLY_PASSIVE_SCAN 0	//Make this 1 to activate only passive scan

SLIST_HEAD(router_info_head, router_info) router_list;
/*
void ICACHE_FLASH_ATTR sniffer_wifi_promiscuous_rx(uint8 *buf, uint16 buf_len)
{	
	uint8 i;
	//Check beacon_buf struct in snifer.h
	struct beacon_buf * beacon = (struct beacon_buf *)buf;	//Store the recieved buffer in beacon_bbuf Struct
	//If first byte of bssid is 0xFF, neglect it
	if(beacon ->beacon_info.bssid[0]!= 0xFF) {
		//if buf_len is 128bytes, the packet received is a AP Beacon
		if (buf_len == sizeof(struct beacon_buf)) {
			//Display MAC adddress, channel, signal strength and ssid
			os_printf("mac addr "MACSTR"  ",MAC2STR(beacon ->beacon_info.bssid));
			os_printf("channel %d,  rssi %d,  ssid ", beacon->rx_ctrl.channel, beacon->rx_ctrl.rssi);
			for(i = 0; i < (beacon ->ssid_len); i++) {
				os_printf("%c", (char)beacon->buf[i]);
			}
			os_printf("\n");
			os_delay_us(33*MS);	//display every AP detail every 33millisec
		}
	}
}*/


void ICACHE_FLASH_ATTR sniffer_wifi_promiscuous_rx(uint8 *buf, uint16 buf_len)
{	
	uint8 i;
	//Check beacon_buf struct in snifer.h
	struct beacon_buf * beacon = (struct beacon_buf *)buf;	//Store the recieved buffer in beacon_bbuf Struct
	//If first byte of bssid is 0xFF, is a probe
	if(beacon ->beacon_info.bssid[0]== 0xFF) {
		//if buf_len is 128bytes, the packet received is a AP Beacon
		if (buf_len == sizeof(struct beacon_buf)) {
			//Display MAC adddress, channel, signal strength and ssid
			os_printf("mac addr "MACSTR"  ",MAC2STR(beacon ->beacon_info.source));
			os_printf("channel %d,  rssi %d ", beacon->rx_ctrl.channel, beacon->rx_ctrl.rssi);
			os_printf("\n");
			os_delay_us(33*MS);	//display every AP detail every 33millisec
		}
	}
}


/******************************************************************************
 * FunctionName : sniffer_channel_timer_cb
 * Description  : Change channel every 3.333sec
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR sniffer_channel_timer_cb(void *arg)
{
	current_channel++;
	if(current_channel < 14) {
		
		wifi_promiscuous_enable(0); //Disable promiscuous mode
		os_delay_us(5*MS);
		wifi_set_channel((current_channel + 1));	//set to different channel
		wifi_promiscuous_enable(1);  //Disable promiscuous mode
		os_printf("current channel %d\n", (current_channel + 1));
		os_timer_arm(&channel_timer, 3333, 0);
	} else {
		current_channel = 1;
	}
}

/******************************************************************************
 * FunctionName : sniffer_wifi_scan_done
 * Description  : Function to display AP from active scan and then activate passive scan.
 				  This function is disabled in ONLY_PASIVE_SCAN mode
 * Parameters   : status-- OK is active scan is successful
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR sniffer_wifi_scan_done(void *arg, STATUS status)
{
	uint8 ssid[33];
	struct router_info *info = NULL;

	while((info = SLIST_FIRST(&router_list)) != NULL){
		SLIST_REMOVE_HEAD(&router_list, next);
		os_free(info);
	}
	if (status == OK) {
		uint8 i;
		struct bss_info *bss = (struct bss_info *)arg;
		while (bss != NULL) {
			if (bss->channel != 0) {
				struct router_info *info = NULL;
				os_printf("ssid %s, channel %d, authmode %d, rssi %d\n",
				bss->ssid, bss->channel, bss->authmode, bss->rssi);
				info = (struct router_info *)os_malloc(sizeof(struct router_info));
				info->authmode = bss->authmode;
				info->channel = bss->channel;
				os_memcpy(info->bssid, bss->bssid, 6);
				SLIST_INSERT_HEAD(&router_list, info, next);
			}
			bss = STAILQ_NEXT(bss, next);
		}
		wifi_set_channel(1);
		wifi_promiscuous_enable(0);	//Disable promiscuous mode
		/*each time packet is received 'sniffer_wifi_promiscuous_rx' function is called*/
		wifi_set_promiscuous_rx_cb(sniffer_wifi_promiscuous_rx);
		wifi_promiscuous_enable(1);	//Enable promiscuous mode
		/*Set timer to 3.333 sec to trigger timer callback function*/
		os_timer_disarm(&channel_timer);
		os_timer_setfn(&channel_timer, sniffer_channel_timer_cb, NULL);
		os_timer_arm(&channel_timer, 3333, 0);
	}else {
		os_printf("err, scan status %d\n", status);
	}
}

void ICACHE_FLASH_ATTR sniffer_system_init(void)
{
/*	#if ONLY_PASSIVE_SCAN
	os_printf("Only Passive Scan activated....\n");
	wifi_set_channel(1); 		//Set wifi channel
	wifi_promiscuous_enable(0); //Disable promiscuous mode 
	//each time packet is received 'sniffer_wifi_promiscuous_rx' function is called
	wifi_set_promiscuous_rx_cb(sniffer_wifi_promiscuous_rx);  //set promiscuous mode call back function.
	wifi_promiscuous_enable(1);  //Enable promiscuous mode
	os_timer_disarm(&channel_timer);
	os_timer_setfn(&channel_timer, sniffer_channel_timer_cb, NULL);
	os_timer_arm(&channel_timer, 3333, 0);
	#else
	*/
	os_printf("Active and Passive Scan activated....\n");
	SLIST_INIT(&router_list);

	if(wifi_station_scan(NULL,sniffer_wifi_scan_done)){
		os_printf("wifi_station_scan ok\n");   
	}
//	#endif
}

/******************************************************************************
 * FunctionName : sniffer_init
 * Description  : We use this function to initialize sniffer mode in ESP8266
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void sniffer_init(void)
{
	os_printf("Testing Sniffer Mode ....: %s\n", system_get_sdk_version());   
	os_delay_us(10*MS);
	wifi_set_opmode(STATION_MODE);	//set ESP as station, sniffer mode can be used only in ESP8266 in station mode
	sniffer_system_init();	//Initialize sniffr mode
	os_delay_us(10*MS);
	//vTaskDelete(NULL);
}
