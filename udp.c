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

/// TODO: Change init to accept server IP and port

/**
*
* __Description__:
*
* UDP Procedures and functions source file, call UDP_Init before the main Loop
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
#include <cstring>            // Required for memcpy
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>            // Added for the nonblocking socket
#include "base64.h"
#include "udp.h"

typedef bool boolean;
typedef unsigned char byte;

struct sockaddr_in ServerAddr;  // Struct var to store the Server address
int ServerSocket;               // Server Socket
int slen=sizeof(ServerAddr);    // Length of server address
struct ifreq ifr;               // Struct far to store the MAC address of ETH0

/**
* UDP TX_Buffer structure
*/
struct UDP_TX_BUFFER_STRUCT {
 uint8_t   UDP_TX_FRAME[UDP_TX_MX_FRAME_SIZE];      /**< TX Frame */
 byte      UDP_TX_FRAME_SIZE;                       /**< Size of frame to transmit */
 uint8_t   UDP_TX_FLAG;                             /**< TX_FLAG: 0 = No frame to send, 1 = Frame to send */
 /// Maybe add other data, flags etc?
};
/**
* UDP TX FIFO Buffer
*/
struct UDP_TX_BUFFER_STRUCT UDP_TX_FIFO_Buffer[UDP_TX_FIFO_DEPTH];
/**
* TXFifo index
*/
uint8_t UDP_TX_FIFO_Idx = 0;

/**
* UDP RX_Buffer structure
*/
struct UDP_RX_BUFFER_STRUCT {
 uint8_t   UDP_RX_FRAME[UDP_RX_MX_FRAME_SIZE];      /**< RX Frame */
 byte      UDP_RX_FRAME_SIZE;                       /**< Size of frame received */
 uint8_t   UDP_RX_FLAG;                             /**< RX_FLAG: 0 = No frame received, 1 = Frame received */
 /// Maybe add other data, flags etc?
};
/**
* UDP RX FIFO Buffer
*/
struct UDP_RX_BUFFER_STRUCT UDP_RX_FIFO_Buffer[UDP_RX_FIFO_DEPTH];
/**
* RX Fifo index
*/
uint8_t UDP_RX_FIFO_Idx = 0;


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
 * __Remarks__: Sets up non-blocking socket with server x on port y
 */
int UDP_Init( void )
{
  // Init vars
  // Make sure the TX fifo pointer points to the first entry in the fifo
  UDP_TX_FIFO_Idx = 0;
  // Make sure the RX fifo pointer points to the first entry in the fifo
  UDP_RX_FIFO_Idx = 0;

  // Open Socket
  if (( ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    printf("UDP_init: Error creating a socket!\n");
    return -1;     /// Error code: -1 = socket error
  }
  // Change the socket into non-blocking state
  fcntl(ServerSocket, F_SETFL, O_NONBLOCK);

  memset((char *) &ServerAddr, 0, sizeof(ServerAddr));
  ServerAddr.sin_family = AF_INET;
  ServerAddr.sin_port = htons(PORT);

  // Load the server address in the structure var
  inet_aton(SERVER , &ServerAddr.sin_addr);

  // Get the mac address of ETH to be used as gateway address
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);  // can we rely on eth0?
  ioctl(ServerSocket, SIOCGIFHWADDR, &ifr);

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

  // Check if any UDP packets are received and put them in the UDP RX FIFO
  UDP_CheckRX();

  // UDP Send messages put in to the UDP TX FIFO if Any
  UDP_CheckTX();

  return 0;
}

/**
* __Function__: UDP_SendUDP
*
* __Description__: Send a UDP Message
*
* __Input__: char *msg, pointer to the message to be transmitted, length of packages
*
* __Output__: Error code: 0 = no error, 1 = UDP TX Buffer Full, 2 = FRame to big
*
* __Status__: Work in Progress
*
* __Remarks__: Procedure to be called from application, add a frame to the TX Buffer
*/
int UDP_SendUDP(char *TxFrame, int FrameSize)
{
  /// __Incode Comments:__

  // check for space in UDP TX FIFO
  // Buffer Index runs from 0 to UDP_FIFO_DEPTH - 1
  if(UDP_TX_FIFO_Idx < (UDP_TX_FIFO_DEPTH))
  {
    if(FrameSize <= UDP_TX_MX_FRAME_SIZE)
    {
      // Copy frame in buffer
      memcpy(UDP_TX_FIFO_Buffer[UDP_TX_FIFO_Idx].UDP_TX_FRAME, TxFrame, FrameSize);
      // set send flag
      UDP_TX_FIFO_Buffer[UDP_TX_FIFO_Idx].UDP_TX_FLAG = 1;                 // Set flag to one to indicate there is a frame to be send
      UDP_TX_FIFO_Buffer[UDP_TX_FIFO_Idx].UDP_TX_FRAME_SIZE = FrameSize;   // Add frame size
      //printf("LW_AddFrameToTXBuffer: Frame added to buffer at position: %d\n", LW_TX_FIFO_Idx );
      //Increase the fifo index
      UDP_TX_FIFO_Idx++;
      // No error, return
      /// The sending of the frame from the UDP TX Fifo is handled in UDP_Engine (UDP_Transmit)
      return 0;
    }
    else
    {
      printf("UDP_SendUDP: FrameSize to big, frame cannot be send!\n");
      return 2;   /// Error 2: FrameSize to big
    }

  }
  else
  {
    // Buffer full
    printf("UDP_SendUDP: Buffer full, frame cannot be send!\n");
    return 1;       /// Error 1: TX Buffer full
  }
}

/**
* __Function__: UDP_ReceiveUDP
*
* __Description__: Check if any UDP packets have been received and are in the FIFO buffer
*
* __Input__: char *buffer = pointer to a buffer
*
* __Output__: Number of bytes or nothing in FIFO = -1
*
* __Status__: Work in Progress
*
* __Remarks__: Procedure is called by the application
*/
int UDP_ReceiveUDP( char *RxBuffer )
{
  // Check for message in FIFO, if not available return -1
  if( UDP_RX_FIFO_Buffer[0].UDP_RX_FLAG != 0)
  {
    // Copy the frame from the FIFO in the application buffer
    memcpy( RxBuffer, UDP_RX_FIFO_Buffer[0].UDP_RX_FRAME, UDP_RX_FIFO_Buffer[0].UDP_RX_FRAME_SIZE);
    UDP_RX_FIFO_Buffer[0].UDP_RX_FLAG = 0;                  // Set flag to 0 to indicate frame has been processed
    UDP_RX_FIFO_Update();                                   // Move received frames down the UDP RX FIFO

    printf("UDP_ReceiveUDP: RX Frame processed\n");         /// Debug
    return UDP_RX_FIFO_Buffer[0].UDP_RX_FRAME_SIZE;         // Return number of bytes received
  }
  else
  {
    // Nothing in the UDP RX FIFO
    return -1;
  }
}

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
 eth0_ifr = &ifr;
 return 0;
}

/**
* __Function__: UDP_Transmit
*
* __Description__: Send a UDP Message
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, -1 = unable to send UDP frame
*
* __Status__: Work in Progress
*
* __Remarks__: Procedure to be called from UDP engine, sending TX frames from the UDP TX FIFO
*/
int UDP_CheckTX( void )
{
  // Check UDP TX fifo
  if( UDP_TX_FIFO_Buffer[0].UDP_TX_FLAG != 0)
  {
    // Send the first message in the Fifo
    printf("UDP_Transmit: There is something to send!\n");    /// Debug

    // Send the frame
    if( sendto(ServerSocket, UDP_TX_FIFO_Buffer[0].UDP_TX_FRAME, UDP_TX_FIFO_Buffer[0].UDP_TX_FRAME_SIZE, 0 , (struct sockaddr *) &ServerAddr, slen) == -1 )
    {
      // error
      printf("UDP_Transmit: Send frame error!\n");
      return -1;
    }
    else
    {
      //Move frames down the Fifo
      printf("UDP_Transmit: TX Frame processed\n");   /// Debug
      UDP_TX_FIFO_Buffer[0].UDP_TX_FLAG = 0;          // Set flag to 0 to indicate frame has been processed
      UDP_TX_FIFO_Update();                           // Shift frames fown the FIFO if applicable
    }
  }
  else
  {
    // Nothing to send skip
    //printf("UDP_Transmit: Nothing to send!\n");      /// Debug
  }
  return 0;
}

/**
* __Function__: UDP_CheckRX
*
* __Description__: Check if any UDP packets have been received, if so add to UDP FIFO
*
* __Input__: void
*
* __Output__: Error code: 0 = nothing received, -1 = FIF full, Error code > 0 = number of bytes
*
* __Status__: Work in Progress
*
* __Remarks__: Procedure is called by UDP_Engine to get UDP frames and stores them in the UDP RX FIFO
*/
int UDP_CheckRX( void )
{
  int NumRXBytes = 0;                   // Number of Bytes received
  char RxBuffer[MAXLINE];               // Receive buffer
  unsigned int AddressLength = 0;       // Address length of server sending packet
  struct sockaddr_in SenderAddr;        // struct to store the sender (in this case the server) address in


  if(UDP_RX_FIFO_Idx < UDP_RX_FIFO_DEPTH)
  {
    // There is space in the UDP RX FIFO so lets get a package
    NumRXBytes = recvfrom(ServerSocket, (char *)RxBuffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &SenderAddr, &AddressLength);

    /// Do I need to double check the package received is from the server to avoid spoofing ?

    if(NumRXBytes != -1)
    {
      // Add to UDP FIFO buffer
      memcpy(UDP_RX_FIFO_Buffer[UDP_RX_FIFO_Idx].UDP_RX_FRAME, RxBuffer, NumRXBytes);
      // set send flag
      UDP_RX_FIFO_Buffer[UDP_RX_FIFO_Idx].UDP_RX_FLAG = 1;                   // Set flag to one to indicate there is a frame to be send
      UDP_RX_FIFO_Buffer[UDP_RX_FIFO_Idx].UDP_RX_FRAME_SIZE = NumRXBytes;   // Add frame size
      printf("UDP_Receive: Frame received and added to buffer at position: %d\n", UDP_RX_FIFO_Idx );
      //Increase the fifo index
      UDP_RX_FIFO_Idx++;
      return NumRXBytes;
    }
    else
    {
      //nothing received
      return 0;

    }
  }
  else
  {
    // Error, RX FIFO is Full
    printf("UDP_Receive: Error, RX FIFO full, check processing of incoming packets!\n");
    return -1;      /// error -1 : RX FIFO full
  }
}

/**
 * __Function__: UDP_TX_FIFO_Update
 *
 * __Description__: Move frames down the fifo
 *
 * __Input__: Void
 *
 * __Output__: void
 *
 * __Status__: Completed
 *
 * __Remarks__: none
 */
void UDP_TX_FIFO_Update( void )
{
  int i;

  // printf("LW_FIFO_Update: index : %d \n", LW_TX_FIFO_Idx);

  if(UDP_TX_FIFO_Idx)
  {
    // update FIFO if Required
    for(i=0; i < (UDP_TX_FIFO_DEPTH - 1); ++i)
    {
      // Move frames down
      UDP_TX_FIFO_Buffer[i] = UDP_TX_FIFO_Buffer[i+1];
      // printf("LW_FIFO_Update: Move queue position : %d to : %d \n", i+1, i);
    }
    //Decrease the fifo index
    UDP_TX_FIFO_Idx--;
    // Make sure the flag is set to 0 to indicate there is space in the buffer
    UDP_TX_FIFO_Buffer[UDP_TX_FIFO_DEPTH - 1].UDP_TX_FLAG = 0;
    // printf("LW_FIFO_Update: Updating position : %d to indicate free space!\n", LW_FIFO_DEPTH - 1 );
    // printf("LW_FIFO_Update: New FIFO Idx: %d\n", LW_TX_FIFO_Idx );
  }
  else
  {
    // Nothing to do, IDX at 0
    // printf("LW_FIFO_Update: Nothing to do, idx at : 0\n");
  }
}

/**
 * __Function__: UDP_RX_FIFO_Update
 *
 * __Description__: Move frames down the fifo
 *
 * __Input__: Void
 *
 * __Output__: void
 *
 * __Status__: Completed
 *
 * __Remarks__: none
 */
void UDP_RX_FIFO_Update( void )
{
  int i;

  // printf("LW_FIFO_Update: index : %d \n", LW_TX_FIFO_Idx);

  if(UDP_RX_FIFO_Idx)
  {
    // update FIFO if Required
    for(i=0; i < (UDP_RX_FIFO_DEPTH - 1); ++i)
    {
      // Move frames down
      UDP_RX_FIFO_Buffer[i] = UDP_RX_FIFO_Buffer[i+1];
      // printf("LW_FIFO_Update: Move queue position : %d to : %d \n", i+1, i);
    }
    //Decrease the fifo index
    UDP_RX_FIFO_Idx--;
    // Make sure the flag is set to 0 to indicate there is space in the buffer
    UDP_RX_FIFO_Buffer[UDP_RX_FIFO_DEPTH - 1].UDP_RX_FLAG = 0;
    // printf("LW_FIFO_Update: Updating position : %d to indicate free space!\n", LW_FIFO_DEPTH - 1 );
    // printf("LW_FIFO_Update: New FIFO Idx: %d\n", LW_TX_FIFO_Idx );
  }
  else
  {
    // Nothing to do, IDX at 0
    // printf("LW_FIFO_Update: Nothing to do, idx at : 0\n");
  }
}
