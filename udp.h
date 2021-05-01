/*******************************************************************************
 * UDP Header file
 *******************************************************************************/

#ifndef _udp_hpp_
#define _udp_hpp_

// Functions which can be called external from the UDP layer
int UDP_Init( void );                           // To be called in the init phase
int UDP_Engine( void );                         // To be called in the main programme loop
int UDP_ReceiveUDP( char *RxBuffer );           // To be called by the Application to receive UDP
int UDP_SendUDP(char *TxFrame, int FrameSize);  // To be called ny the application to send UDP

// Supporting Functions
int UDP_GetEth0Mac( struct ifreq *eth0_ifr);    // Get the MAC address of ETH0

// Functions Internal to the UDP Layer
static void UDP_TX_FIFO_Update( void );
static void UDP_RX_FIFO_Update( void );
static int UDP_CheckTX( void );
static int UDP_CheckRX( void );


/// Define your server IP address below
//#define SERVER "54.72.145.119"       // The Things Network: croft.thethings.girovito.nl
//#define SERVER "75.119.128.125"      // My test VPS running TTS
#define SERVER "172.19.0.5"            // using the VPN to my VPS running TTS
//#define SERVER "192.168.1.10"        // local

/// Default port for The Things Stack = 1700
#define PORT 1700                      // The port on which to send data, this is the TTS standard port

#define MAXLINE 1024                   // Need to be double checked
#define BUFLEN 2048                    // Max length of buffer

#define UDP_TX_MX_FRAME_SIZE    1024   // Maximum TX frame length = 1024 --> Double check this!!!!
#define UDP_TX_FIFO_DEPTH         10   // Max 10 frames in UDP TX buffer

#define UDP_RX_MX_FRAME_SIZE    1024   // Maximum TX frame length = 1024 --> Double check this!!!!
#define UDP_RX_FIFO_DEPTH         10   // Max 10 frames in UDP TX buffer


#endif // _udp_hpp_
