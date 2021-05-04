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
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cstring>            // Required for memcpy
#include <json-c/json.h>     // required for json manipulation
#include <time.h>
#include "udp.h"
#include "hal.h"
#include "gateway.h"
#include "base64.h"


// Timers
uint32_t Status_lasttime;     // Send regular status updated from the GW to the server
uint32_t PushData_lasttime;   // Send PushData frames to keep channel open


// variable to store mac address of gateway
struct ifreq GW_ifr;

// Informal status fields
static char platform[24]    = "Single Channel Gateway";  /* platform definition */
static char email[40]       = "";                        /* used for contact email */
static char description[64] = "";                        /* used for free form description */

byte currentMode = 0x81;   /// check if this is used

int SpreadingFactor = 0;           // Spreading Factor
uint32_t LoraFreq = 0;    // Lora Frequency Used
/// Little hack to get the TTS to think we are working with 868 modules whilst I only have 433 modules....to be replace soon
double  freq2 = 868100000; // in Mhz! (868.1)

// Set location and altitude
float lat=0;
float lon=0;
int   alt=0;

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


  // get the Eth0 Mac address as this is used for the Gateway EUI
  UDP_GetEth0Mac(&GW_ifr);


  // Get the Lora Spreading Factor
  SpreadingFactor = HAL_GetSF();
  // Get the Lora Frequency used
  LoraFreq = HAL_GetFreq();

  // Inform the user of the settings of the GW --> later change to Oled
  printf("--------------------------------------------------------\n");
  printf("Listening at SF%i on %.6lf Mhz.\n", SpreadingFactor,(double)LoraFreq/1000000);
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

 // Check on Lora packet received, if yes process and send through UDP_Init
 GW_ProcessRX_Lora();

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
      GW_SendPullData();
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
void GW_SendStat()
{

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

    uint32_t NumRx = HAL_GetNumRX();
    uint32_t RxOk = HAL_GetRxOk();
    uint32_t PktFwd = HAL_GetPktWfd();


    int j = snprintf((char *)(status_report + stat_index), STATUS_SIZE-stat_index, "{\"stat\":{\"time\":\"%s\",\"lati\":%.5f,\"long\":%.5f,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%.1f,\"dwnb\":%u,\"txnb\":%u,\"pfrm\":\"%s\",\"mail\":\"%s\",\"desc\":\"%s\"}}", stat_timestamp, lat, lon, (int)alt, NumRx, RxOk, PktFwd, (float)0, 0, 0,platform,email,description);
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
void GW_ProcessRX_UDP(void)
{
  char buffer[MAXLINE];  // Receive buffer
  char JsonPayload[MAXLINE];
  unsigned char RF_Payload[MAXLINE];
  char *RF_B64_Payload_Str;
  //unsigned int len = 0;
  int RF_Len, ResultLen = 0;
  int NumBytes = 0;

  struct json_object *RF_B64_Payload;
  struct json_object *RF_Pkt_Len;
  struct json_object *RF_TX_Pkt;
  struct json_object *PushPacket;

  // Change this to get message from UDP FIFO RX Buffer
  NumBytes = UDP_ReceiveUDP((char *)buffer);

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
        printf("GW_ProcessRX_UDP: PULL_RESP : Received!!!!!\n");
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
        printf("GW_ProcessRX_UDP: RF Packet length : %d \n", RF_Len);


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
         printf("GW_ProcessRX_UDP: FRame handed of to Lora for transmit to node \n");
         printf("GW_ProcessRX_UDP: MAC Header:");
         OS_PrintBin( (byte)RF_Payload[0]);
         printf("\n");

         // Send out the frame using LORA
         HAL_TransmitFrame(RF_Payload, ResultLen);

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


/**
* __Function__: GW_ProcessRX_Lora
*
* __Description__: Gateway check Lora packages received and process
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = Socket error
*
* __Status__: Work in Progress
*
* __Remarks__: Function to be called from gateway engine
*/
/// JS Clean this up, call HAL to retreive message from Fifo
int GW_ProcessRX_Lora()
{
  uint8_t Lora_RX_Message[LORA_RX_MX_FRAME_SIZE];
  int RxNumBytes;
  //int BytesProcessed;
  char buff_up[TX_BUFF_SIZE];   /* buffer to compose the upstream packet */
  int buff_index=0;
  struct timeval now;
  int j;
  long int snr;
  int rssicorr;

  snr = HAL_GetSNR();
  rssicorr = HAL_GetRssiCor();

  // Check if there is a Lora message in the Lora FIFO
  if((RxNumBytes = HAL_ReceiveFrame(Lora_RX_Message)) > 0)
  {
    printf("GW_ProcessRX_Lora: Package received with: %d bytes \n", RxNumBytes);
    // Message received, convert to B64 message
    //BytesProcessed = bin_to_b64(Lora_RX_Message, RxNumBytes, (char *)(b64), 341);

    //  Bytes  | Function
    // :------:|---------------------------------------------------------------------
    //  0      | protocol version = 2
    //  1-2    | random token
    //  3      | PUSH_DATA identifier 0x00
    //  4-11   | Gateway unique identifier (MAC address)
    //  12-end | JSON object, starting with {, ending with }, see section 4

    /* pre-fill the data buffer with fixed fields */
    buff_up[0] = PROTOCOL_VERSION;
    /* start composing datagram with the header */
    uint8_t token_h = (uint8_t)rand(); /* random token */
    uint8_t token_l = (uint8_t)rand(); /* random token */
    buff_up[1] = token_h;
    buff_up[2] = token_l;
    // Add PUSH_Data Identifier
    buff_up[3] = PKT_PUSH_DATA;

    // Add the gateway unique ID
    buff_up[4] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[0];
    buff_up[5] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[1];
    buff_up[6] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[2];
    buff_up[7] = 0xFF;
    buff_up[8] = 0xFF;
    buff_up[9] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[3];
    buff_up[10] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[4];
    buff_up[11] = (unsigned char)GW_ifr.ifr_hwaddr.sa_data[5];
    // Pint index to point 12 in the buffer
    buff_index = 12; /* 12-byte header */

    // TODO: tmst can jump is time is (re)set, not good.      /// Check this do not understand what is meant here
    gettimeofday(&now, NULL);
    uint32_t tmst = (uint32_t)(now.tv_sec*1000000 + now.tv_usec);

    /* start of JSON structure */
    memcpy((void *)(buff_up + buff_index), (void *)"{\"rxpk\":[", 9);
    buff_index += 9;
    buff_up[buff_index] = '{';
    ++buff_index;
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, "\"tmst\":%u", tmst);
    buff_index += j;
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"chan\":%1u,\"rfch\":%1u,\"freq\":%.6lf", 0, 0, freq2/1000000);
    buff_index += j;
    memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":1", 9);
    buff_index += 9;
    memcpy((void *)(buff_up + buff_index), (void *)",\"modu\":\"LORA\"", 14);
    buff_index += 14;
    /* Lora datarate & bandwidth, 16-19 useful chars */
    switch (SpreadingFactor) {
      case SF7:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF7", 12);
          buff_index += 12;
          break;
      case SF8:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF8", 12);
          buff_index += 12;
          break;
      case SF9:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF9", 12);
          buff_index += 12;
          break;
      case SF10:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF10", 13);
          buff_index += 13;
          break;
      case SF11:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF11", 13);
          buff_index += 13;
          break;
      case SF12:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF12", 13);
          buff_index += 13;
          break;
      default:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF?", 12);
          buff_index += 12;
    }
    memcpy((void *)(buff_up + buff_index), (void *)"BW125\"", 6);
    buff_index += 6;
    memcpy((void *)(buff_up + buff_index), (void *)",\"codr\":\"4/5\"", 13);
    buff_index += 13;
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"lsnr\":%li", snr);
    buff_index += j;
    j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"rssi\":%d,\"size\":%u", HAL_readRegister(0x1A)-rssicorr, RxNumBytes);
    buff_index += j;
    memcpy((void *)(buff_up + buff_index), (void *)",\"data\":\"", 9);
    buff_index += 9;
    j = bin_to_b64((uint8_t *)Lora_RX_Message, RxNumBytes, (char *)(buff_up + buff_index), TX_BUFF_SIZE);    /// Why 341, check it out
    buff_index += j;
    buff_up[buff_index] = '"';
    ++buff_index;

    /* End of packet serialization */
    buff_up[buff_index] = '}';
    ++buff_index;
    buff_up[buff_index] = ']';
    ++buff_index;
    /* end of JSON datagram payload */
    buff_up[buff_index] = '}';
    ++buff_index;
    buff_up[buff_index] = 0; /* add string terminator, for safety */

    printf("GW_ProcessRX_Lora: %s\n", (char *)(buff_up + 12)); /* DEBUG: display JSON payload */

    //send the message using UDP
    if( UDP_SendUDP(buff_up, buff_index))
    {
      printf("GW_ProcessRX_Lora: Error sending UDP \n");
    }

    printf("GW_ProcessRX_Lora: Package handed over to UDP with Length: %d \n", buff_index);
    fflush(stdout);       /// Why do we need this?
  } // No message in FIFO
  return 0;
}

/**
 * __Function__: OS_PrintBin
 *
 * __Description__: Print the binary value of an integer
 *
 * __Input__: Integer to be printed in Binary
 *
 * __Output__: void
 *
 * __Status__: Complete
 *
 * __Remarks__: None
 */
void OS_PrintBin(byte x)
{
   /// __Incode Comments:__
   int i;
   printf("0b");
   for(i=0; i<8; i++)
   {
      if((x & 0x80) !=0)
      {
         printf("1");
      }
      else
      {
         printf("0");
      }
      if (i==3)
      {
         printf(" ");
      }
      x = x<<1;
   }
}
