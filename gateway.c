/*******************************************************************************
 * Application layer
 *
 * Used base source code from other authors, see credits below but I have
 * modified it to suit my own needs.
 *
 * Dependencies: wiringPi
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

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include "lorawan.h"
#include "udp.h"
#include "hal.h"
#include "gateway.h"


// Timers
uint32_t Status_lasttime;     // Send regular status updated from the GW to the server
uint32_t PushData_lasttime;   // Send PushData frames to keep channel open

// describe the below better
uint32_t cp_nb_rx_rcv;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

// variable to store mac address of gateway
struct ifreq GW_ifr;

// Informal status fields
static char platform[24]    = "Single Channel Gateway";  /* platform definition */
static char email[40]       = "";                        /* used for contact email */
static char description[64] = "";                        /* used for free form description */

byte receivedbytes;

byte currentMode = 0x81;   /// check if this is used

char message[256];
char b64[256];

/**
* __Function__: GW_Init
*
* __Description__: GW Initialisation procedure
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int GW_Init(void)
{
  int sf = 0;
  uint32_t freq = 0;

  // get the Eth0 Mac address as this is used for the Gateway EUI
  UDP_GetEth0Mac(&GW_ifr);


  // Get the Lora Spreading Factor
  sf = HAL_GetSF();
  // Get the Lora Frequency used
  freq = HAL_GetFreq();

  // Inform the user of the settings of the GW --> later change to Oled
  printf("--------------------------------------------------------\n");
  printf("Listening at SF%i on %.6lf Mhz.\n", sf,(double)freq/1000000);
  printf("--------------------------------------------------------\n");
  printf("Gateway ID: %.2x:%.2x:%.2x:ff:ff:%.2x:%.2x:%.2x\n",
    (unsigned char)GW_ifr.ifr_hwaddr.sa_data[0],
    (unsigned char)GW_ifr.ifr_hwaddr.sa_data[1],
    (unsigned char)GW_ifr.ifr_hwaddr.sa_data[2],
    (unsigned char)GW_ifr.ifr_hwaddr.sa_data[3],
    (unsigned char)GW_ifr.ifr_hwaddr.sa_data[4],
    (unsigned char)GW_ifr.ifr_hwaddr.sa_data[5]);
  printf("--------------------------------------------------------\n");

  // Send status update to the server
  GW_SendStat();
  // Send pull data request to server
  GW_SendPullData();
  // No errors
  return 0;
}

/**
* __Function__: GW_Engine
*
* __Description__: GW Engine to be called in the main loop to execute Gateway tasks
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int GW_Engine(void)
{

 // Check on Lora packate received, if yes process and send through UDP_Init



 // Check on UDP Packet received, if yes process and forward to LoRa
 GW_ProcessRX_UDP();

 // Send status updated to server
 GW_SendGWStatusUpate();

 // Send regular Pull data requests to the server to allow downstream traffic
 GW_SendPullDataTMR();

 return 0;
}




/**
* __Function__: GW_SendGWStatusUpate
*
* __Description__: Procedure to send Gatway status updates to the server
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int GW_SendGWStatusUpate(void)
{
  struct timeval nowtime;

  gettimeofday(&nowtime, NULL);
  uint32_t nowseconds = (uint32_t)(nowtime.tv_sec);
  if (nowseconds - Status_lasttime >= TMR_STAT_TX) {
      Status_lasttime = nowseconds;
      GW_SendStat();
      cp_nb_rx_rcv = 0;
      cp_nb_rx_ok = 0;
      cp_up_pkt_fwd = 0;
  }
  return 0;
}


/**
* __Function__: GW_Engine
*
* __Description__: GW Engine to be called in the main loop to execute Gateway tasks
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int GW_SendPullDataTMR(void)
{
  struct timeval nowtime;

  gettimeofday(&nowtime, NULL);
  uint32_t nowseconds = (uint32_t)(nowtime.tv_sec);
  if (nowseconds - PushData_lasttime >= TMR_TX_PULL) {
      PushData_lasttime = nowseconds;
      // Send PULL_DATA
      UDP_SendPullData();
      cp_nb_rx_rcv = 0;
      cp_nb_rx_ok = 0;
      cp_up_pkt_fwd = 0;
  }
  return 0;
}

/**
* __Function__: GW_SendStat
*
* __Description__: Gateway Send Stats to server
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__: Send a status up date to the server
*/
void GW_SendStat() {

    static char status_report[STATUS_SIZE]; /* status report as a JSON object */
    char stat_timestamp[24];
    time_t t;

    int stat_index=0;

    /* pre-fill the data buffer with fixed fields */
    status_report[0] = PROTOCOL_VERSION;
    status_report[3] = PKT_PUSH_DATA;

    status_report[4] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[0];
    status_report[5] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[1];
    status_report[6] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[2];
    status_report[7] = 0xFF;
    status_report[8] = 0xFF;
    status_report[9] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[3];
    status_report[10] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[4];
    status_report[11] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[5];

    /* start composing datagram with the header */
    uint8_t token_h = (uint8_t)rand(); /* random token */
    uint8_t token_l = (uint8_t)rand(); /* random token */
    status_report[1] = token_h;
    status_report[2] = token_l;
    stat_index = 12; /* 12-byte header */

    /* get timestamp for statistics */
    t = time(NULL);
    strftime(stat_timestamp, sizeof stat_timestamp, "%F %T %Z", gmtime(&t));

    int j = snprintf((char *)(status_report + stat_index), STATUS_SIZE-stat_index, "{\"stat\":{\"time\":\"%s\",\"lati\":%.5f,\"long\":%.5f,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%.1f,\"dwnb\":%u,\"txnb\":%u,\"pfrm\":\"%s\",\"mail\":\"%s\",\"desc\":\"%s\"}}", stat_timestamp, lat, lon, (int)alt, cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd, (float)0, 0, 0,platform,email,description);
    stat_index += j;
    status_report[stat_index] = 0; /* add string terminator, for safety */

    printf("stat update: %s\n", (char *)(status_report+12)); /* DEBUG: display JSON stat */

    //send the Gateway status updates to the server
    if(UDP_SendUDP(status_report, stat_index))
    {
      // if not 0 = error
      printf("GW_SendStat: Error sending UDP!");
    }
}

/**
* __Function__: GW_SendPullData
*
* __Description__: Gateway Send Pull Data request to the sever to enable down stream comms
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__:
*
*  Bytes  | Function
* :------:|---------------------------------------------------------------------
*  0      | protocol version = 2
*  1-2    | random token
*  3      | PULL_DATA identifier 0x02
*  4-11   | Gateway unique identifier (MAC address)
*/
int GW_SendPullData()
{
  char buff_up[PULL_DATA_PKT_LEN]; /* buffer to compose the upstream packet */
  int buff_index=PULL_DATA_PKT_LEN;

  // Add protocol version
  buff_up[0] = PROTOCOL_VERSION;

  // Add random toker
  uint8_t token_h = (uint8_t)rand(); /* random token */
  uint8_t token_l = (uint8_t)rand(); /* random token */
  buff_up[1] = token_h;
  buff_up[2] = token_l;

  // Add pull data identifier
  buff_up[3] = PKT_PULL_DATA;

  // Add gateway ID (derived from eth0 MAC address)
  buff_up[4]  = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[0];
  buff_up[5]  = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[1];
  buff_up[6]  = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[2];
  buff_up[7]  = 255;  // 0xff in the middle
  buff_up[8]  = 255;  // 0xff in the middle
  buff_up[9]  = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[3];
  buff_up[10] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[4];
  buff_up[11] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[5];


  /* display result */
  // printf("send_pull_data: ID: %.2x:%.2x:%.2x:ff:ff:%.2x:%.2x:%.2x\n",
  //        (unsigned char)ifr.ifr_hwaddr.sa_data[0],
  //        (unsigned char)ifr.ifr_hwaddr.sa_data[1],
  //        (unsigned char)ifr.ifr_hwaddr.sa_data[2],
  //        (unsigned char)ifr.ifr_hwaddr.sa_data[3],
  //        (unsigned char)ifr.ifr_hwaddr.sa_data[4],
  //        (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

  // Send Pull data requests to the server
  if( UDP_SendUDP(buff_up, buff_index))
  {
    // Error if not 0
    printf("GW_SendPullData: Error sending UDP!\n");
  }

  fflush(stdout);

  return 0;
}

/**
* __Function__: GW_ProcessRX_UDP
*
* __Description__: Gateway check UDP packages received and process
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__: Function to be called from gateway engine
*/
GW_ProcessRX_UDP()
{
  char buffer[MAXLINE];  // Receive buffer
  char JsonPayload[MAXLINE];
  unsigned char RF_Payload[MAXLINE];
  char *RF_B64_Payload_Str;
  unsigned int len = 0;
  int RF_Len, ResultLen = 0;
  int NumBytes = 0;

  struct json_object *RF_B64_Payload;
  struct json_object *RF_Pkt_Len;
  struct json_object *RF_TX_Pkt;
  struct json_object *PushPacket;

  // Change this to get message from UDP FIFO RX Buffer
  NumBytes = UDP_RetreiveRXfromFIFO((char *)buffer);

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
        printf("GW_ProcessRX_UDP: PUSH_ACK Received!\n");
      break;

      case PKT_PULL_RESP:
        // Bytes   | Function
        // :------:|---------------------------------------------------------------------
        // 0       | protocol version = 2
        // 1-2     | random token
        // 3       | PULL_RESP identifier 0x03
        // 4-end   | JSON object, starting with {, ending with }, see section 6
        printf("GW_ProcessRX_UDP: PULL_RESP : Received!\n");
        printf("GW_ProcessRX_UDP: PULL_RESP : Numbytes : %d \n", NumBytes);
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
        printf("GW_ProcessRX_UDP: jobj from str:\n---\n%s\n---\n", json_object_to_json_string_ext(PushPacket, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

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
        printf("GW_ProcessRX_UDP: RF Packet length : %d ", RF_Len);


        // Next get the data object = string
        json_object_object_get_ex(RF_TX_Pkt, "data", &RF_B64_Payload);

        RF_B64_Payload_Str = (char *) json_object_get_string(RF_B64_Payload);

        // Decode packet, use the length of the b64 sting as a length not the size recovered from the received packet!
        if(( ResultLen = b64_to_bin(RF_B64_Payload_Str, strlen(RF_B64_Payload_Str), RF_Payload, MAXLINE)) > 1)
         {
           /// Debug
           printf("GW_ProcessRX_UDP: B64 to bin length : %d \n", ResultLen );
         }
         else
         {
           printf("GW_ProcessRX_UDP: B64 error: %d\n", ResultLen);
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
        printf("GW_ProcessRX_UDP: PULL_ACK Received!\n");
      break;

      default:
        printf("GW_ProcessRX_UDP: Unknown package received!\n");

    }

  }
  else
  {
    //printf("Nothing received\n");
  }

}
