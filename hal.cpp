/*******************************************************************************
 * hardware Abstraction Layer (HAL)
 *
 * Used base source code from other authors, see credits below but I have
 * modified it to suit my own needs
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
//#include "os.h"               // Used for print frame
#include "hal.h"             // The header file for this



/**
*
* User defined variables below!
*
* uint32_t  freq = 868100000;   /// in Mhz! (868.1) = 868Mhz channel 1
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

  wiringPiSetup ();
  pinMode(ssPin, OUTPUT);
  pinMode(dio0, INPUT);
  pinMode(RST, OUTPUT);

  wiringPiSPISetup(CHANNEL, 500000);

  if( HAL_SetupLoRa() != 0)
  {
    // ERROR
    return 1;
  }

  return 0;
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
  		printf("HAL_packagesend : TxDone flag is set, reset flag\n");
                // clear TxDone IRQ
                HAL_writeRegister(REG_IRQ_FLAGS, 0x8);

                // Change mode to standby, probably not required by going in listening mode
                HAL_writeRegister(REG_OPMODE, SX72_MODE_STANDBY);
	}

}


void HAL_ReceivePacket() {

    long int SNR;
    int rssicorr;

    if(digitalRead(dio0) == 1)
    {
        if(HAL_ReceivePkt(message)) {
            byte value = HAL_readRegister(REG_PKT_SNR_VALUE);
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

            printf("Packet RSSI: %d, ",HAL_readRegister(0x1A)-rssicorr);
            printf("RSSI: %d, ",HAL_readRegister(0x1B)-rssicorr);
            printf("SNR: %li, ",SNR);
            printf("Length: %i",(int)receivedbytes);
            printf("\n");

            int j;
            j = bin_to_b64((uint8_t *)message, receivedbytes, (char *)(b64), 341);
            //fwrite(b64, sizeof(char), j, stdout);


           //  Bytes  | Function
           // :------:|---------------------------------------------------------------------
           //  0      | protocol version = 2
           //  1-2    | random token
           //  3      | PUSH_DATA identifier 0x00
           //  4-11   | Gateway unique identifier (MAC address)
           //  12-end | JSON object, starting with {, ending with }, see section 4




            char buff_up[TX_BUFF_SIZE]; /* buffer to compose the upstream packet */
            int buff_index=0;

            /* gateway <-> MAC protocol variables */
            //static uint32_t net_mac_h; /* Most Significant Nibble, network order */
            //static uint32_t net_mac_l; /* Least Significant Nibble, network order */

            /* pre-fill the data buffer with fixed fields */
            buff_up[0] = PROTOCOL_VERSION;
            buff_up[3] = PKT_PUSH_DATA;

            /* process some of the configuration variables */
            //net_mac_h = htonl((uint32_t)(0xFFFFFFFF & (lgwm>>32)));
            //net_mac_l = htonl((uint32_t)(0xFFFFFFFF &  lgwm  ));
            //*(uint32_t *)(buff_up + 4) = net_mac_h;
            //*(uint32_t *)(buff_up + 8) = net_mac_l;

            buff_up[4] = (unsigned char)ifr.ifr_hwaddr.sa_data[0];
            buff_up[5] = (unsigned char)ifr.ifr_hwaddr.sa_data[1];
            buff_up[6] = (unsigned char)ifr.ifr_hwaddr.sa_data[2];
            buff_up[7] = 0xFF;
            buff_up[8] = 0xFF;
            buff_up[9] = (unsigned char)ifr.ifr_hwaddr.sa_data[3];
            buff_up[10] = (unsigned char)ifr.ifr_hwaddr.sa_data[4];
            buff_up[11] = (unsigned char)ifr.ifr_hwaddr.sa_data[5];

            /* start composing datagram with the header */
            uint8_t token_h = (uint8_t)rand(); /* random token */
            uint8_t token_l = (uint8_t)rand(); /* random token */
            buff_up[1] = token_h;
            buff_up[2] = token_l;
            buff_index = 12; /* 12-byte header */

            // TODO: tmst can jump is time is (re)set, not good.
            struct timeval now;
            gettimeofday(&now, NULL);
            uint32_t tmst = (uint32_t)(now.tv_sec*1000000 + now.tv_usec);

            /* start of JSON structure */
            memcpy((void *)(buff_up + buff_index), (void *)"{\"rxpk\":[", 9);
            buff_index += 9;
            buff_up[buff_index] = '{';
            ++buff_index;
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, "\"tmst\":%u", tmst);
            buff_index += j;
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"chan\":%1u,\"rfch\":%1u,\"freq\":%.6lf", 0, 0, (double)freq2/1000000);
            buff_index += j;
            memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":1", 9);
            buff_index += 9;
            memcpy((void *)(buff_up + buff_index), (void *)",\"modu\":\"LORA\"", 14);
            buff_index += 14;
            /* Lora datarate & bandwidth, 16-19 useful chars */
            switch (sf) {
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
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"lsnr\":%li", SNR);
            buff_index += j;
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"rssi\":%d,\"size\":%u", HAL_readRegister(0x1A)-rssicorr, receivedbytes);
            buff_index += j;
            memcpy((void *)(buff_up + buff_index), (void *)",\"data\":\"", 9);
            buff_index += 9;
            j = bin_to_b64((uint8_t *)message, receivedbytes, (char *)(buff_up + buff_index), 341);
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

            printf("rxpk update: %s\n", (char *)(buff_up + 12)); /* DEBUG: display JSON payload */

            //send the messages
            sendudp(buff_up, buff_index);

            fflush(stdout);

        } // received a message

    } // dio0=1
}



boolean HAL_ReceivePkt(char *payload)
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
