#ifndef COMMON_HAL_H
#define COMMON_HAL_H

#include "mbed.h"


#define RADIO_WAKEUP_TIME                           1 // [ms]

/*!
 * Sync word for Private LoRa networks
 */
#define LORA_MAC_PRIVATE_SYNCWORD                   0x12

/*!
 * Sync word for Public LoRa networks
 */
#define LORA_MAC_PUBLIC_SYNCWORD                    0x34


/*!
 * SX1272 definitions
 */
#define XTAL_FREQ                                   32000000
#define FREQ_STEP                                   61.03515625

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET                                 -139



typedef enum RadioState
{
	RF_NULL = 0,
    RF_IDLE,
    RF_RX_RUNNING,
    RF_TX_RUNNING,
    RF_CAD,
}RadioState_t;

/*!
 *    Type of the modem. [LORA / FSK]
 */
typedef enum ModemType
{
    MODEM_FSK = 0,
    MODEM_LORA
}RadioModems_t;


//HW abstraction -------------------------------------

/*!
 * Radio LoRa modem parameters
 */
typedef struct
{
    int8_t   Power;
    uint32_t Bandwidth;
    uint32_t Datarate;
    bool     LowDatarateOptimize;
    uint8_t  Coderate;
    uint16_t PreambleLen;
    bool     FixLen;
    uint8_t  PayloadLen;
    bool     CrcOn;
    bool     FreqHopOn;
    uint8_t  HopPeriod;
    bool     IqInverted;
    bool     RxContinuous;
    uint32_t TxTimeout;
    bool     PublicNetwork;
	uint16_t rxSymbTimeout;
}RadioLoRaSettings_t;


typedef struct
{
    int8_t SnrValue;
    int8_t RssiValue;
    uint8_t Size;
}RadioLoRaPacketHandler_t;



typedef struct
{
    RadioState               State;
    ModemType                Modem;
    uint32_t                 Channel;
    RadioLoRaSettings_t      LoRa;
    RadioLoRaPacketHandler_t LoRaPacketHandler;
} RadioSettings_t;


typedef struct {
    uint8_t boardConnected;
    RadioSettings_t settings;
} HWStatus_t;



RadioState HAL_getState(void);
void HAL_Sleep( void );
void HAL_Standby( void );
const HWStatus_t HAL_getHwAbst(void);



//abstract configuration functions
void HAL_setLoRaLowDROpt(bool input);
void HAL_setLoRaDatarate(uint32_t input);
void HAL_setLoRaPower(uint32_t input);
void HAL_setLoRaBandwidth(uint32_t input);
void HAL_setLoRaCoderate(int8_t input);
void HAL_setLoRaPreambleLen(uint16_t input);
void HAL_setLoRaPacketHandlerSize(uint8_t size);
void HAL_setLoRaFrequencyChannel(uint32_t frequency);
void HAL_setLoRaRxContinuous(bool option);
void HAL_setLoRaRxTime(uint16_t time);

void HAL_clearBuffer();


//HW config command from HAL to HAL
void HAL_SetTxConfig(void);
void HAL_SetRxConfig(void);



//interrupt handlers
void HAL_swTimeout( void );
void HAL_hwTimeout(void);
void HAL_hwDone(void);
void HAL_startRx(uint32_t timeout);
int8_t HAL_startTx( uint8_t *buffer, uint8_t size );
void HAL_setRfTxPower(int8_t power);


#endif

