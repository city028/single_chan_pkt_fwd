/*******************************************************************************
 * hardware Abstraction Layer (HAL)
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
#include <stdint.h>           // Required for unint8 etc
#include <wiringPi.h>         // Required for using wiringPi
#include <wiringPiSPI.h>      // Required for using SPI
#include <cstring>            // Required for memcpy
#include "hal.h"              // The header file for this
#include "base64.h"
#include "udp.h"

/**
*
* User defined variables below!
*
*/
uint32_t  freq = 433175000;     /// in Mhz! (433.175) = 433Mhz channel 1
//uint32_t  freq = 868100000;   /// in Mhz! (868.1) = 868Mhz channel 1

/// Set spreading factor (SF7 - SF12)
sf_t sf = SF7;

// End of user defined variables!


// HAL Variables
int RST   = 0;
bool sx1272 = true;

// SX1272 - Raspberry connections
int ssPin = 6;
int dio0  = 7;

// Message buffer

char b64[256];

// Received number of bytes by RF
byte receivedbytes;

uint32_t cp_nb_rx_rcv = 0;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

/**
* Lora TX_Buffer structure
*/
struct LORA_TX_BUFFER_STRUCT {
 uint8_t   LORA_TX_FRAME[LORA_TX_MX_FRAME_SIZE];      /**< LORA TX Frame */
 byte      LORA_TX_FRAME_SIZE;                       /**< Size of frame to transmit */
 uint8_t   LORA_TX_FLAG;                             /**< TX_FLAG: 0 = No frame to send, 1 = Frame to send */
 /// Maybe add other data, flags etc?
};
/**
* LORA TX FIFO Buffer
*/
struct LORA_TX_BUFFER_STRUCT LORA_TX_FIFO_Buffer[LORA_TX_FIFO_DEPTH];
/**
* TXFifo index
*/
uint8_t LORA_TX_FIFO_Idx = 0;

/**
* UDP RX_Buffer structure
*/
struct LORA_RX_BUFFER_STRUCT {
 uint8_t    LORA_RX_FRAME[LORA_RX_MX_FRAME_SIZE];      /**< RX Frame */
 byte       LORA_RX_FRAME_SIZE;                       /**< Size of frame received */
 uint8_t    LORA_RX_FLAG;                             /**< RX_FLAG: 0 = No frame received, 1 = Frame received */
 byte       LORA_RX_RSSI;
 byte       LORA_RX_PACKET_RSSI;
 long int   LORA_RX_SNR;
 /// Maybe add other data, flags etc?
};
/**
* UDP RX FIFO Buffer
*/
struct LORA_RX_BUFFER_STRUCT LORA_RX_FIFO_Buffer[LORA_RX_FIFO_DEPTH];
/**
* RX Fifo index
*/
uint8_t LORA_RX_FIFO_Idx = 0;


long int SNR;
int rssicorr;



/**
* __Function__: HAL_Init
*
* __Description__: Initalise Hardware Abstraction Layer (HAL)
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, 1 =
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int HAL_Init( void )
{
  printf("HAL_Init: Started!\n");

  // Initialise wiringpi
  wiringPiSetup ();
  pinMode(ssPin, OUTPUT);
  pinMode(dio0, INPUT);
  pinMode(RST, OUTPUT);

  wiringPiSPISetup(CHANNEL, 500000);

  if( HAL_SetupLoRa() != 0)
  {
    // ERROR
    printf("HAL_Init: Error in setting up Lora!\n");
    return 1;
  }

  return 0;
}

/**
* __Function__: HAL_Engine
*
* __Description__: HAL procedures to be called in the main loop
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, 1 =
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int HAL_Engine(void)
{
  // Execute HAL tasks
  // Checking for received RF Packets and put them into the FIFO buffer, the GW layer to process the received messages
  HAL_Process_RX();

  /// TODO: check TX to node, does it need to be in lockstep with node availability?
  /// Keep track of Node RX windows ?
  // Check for TX message waiting in the TX Fifo and send them if required
  HAL_Process_TX();


  return 0;
}



/**
* __Function__: HAL_writeRegister
*
* __Description__: Writes a byte to the specificed register in the lora chip
*
* __Input__: byte addr = register address, byte vale = value to be written to the register
*
* __Output__: void
*
* __Status__: Completed
*
* __Remarks__:
*/
void HAL_writeRegister(byte addr, byte value)
{
    unsigned char spibuf[2];
    spibuf[0] = addr | 0x80;
    spibuf[1] = value;

    HAL_selectreceiver();
    wiringPiSPIDataRW(CHANNEL, spibuf, 2);
    HAL_unselectreceiver();
}

/**
* __Function__: HAL_readRegister
*
* __Description__: Reads a specific register from the Lora chip
*
* __Input__: byte = register address
*
* __Output__: contents of the register selected
*
* __Status__: Completed
*
* __Remarks__:
*/
byte HAL_readRegister(byte addr)
{
    unsigned char spibuf[2];

    HAL_selectreceiver();
    spibuf[0] = addr & 0x7F;
    spibuf[1] = 0x00;
    wiringPiSPIDataRW(CHANNEL, spibuf, 2);
    HAL_unselectreceiver();
    // Return the contents of the register
    return spibuf[1];
}

/**
* __Function__: HAL_selectreceiver
*
* __Description__: Pulls the Chip select pin low so that the SPI device (Lora Module) is selected
*
* __Input__: void
*
* __Output__: void
*
* __Status__: Completed
*
* __Remarks__:
*/
void HAL_selectreceiver()
{
    digitalWrite(ssPin, LOW);
}

/**
* __Function__: HAL_unselectreceiver
*
* __Description__: Puts the Chip select pin high so that the SPI device (Lora Module) is unsselected
*
* __Input__: void
*
* __Output__: void
*
* __Status__: Completed
*
* __Remarks__:
*/
void HAL_unselectreceiver()
{
    digitalWrite(ssPin, HIGH);
}

/**
* __Function__: HAL_SetupLoRa
*
* __Description__: Setup the Lora radio
*
* __Input__: void
*
* __Output__: Error code: 0 = no error, 1 =
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int HAL_SetupLoRa( void )
{
    digitalWrite(RST, HIGH);
    delay(100);
    digitalWrite(RST, LOW);
    delay(100);

    byte version = HAL_readRegister(REG_VERSION);

    if (version == 0x22) {
        // sx1272
        printf("HAL_SetupLoRa: SX1272 detected, starting.\n");
        sx1272 = true;
    } else {
        // sx1276?
        digitalWrite(RST, LOW);
        delay(100);
        digitalWrite(RST, HIGH);
        delay(100);
        version = HAL_readRegister(REG_VERSION);
        if (version == 0x12) {
            // sx1276
            printf("HAL_SetupLoRa: SX1276 detected, starting.\n");
            sx1272 = false;
        } else {
            printf("HAL_SetupLoRa: Unrecognized transceiver.\n");
            printf("HAL_SetupLoRa: Version: 0x%x\n",version);
            return 1;
        }
    }

    HAL_writeRegister(REG_OPMODE, SX72_MODE_SLEEP);

    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    HAL_writeRegister(REG_FRF_MSB, (uint8_t)(frf>>16) );
    HAL_writeRegister(REG_FRF_MID, (uint8_t)(frf>> 8) );
    HAL_writeRegister(REG_FRF_LSB, (uint8_t)(frf>> 0) );

    HAL_writeRegister(REG_SYNC_WORD, 0x34); // LoRaWAN public sync word

    if (sx1272) {
        if (sf == SF11 || sf == SF12) {
            HAL_writeRegister(REG_MODEM_CONFIG,0x0B);
        } else {
            HAL_writeRegister(REG_MODEM_CONFIG,0x0A);
        }
        HAL_writeRegister(REG_MODEM_CONFIG2,(sf<<4) | 0x04);
    } else {
        if (sf == SF11 || sf == SF12) {
            HAL_writeRegister(REG_MODEM_CONFIG3,0x0C);
        } else {
            HAL_writeRegister(REG_MODEM_CONFIG3,0x04);
        }
        HAL_writeRegister(REG_MODEM_CONFIG,0x72);
        HAL_writeRegister(REG_MODEM_CONFIG2,(sf<<4) | 0x04);
    }

    if (sf == SF10 || sf == SF11 || sf == SF12) {
        HAL_writeRegister(REG_SYMB_TIMEOUT_LSB,0x05);
    } else {
        HAL_writeRegister(REG_SYMB_TIMEOUT_LSB,0x08);
    }
    HAL_writeRegister(REG_MAX_PAYLOAD_LENGTH,0x80);
    HAL_writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);
    HAL_writeRegister(REG_HOP_PERIOD,0xFF);
    HAL_writeRegister(REG_FIFO_ADDR_PTR, HAL_readRegister(REG_FIFO_RX_BASE_AD));

    // Set Continous Receive Mode
    HAL_writeRegister(REG_LNA, LNA_MAX_GAIN);  // max lna gain
    HAL_writeRegister(REG_OPMODE, SX72_MODE_RX_CONTINUOS);

    // No errors
    return 0;
}

 /**
 * __Function__: HAL_SendFrame
 *
 * __Description__: Send a frame using the Lora radio
 *
 * __Input__: Pointer to the frame buffer, Buffer length
 *
 * __Output__: Error code: 0 = no error, 1 = TX Buffer full, 2 = FrameSize to big
 *
 * __Status__: Work in Progress
 *
 * __Remarks__:
 */
 // Change this to add to TX FIFO, send to be done by HAL_Process_TX
int HAL_SendFrame( uint8_t *TxFrame, byte FrameSize )
{
  /// Debug
  printf("HAL_SendFrame: Sending frame, frame Size: %d\n", FrameSize);
  // OS_PrintFrame( TxFrame, FrameSize);

  // clear TxDone IRQ
  HAL_writeRegister(REG_IRQ_FLAGS, 0x8);

  // Setup operation mode to standby to allow to send data
	HAL_writeRegister(REG_OPMODE, SX72_MODE_STANDBY);

  // TX Init
	HAL_writeRegister(REG_FIFO_TX_BASE_AD, 0);
	HAL_writeRegister(REG_FIFO_ADDR_PTR, 0);
	HAL_writeRegister(REG_PAYLOAD_LENGTH, FrameSize);   //now manually set to 12.....

  // Write data to FIFO
  for(int i = 0; i < FrameSize; i++)
  {
    HAL_writeRegister(REG_FIFO, TxFrame[i]);        /// double check size of FIFO buffer in SX
  }

  //Mode Request TX
  HAL_writeRegister(REG_OPMODE, SX72_MODE_TX);          /// need to check this, was expecting send to happen but does not seem the case, only when standby
  // Go to standby mode
  HAL_writeRegister(REG_OPMODE, SX72_MODE_STANDBY);     /// See above, not sure when TX only seems to happen when going to standby

  return 0;
}


/**
* __Function__: HAL_Process_TX
*
* __Description__: Send a frame using the Lora radio if one is in the FIFO
*
* __Input__: void
*
* __Output__: void
*
* __Status__: Work in Progress
*
* __Remarks__: Check if there is a frame to send in the FIFO , if there is, use
*/
int HAL_Process_TX()
{
  // Check LORA TX fifo
  if( LORA_TX_FIFO_Buffer[0].LORA_TX_FLAG != 0)
  {
    // Send the first message in the Fifo
    printf("HAL_Process_TX: There is something to send!\n");    /// Debug
    HAL_SendFrame( LORA_TX_FIFO_Buffer[0].LORA_TX_FRAME, LORA_TX_FIFO_Buffer[0].LORA_TX_FRAME_SIZE );

    printf("HAL_Process_TX: TX Frame processed\n");   /// Debug
    LORA_TX_FIFO_Buffer[0].LORA_TX_FLAG = 0;          // Set flag to 0 to indicate frame has been processed
    HAL_TX_FIFO_Update();                           // Shift frames fown the FIFO if applicable
    return 0;
  }
  else
  {
    // Nothing to process return
    return 0;
  }
}




/**
* __Function__: HAL_packagesend
*
* __Description__: Send a frame using the Lora radio
*
* __Input__: void
*
* __Output__: void
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
void HAL_packagesend()
{
	// Get IRQ flags
  int irqflags = HAL_readRegister(REG_IRQ_FLAGS);
	//Check of TXDone flag is set
	if(( irqflags & 0x8 ) == 0x8)
	{
    /// Debug, remove in final version
    printf("HAL_packagesend : TxDone flag is set, reset flag\n");
    // clear TxDone IRQ
    HAL_writeRegister(REG_IRQ_FLAGS, 0x8);

    // Change mode to standby, probably not required by going in listening mode
    HAL_writeRegister(REG_OPMODE, SX72_MODE_STANDBY);
	}
}

/**
* __Function__: HAL_Process_RX
*
* __Description__: Check for packets receieved from the RF radio
*
* __Input__: void
*
* __Output__: void
*
* __Status__: Work in Progress
*
* __Remarks__: If packet received put in FIFO, Application layer to process received LORA packages
*/
int HAL_Process_RX()
{
    char Lora_RX_Message[LORA_RX_MX_FRAME_SIZE];

    // Check DIO0 if there is a package received, if so process if not move on
    if(digitalRead(dio0) == 1)
    {
        // Message received, process
        if(HAL_ReceivePkt(Lora_RX_Message))
        {
            byte value = HAL_readRegister(REG_PKT_SNR_VALUE);     /// Check on what the SNR value is
            if( value & 0x80 ) // The SNR sign bit is 1
            {
                // Invert and divide by 4
                value = ( ( ~value + 1 ) & 0xFF ) >> 2;
                SNR = -value;
            }
            else
            {
                // Divide by 4
                SNR = ( value & 0xFF ) >> 2;
            }

            if (sx1272) {
                rssicorr = 139;
            } else {
                rssicorr = 157;
            }

            ///Debug, remove when done
            printf("HAL_Process_RX: Packet RSSI: %d, \n",HAL_readRegister(0x1A)-rssicorr);
            printf("HAL_Process_RX: RSSI: %d, \n",HAL_readRegister(0x1B)-rssicorr);
            printf("HAL_Process_RX: SNR: %li, \n",SNR);
            printf("HAL_Process_RX: Length: %i \n",(int)receivedbytes);

            // message contains package, length in receivedbytes
            // Add to LORA FIFO buffer
            memcpy(LORA_RX_FIFO_Buffer[LORA_RX_FIFO_Idx].LORA_RX_FRAME, Lora_RX_Message, receivedbytes);
            // set send flag
            LORA_RX_FIFO_Buffer[LORA_RX_FIFO_Idx].LORA_RX_FLAG = 1;                                       // Set flag to one to indicate there is a frame to be send
            LORA_RX_FIFO_Buffer[LORA_RX_FIFO_Idx].LORA_RX_FRAME_SIZE = receivedbytes;                     // Add frame size
            LORA_RX_FIFO_Buffer[LORA_RX_FIFO_Idx].LORA_RX_PACKET_RSSI = HAL_readRegister(0x1A)-rssicorr;  // Store Packet RSSI
            LORA_RX_FIFO_Buffer[LORA_RX_FIFO_Idx].LORA_RX_RSSI = HAL_readRegister(0x1B)-rssicorr;         // Store RSSI
            LORA_RX_FIFO_Buffer[LORA_RX_FIFO_Idx].LORA_RX_SNR = SNR;
            printf("HAL_Process_RX: Lora Frame received and added to buffer at position: %d\n", LORA_RX_FIFO_Idx );
            //Increase the fifo index
            LORA_RX_FIFO_Idx++;
        }
    } // dio0=1
    return 0;
}


/**
* __Function__: HAL_ReceivePkt
*
* __Description__: Package has been received, copy to payload buffer
*
* __Input__: Pointer to payload buffer
*
* __Output__: bool, False = error, True = no error (change this to int)
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
bool HAL_ReceivePkt(char *payload)
{

    // clear rxDone
    HAL_writeRegister(REG_IRQ_FLAGS, 0x40);

    int irqflags = HAL_readRegister(REG_IRQ_FLAGS);

    cp_nb_rx_rcv++;

    //  payload crc: 0x20
    if((irqflags & 0x20) == 0x20)
    {
        printf("CRC error\n");
        HAL_writeRegister(REG_IRQ_FLAGS, 0x20);
        return false;
    } else {

        cp_nb_rx_ok++;

        byte currentAddr = HAL_readRegister(REG_FIFO_RX_CURRENT_ADDR);
        byte receivedCount = HAL_readRegister(REG_RX_NB_BYTES);
        receivedbytes = receivedCount;

        HAL_writeRegister(REG_FIFO_ADDR_PTR, currentAddr);

        for(int i = 0; i < receivedCount; i++)
        {
            payload[i] = (char)HAL_readRegister(REG_FIFO);
        }
    }
    return true;
}


/**
* __Function__: HAL_GetSF
*
* __Description__: In case outer layers, such as UDP need to understand the spreading factor the RF is using
*
* __Input__: int -1 = Error else SF
*
* __Output__: uint32_t Frequency
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int HAL_GetSF( void )
{
  int SpreadingFactor;
  switch (sf) {
    case SF7:
      SpreadingFactor = 7;
    break;
    case SF8:
      SpreadingFactor = 8;
    break;
    case SF9:
      SpreadingFactor = 9;
    break;
    case SF10:
      SpreadingFactor = 10;
    break;
    case SF11:
      SpreadingFactor = 11;
    break;
    case SF12:
      SpreadingFactor = 12;
    break;
    default:
      printf("HAL_GetSF: Error: no such spreading factor!\n");
      SpreadingFactor = -1;
  }
  return SpreadingFactor;
}


/**
* __Function__: HAL_GetFreq
*
* __Description__: In case outer layers, such as UDP need to understand the frequency the RF is listening to
*
* __Input__: void
*
* __Output__: uint32_t Frequency
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
uint32_t HAL_GetFreq( void )
{
  return freq;
}






/**
* __Function__: HAL_ReceiveFrame
*
* __Description__: Function to be called from application layer to get received package out of FIFO if available
*
* __Input__: Pointer to buffer where frame can be stored
*
* __Output__: Error code: -1 = No Package available else Number of bytes received
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int HAL_ReceiveFrame(uint8_t *RxFrame)
{
  // Check for message in FIFO, if not available return -1
  if( LORA_RX_FIFO_Buffer[0].LORA_RX_FLAG != 0)
  {
    // Copy the frame from the FIFO in the application buffer
    memcpy( RxFrame, LORA_RX_FIFO_Buffer[0].LORA_RX_FRAME, LORA_RX_FIFO_Buffer[0].LORA_RX_FRAME_SIZE);
    LORA_RX_FIFO_Buffer[0].LORA_RX_FLAG = 0;                    // Set flag to 0 to indicate frame has been processed
    HAL_RX_FIFO_Update();                                      // Move received frames down the UDP RX FIFO
    /// Not returning the other information stores such as RSSI, might need this in the future
    printf("HAL_ReceiveFrame: RX Frame processed\n");           /// Debug
    return LORA_RX_FIFO_Buffer[0].LORA_RX_FRAME_SIZE;           // Return number of bytes received
  }
  else
  {
    // Nothing in the LORA RX FIFO
    return -1;
  }
}


/**
* __Function__: HAL_TransmitFrame
*
* __Description__: Function to be called by application layer to send frames using Lora
*
* __Input__: void
*
* __Output__: uint32_t Frequency
*
* __Status__: Work in Progress
*
* __Remarks__:
*/
int HAL_TransmitFrame(uint8_t *TxFrame,int FrameSize)
{
  /// __Incode Comments:__
  // check for space in HAL TX FIFO
  // Buffer Index runs from 0 to HAL_FIFO_DEPTH - 1
  if(LORA_TX_FIFO_Idx < (LORA_TX_FIFO_DEPTH))
  {
    if(FrameSize <= LORA_TX_MX_FRAME_SIZE)
    {
      // Copy frame in buffer
      memcpy(LORA_TX_FIFO_Buffer[LORA_TX_FIFO_Idx].LORA_TX_FRAME, TxFrame, FrameSize);
      // set send flag
      LORA_TX_FIFO_Buffer[LORA_TX_FIFO_Idx].LORA_TX_FLAG = 1;                 // Set flag to one to indicate there is a frame to be send
      LORA_TX_FIFO_Buffer[LORA_TX_FIFO_Idx].LORA_TX_FRAME_SIZE = FrameSize;   // Add frame size
      //printf("LW_AddFrameToTXBuffer: Frame added to buffer at position: %d\n", HAL_TX_FIFO_Idx );
      //Increase the fifo index
      LORA_TX_FIFO_Idx++;
      // No error, return
      /// The sending of the frame from the HAL TX Fifo is handled in HAL_Engine (HAL_Process_TX)
      return 0;
    }
    else
    {
      printf("HAL_TransmitFrame: FrameSize to big, frame cannot be send!\n");
      return 2;   /// Error 2: FrameSize to big
    }

  }
  else
  {
    // Buffer full
    printf("HAL_TransmitFrame: Buffer full, frame cannot be send!\n");
    return 1;       /// Error 1: TX Buffer full
  }
}

/**
 * __Function__: LORA_RX_FIFO_Update
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
void HAL_RX_FIFO_Update( void )
{
  int i;

  // printf("LW_FIFO_Update: index : %d \n", LW_TX_FIFO_Idx);

  if(LORA_RX_FIFO_Idx)
  {
    // update FIFO if Required
    for(i=0; i < (LORA_RX_FIFO_DEPTH - 1); ++i)
    {
      // Move frames down
      LORA_RX_FIFO_Buffer[i] = LORA_RX_FIFO_Buffer[i+1];
      // printf("LW_FIFO_Update: Move queue position : %d to : %d \n", i+1, i);
    }
    //Decrease the fifo index
    LORA_RX_FIFO_Idx--;
    // Make sure the flag is set to 0 to indicate there is space in the buffer
    LORA_RX_FIFO_Buffer[LORA_RX_FIFO_DEPTH - 1].LORA_RX_FLAG = 0;
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
 * __Function__: HAL_TX_FIFO_Update
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
 void HAL_TX_FIFO_Update( void )
{
  int i;

  // printf("LW_FIFO_Update: index : %d \n", LW_TX_FIFO_Idx);

  if(LORA_TX_FIFO_Idx)
  {
    // update FIFO if Required
    for(i=0; i < (LORA_TX_FIFO_DEPTH - 1); ++i)
    {
      // Move frames down
      LORA_TX_FIFO_Buffer[i] = LORA_TX_FIFO_Buffer[i+1];
      // printf("LW_FIFO_Update: Move queue position : %d to : %d \n", i+1, i);
    }
    //Decrease the fifo index
    LORA_TX_FIFO_Idx--;
    // Make sure the flag is set to 0 to indicate there is space in the buffer
    LORA_TX_FIFO_Buffer[LORA_TX_FIFO_DEPTH - 1].LORA_TX_FLAG = 0;
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
 * __Function__: HAL_GetSNR
 *
 * __Description__: Get the signa to noice ratio
 *
 * __Input__: Void
 *
 * __Output__: void
 *
 * __Status__: Completed
 *
 * __Remarks__: none
 */
long int HAL_GetSNR(void)
{
  return SNR;
}

/**
 * __Function__: HAL_GetRssiCor
 *
 * __Description__: Get the RSSI Correction based upon used chip
 *
 * __Input__: Void
 *
 * __Output__: void
 *
 * __Status__: Completed
 *
 * __Remarks__: none
 */
int HAL_GetRssiCor(void)
{
  return rssicorr;
}


uint32_t HAL_GetNumRX(void)
{
  return cp_nb_rx_rcv;
}


uint32_t HAL_GetRxOk(void)
{
  return cp_nb_rx_ok;
}

uint32_t HAL_GetRxBad(void)
{
  return cp_nb_rx_bad;
}

uint32_t HAL_GetRxNoCRC(void)
{
  return cp_nb_rx_nocrc;
}

uint32_t HAL_GetPktWfd(void)
{
  return cp_up_pkt_fwd;
}
