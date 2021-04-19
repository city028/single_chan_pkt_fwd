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
* __Remarks__:
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

    //send the update
    UDP_SendUDP(status_report, stat_index);
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
int GW_SendPullPata()
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

  // Send package
  UDP_SendUDP(buff_up, buff_index);

  fflush(stdout);

  return 0;
}
