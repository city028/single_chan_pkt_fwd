/*******************************************************************************
 *
 * Making some changes to the below code, orignial credits, see below!!
 *
 *******************************************************************************/
/*******************************************************************************
 *
 * Copyright (c) 2015 Thomas Telkamp
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/

/**
*
* __Description__:
*
* UDP Procedures and functions source file, call UDP_init before the main Loop
* call UDP_Engine in the main loop of the programme to process any packages
*
*/

#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <json-c/json.h>     // required for json manipulation
#include <cstring>           // Required form memcpy
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h> /* Added for the nonblocking socket */
#include "hal.h"
#include "base64.h"
#include "udp.h"

typedef bool boolean;
typedef unsigned char byte;

 // Set location and altitude
 float lat=0;
 float lon=0;
 int   alt=0;

 // Informal status fields
 static char platform[24]    = "Single Channel Gateway";  /* platform definition */
 static char email[40]       = "";                        /* used for contact email */
 static char description[64] = "";                        /* used for free form description */

 byte receivedbytes;

 byte currentMode = 0x81;   /// check if this is used

 struct sockaddr_in si_other;
 int s, slen=sizeof(si_other);

 char message[256];
 char b64[256];

struct ifreq ifr;

 /**
 * __Function__: UDP_GetEth0Mac
 *
 * __Description__: Get the Ethernet 0 MAC address from the UDP layer
 *
 * __Input__: void
 *
 * __Output__: Error code: 0 = no error, -1 = Socket error
 *
 * __Status__: Work in Progress
 *
 * __Remarks__:
 */
int UDP_GetEth0Mac( struct ifreq *eth0_ifr)
{
  eth0_ifr = ifr;
  return 0;
}

 /**
 * __Function__: UDP_Init
 *
 * __Description__: UDP Initialisation
 *
 * __Input__: void
 *
 * __Output__: Error code: 0 = no error, -1 = Socket error
 *
 * __Status__: Work in Progress
 *
 * __Remarks__:
 */
int UDP_init( void )
{
  // Open Socket
  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    printf("UDP_init: Error creating a socket!\n");
    return -1;     /// Error code: -1 = socket error
  }
  // Change the socket into non-blocking state
  fcntl(s, F_SETFL, O_NONBLOCK);

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);

  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);  // can we rely on eth0?
  ioctl(s, SIOCGIFHWADDR, &ifr);

  return 0;
}



/**
* __Function__: UDP_Engine
*
* __Description__: UDP procedures to be called in the main loop
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, 1 =
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int UDP_Engine(void)
{

  // Check if any UDP packets are received
  UDP_Receive();

  // UDP Send ??....probably not required as we can send at any time to the server, we do not have to wait

  return 0;
}


/**
* __Function__: UDP_SendUDP
*
* __Description__: Send a UDP Message
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, 1 =
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int UDP_SendUDP(char *msg, int length)
{

  //Convert Server address to struct
  inet_aton(SERVER , &si_other.sin_addr);
  //Send UDP Package
  if (sendto(s, (char *)msg, length, 0 , (struct sockaddr *) &si_other, slen)==-1)
  {
      return -1;
  }
  return 0;
}


/**
* __Function__: UDP_Receive
*
* __Description__: Check if any UDP packets have been received, if so process
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, 1 =
*
* __Status__: Work in Progress
*
* __Remarks__: Procedure is called by UDP_Engine
*/
void UDP_Receive( void )
{
  char buffer[MAXLINE];  // Receive buffer
  char JsonPayload[MAXLINE];
  unsigned char RF_Payload[MAXLINE];

  char *RF_B64_Payload_Str;

  unsigned int len = 0;
  int RF_Len, ResultLen = 0;

  struct json_object *RF_B64_Payload;
  struct json_object *RF_Pkt_Len;
  struct json_object *RF_TX_Pkt;

  int NumBytes = 0;

  struct sockaddr_in cliaddr;

  struct json_object *PushPacket;


  len = sizeof(cliaddr);  //len is value/result



  inet_aton(SERVER , &cliaddr.sin_addr);

  NumBytes = recvfrom(s, (char *)buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);

  if(NumBytes != -1)
  {
    // Check what type of package is received, to do that, read the 4th byte, the identifier
    switch (buffer[3])
    {
      case PKT_PUSH_ACK:
        // Bytes   | Function
        // :------:|---------------------------------------------------------------------
        // 0       | protocol version = 2
        // 1-2     | same token as the PUSH_DATA packet to acknowledge
        // 3       | PUSH_ACK identifier 0x01
        printf("UDP_Receive: PUSH_ACK Received!\n");
      break;

      case PKT_PULL_RESP:
        // Bytes   | Function
        // :------:|---------------------------------------------------------------------
        // 0       | protocol version = 2
        // 1-2     | random token
        // 3       | PULL_RESP identifier 0x03
        // 4-end   | JSON object, starting with {, ending with }, see section 6
        printf("UDP_Receive: PULL_RESP : Received!\n");
        printf("UDP_Receive: PULL_RESP : Numbytes : %d \n", NumBytes);
        // Packet needs to be transmitted so that the end device can pick it up, but first lets check the package
        // Payload is received as a JSON:
        // {
        // 	"txpk": {...}
        // }

        // Copy JSON payload in a seperate buffer
        memcpy((void *)JsonPayload, (void *)(buffer + 4 ), NumBytes - 4);

        // Parse JSON package
        PushPacket = json_tokener_parse( JsonPayload );

        /// Debug
        printf("UDP_Receive: jobj from str:\n---\n%s\n---\n", json_object_to_json_string_ext(PushPacket, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

        // Decode payload (raw payload is b64 encoded)
        // txpk.data | string | Base64 encoded RF packet payload, padding optional
        // get raw data from json, ignore the rest for nowtime
        // Get length of raw data: size | number | RF packet payload size in bytes (unsigned integer)
        // First get top level JSON entry as the size and data are nested
        json_object_object_get_ex(PushPacket, "txpk", &RF_TX_Pkt);
        // Next get the size object
        json_object_object_get_ex(RF_TX_Pkt, "size", &RF_Pkt_Len);

        RF_Len = json_object_get_int(RF_Pkt_Len);
        /// Debug
        printf("UDP_Receive: RF Packet length : %d ", RF_Len);


        // Next get the data object = string
        json_object_object_get_ex(RF_TX_Pkt, "data", &RF_B64_Payload);

        RF_B64_Payload_Str = (char *) json_object_get_string(RF_B64_Payload);

        // Decode packet, use the length of the b64 sting as a length not the size recovered from the received packet!
        if(( ResultLen = b64_to_bin(RF_B64_Payload_Str, strlen(RF_B64_Payload_Str), RF_Payload, MAXLINE)) > 1)
         {
           /// Debug
           printf("UDP_Receive: B64 to bin length : %d \n", ResultLen );
         }
         else
         {
           printf("UDP_Receive: B64 error: %d\n", ResultLen);
           break;
         }
         // Ok now we have the decoded package on RF_B64_Payload_Str and the size in RF_Len
         // Only send when node is listening



      break;

      case PKT_PULL_ACK:
        // Bytes   | Function
        // :------:|---------------------------------------------------------------------
        // 0       | protocol version = 2
        // 1-2     | same token as the PULL_DATA packet to acknowledge
        // 3       | PULL_ACK identifier 0x04
        printf("UDP_Receive: PULL_ACK Received!\n");
      break;

      default:
        printf("UDP_Receive: Unknown package received!\n");

    }

  }
  else
  {
    //printf("Nothing received\n");
  }


}












char UDP_GetProtoVersion( void )
{
  return PROTOCOL_VERSION;
}
