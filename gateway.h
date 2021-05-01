/*******************************************************************************
 * Gateway Header file
 *******************************************************************************/

#ifndef _gateway_h_
#define _gateway_h_


int GW_Init(void);
int GW_Engine(void);
int GW_SendGWStatusUpate(void);
int GW_SendPullDataTMR(void);
void GW_SendStat(void);
int GW_SendPullData(void);
void GW_ProcessRX_UDP(void);
int GW_ProcessRX_Lora(void);



// Join procedure constists of 2 frames the request and the accept frame
enum {
    // Join Request frame format --> Updated to V1.1
    OFF_JR_HDR      = 0,
    OFF_JR_JOINEUI  = 1,    // 8 Octets
    OFF_JR_DEVEUI   = 9,    // 8 Octets
    OFF_JR_DEVNONCE = 17,   // 2 Octets
    OFF_JR_MIC      = 19,
    LEN_JR          = 23
};

enum {
    // Join Accept frame format
    OFF_JA_HDR      = 0,
    OFF_JA_ARTNONCE = 1,
    OFF_JA_NETID    = 4,
    OFF_JA_DEVADDR  = 7,
    OFF_JA_RFU      = 11,
    OFF_JA_DLSET    = 11,
    OFF_JA_RXDLY    = 12,
    OFF_CFLIST      = 13,
    LEN_JA          = 17,
    LEN_JAEXT       = 17+16
};




#define TMR_TX_PULL       5     /// Send data pull request every 5 seconds
#define TMR_STAT_TX       30    /// Send gateway status every 30 seconds

#define PROTOCOL_VERSION  2      // Found a description of version 2 so that is what I will be using

#define PKT_PUSH_DATA    0
#define PKT_PUSH_ACK     1
#define PKT_PULL_DATA    2
#define PKT_PULL_RESP    3
#define PKT_PULL_ACK     4
#define PKT_TX_ACK       5

#define STATUS_SIZE      1024

#define TX_BUFF_SIZE                2048          /// Double check this

#define PULL_DATA_PKT_LEN   12

#endif // _gateway_h_
