#ifndef _SNIFFER_H
#define _SNIFFER_H
#define MS 1000
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

typedef enum _encrytion_mode {
    ENCRY_NONE           = 1,
    ENCRY_WEP,
    ENCRY_TKIP,
    ENCRY_CCMP
} ENCYTPTION_MODE;

/**
@breif Struct that holds AP information in Active Scan
**/

struct router_info {
    SLIST_ENTRY(router_info)     next;
    uint8  bssid[6];
    uint8  channel;
    uint8  authmode;
    uint16 rx_seq;
    uint8  encrytion_mode;
    uint8  iv[8];
    uint8  iv_check;
};

/**
@breif Struct that holds the Rx parameters and calculated parameters like signal strength, etc.
**/
struct RxControl {
    signed rssi:8;
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;
    unsigned legacy_length:12;
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;
    unsigned CWB:1;
    unsigned HT_length:16;
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1;
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4;
    unsigned:12;
};


struct Ampdu_Info
{
  uint16 length;
  uint16 seq;
  uint8  address3[6];
};

struct sniffer_buf {
    struct RxControl rx_ctrl;
    uint8  buf[36];
    uint16 cnt;
    struct Ampdu_Info ampdu_info[1];
};

/**
@breif Struct that holds first 24 bytes of Beacon
**/
struct Beacon_Info
{
	unsigned version:2;
	unsigned type:2;		//00 is Management type for beacon
	unsigned sub_type:4;	//1000 is a beacon
	uint8 frame_ctrl_flg;
	uint8 duration[2];		//duration in microsecs
    uint8 destination[6];	//Destination Address
	uint8 source[6];		//Source Address
	uint8 bssid[6];			//MAC address of AP
	unsigned frag_num: 4;
	unsigned seq_num: 12;
};

/**
@breif Struct that holds first 24 bytes of Beacon
**/
struct beacon_buf {
    struct RxControl rx_ctrl;
	struct Beacon_Info beacon_info;
	uint8 timestamp[8];			//timestamp in microseconds
	uint8 beacon_interval[2];	//100millisec
	uint8 capability[2];
	uint8 ssid_ele_id;			//Element id is 0 means ssid
	uint8 ssid_len;				//ssid length
	uint8 buf[74];				//buf[0]--buf[ssid_len-1] == ssid
    uint16 cnt;
    uint16 len; 				//length of packet
};

#endif
