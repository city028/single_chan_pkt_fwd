/*******************************************************************************
 * hal Header file
 *******************************************************************************/

#ifndef _hal_hpp_
#define _hal_hpp_

typedef unsigned char byte;

/**
* HAL Functions and Procedures
*/
int HAL_Init( void );
int HAL_SendFrame( uint8_t *TxFrame, byte FrameSize );
int HAL_SetupLoRa( void );
byte HAL_readRegister(byte addr);
void HAL_writeRegister(byte addr, byte value);
void HAL_selectreceiver(void);
void HAL_unselectreceiver(void);
void HAL_packagesend(void);
int HAL_Engine(void);
int HAL_GetSF( void );
uint32_t HAL_GetFreq( void );
bool HAL_ReceivePkt(char *payload);
void HAL_ReceivePacket(void);

#define REG_FIFO                    0x00
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_OPMODE                  0x01
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_SYMB_TIMEOUT_LSB  		  0x1F
#define REG_PKT_SNR_VALUE			      0x19
#define REG_PAYLOAD_LENGTH          0x22
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_MAX_PAYLOAD_LENGTH 		  0x23
#define REG_HOP_PERIOD              0x24
#define REG_SYNC_WORD				        0x39
#define REG_VERSION	  				      0x42

#define SX72_MODE_RX_CONTINUOS      0x85
#define SX72_MODE_TX                0x83
#define SX72_MODE_SLEEP             0x80
#define SX72_MODE_STANDBY           0x81

#define PAYLOAD_LENGTH              0x40

// FRF
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08

#define FRF_MSB                     0xD9 // 868.1 Mhz
#define FRF_MID                     0x06
#define FRF_LSB                     0x66

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN		    	      0x20






/**
* Channel number constant
*/
static const int CHANNEL = 0;

/**
* Spreading factors
*/
enum sf_t { SF7=7, SF8, SF9, SF10, SF11, SF12 };





#endif // _hal_hpp_
