/*


my Lorawan header file



*/


//! \file
#ifndef _lorawan_h_
#define _lorawan_h_


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









#endif // _lorawan_h_
