#ifndef TYPEDEF_H
#define TYPEDEF_H

#define UDP_MAC_LEN     18
#define MAX_SN_LEN      32
#define IPADDR_BUF_LEN  20
#define MAX_VER_LEN     32
#define CID_LEN         11
#define COMMIT_LEN      41
#define KEY_LEN         16
#define DATA_LEN        128

#define	HEAD_SUCCESS_ACK	0x3100
#define	HEAD_FAIL_ACK		0x3101
#define	HEAD_CONTENT_ACK	0x3102
#define	MAX_CONTENT_LEN	256

typedef struct {
    short head;
    unsigned short len;
    short checksum;
    char content[MAX_CONTENT_LEN];
} Cmd_pkt_t;

typedef struct {
    char mac[UDP_MAC_LEN];
    char sn[MAX_SN_LEN];
    char ip[IPADDR_BUF_LEN];
    char ver[MAX_VER_LEN];
    char cid[CID_LEN];
    char commit[COMMIT_LEN];
    int type;
    int uid;
} udp_pkt_t;

typedef enum {
    SET_TUYA,
    GET_TUYA,
    SET_CMCC,
    GET_CMCC,
    SET_NETWORK,
    GET_NETWORK
} udp_cmd_e;

typedef struct {
    char pid[MAX_SN_LEN];
    char uuid[MAX_SN_LEN];
    char authkey[MAX_SN_LEN];
} tuya_config_t;

typedef struct {
    char sn[MAX_SN_LEN];
    char cmei[MAX_SN_LEN];
    char videokey[KEY_LEN];
    char loginkey[KEY_LEN];
} cmcc_config_t;

typedef struct {
    char target_ip[IPADDR_BUF_LEN];
    char cid[CID_LEN];
    char user[KEY_LEN];
    char passwd[KEY_LEN];
    char command;
    char hw_sn[MAX_SN_LEN];
    char hw_mac[UDP_MAC_LEN];
    char hw_lan[UDP_MAC_LEN];
    char data[DATA_LEN];
} udp_hw_config_t;

typedef struct {
    char command;
    char state;
    char cid[CID_LEN];
    char data[DATA_LEN];
} udp_ack_t;

typedef enum {
    IPMODE_STATIC = 0,
    IPMODE_DHCP
} ipmode_e;

typedef struct network_config_s {
    ipmode_e ipmode;
    char ip[IPADDR_BUF_LEN];
    char netmask[IPADDR_BUF_LEN];
    char gateway[IPADDR_BUF_LEN];
    char dns1[IPADDR_BUF_LEN];
    char dns2[IPADDR_BUF_LEN];
} __attribute__((aligned(4)))network_config_t;

typedef enum head_type {
    HEAD_CLEAR_CONFIG   = 0x270E,
    HEAD_TEST_MIC       = 0x280F,
    HEAD_PLAY_AUDIO		= 0x2816,
    HEAD_TEST_IPERF     = 0x2822,
    HEAD_IR_LED_OPEN,
    HEAD_IR_LED_CLOSE,
    HEAD_WHITE_LED_OPEN,
    HEAD_WHITE_LED_CLOSE,
    HEAD_ALARM_LED_OPEN,
    HEAD_ALARM_LED_COLSE,
    HEAD_ALARM_BLED_OPEN,
    HEAD_ALARM_BLED_COLSE,
    HEAD_IRCUT_DAY,
    HEAD_IRCUT_NIGHT,
    HEAD_WIFI_RSSI,
    HEAD_PTZCTRL_UP,
    HEAD_PTZCTRL_DOWN,
    HEAD_PTZCTRL_LEFT,
    HEAD_PTZCTRL_RIGHT,
    HEAD_PTZCTRL_RESET,
    HEAD_FORMAT_SD,
    HEAD_LOGIN,
    HEAD_SPEAKER_START,
    HEAD_SPEAKER_STOP,
    HEAD_REBOOT
} head_type_e;

#endif // TYPEDEF_H
