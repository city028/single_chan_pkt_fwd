/*******************************************************************************
 * UDP Header file
 *******************************************************************************/

#ifndef _udp_hpp_
#define _udp_hpp_



int UDP_init( void );
int UDP_Engine( void );
int UDP_SendUDP( char *msg, int length );
void UDP_Receive( void );
int UDP_SendPullDataTMR( void );
void UDP_SendStat( void );
int UDP_SendGWStatusUpate( void );
int UDP_SendPullData( void );
char UDP_GetProtoVersion( void );



/// Define your server IP address below
//#define SERVER "54.72.145.119"       // The Things Network: croft.thethings.girovito.nl
//#define SERVER "75.119.128.125"      // My test VPS running TTS
#define SERVER "172.19.0.5"            // using the VPN to my VPS running TTS
//#define SERVER "192.168.1.10"        // local

#define PORT 1700                      // The port on which to send data

#define MAXLINE 1024


#define BUFLEN 2048  //Max length of buffer

#define PROTOCOL_VERSION  2      // Found a description of version 2 so that is what I will be using

#define PKT_PUSH_DATA    0
#define PKT_PUSH_ACK     1
#define PKT_PULL_DATA    2
#define PKT_PULL_RESP    3
#define PKT_PULL_ACK     4
#define PKT_TX_ACK       5

#define PULL_DATA_PKT_LEN 12

#define TX_BUFF_SIZE     2048
#define STATUS_SIZE      1024

#define TMR_TX_PULL       5     /// Send data pull request every 5 seconds
#define TMR_STAT_TX       30    /// Send gateway status every 30 seconds

#endif // _udp_hpp_
