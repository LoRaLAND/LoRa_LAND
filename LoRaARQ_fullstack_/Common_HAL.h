#ifndef COMMON_HAL_H
#define COMMON_HAL_H

#include "mbed.h"


#define RADIO_WAKEUP_TIME                           1 // [ms]   

/*!
 * Sync word for Private LoRa networks
 */
#define LORA_MAC_PRIVATE_SYNCWORD                   0x12        //프라이빗 로라 네트워크를 위한 Sync word

/*!
 * Sync word for Public LoRa networks
 */
#define LORA_MAC_PUBLIC_SYNCWORD                    0x34        //퍼블릭 로라 네트워크를 위한 Sync word


/*!
 * SX1272 definitions       //sx1272 = 로라 통신 모듈(초록색 그거)
 */
#define XTAL_FREQ                                   32000000        //XTAL(크리스탈,수동 발진자) 주파수    
#define FREQ_STEP                                   61.03515625     //주파수 스텝

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET                                 -139            //RSSI, 수신된 신호의 세기 (snr은 노이즈에 대한 신호 세기)



typedef enum RadioState   //radiostate 구조체(null, idle, rx실행, tx 실행, cad 총 5가지 상태으로 구성)
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
typedef enum ModemType          //modemtype 구조체(FSK인지 LOLA인지 모뎀 구분)
{
    MODEM_FSK = 0,
    MODEM_LORA
}RadioModems_t;


//HW abstraction -------------------------------------  = HAL(Hardware Abstraction Layer, 하드웨어 추상화 계층)

/*!
 * Radio LoRa modem parameters          //로라 모뎀 파라미터
 */
typedef struct                          //로라 세팅
{
    int8_t   Power;                     //파워
    uint32_t Bandwidth;                 //대역폭
    uint32_t Datarate;                  //데이터레이트
    bool     LowDatarateOptimize;       //낮은 데이터 속도 최적화 여부?
    uint8_t  Coderate;                  //코드 레이트
    uint16_t PreambleLen;               //프리엠블 길이
    bool     FixLen;                    //길이 고정 여부
    uint8_t  PayloadLen;                //페이로드 길이
    bool     CrcOn;                     //crc 여부
    bool     FreqHopOn;                 //주파수 호핑 여부
    uint8_t  HopPeriod;                 //호핑 주기
    bool     IqInverted;                //IQ 인버터 여부
    bool     RxContinuous;              //rx 지속 여부
    uint32_t TxTimeout;                 //tx 타임아웃 시간 
    bool     PublicNetwork;             //퍼블릭네트워크인지 (false면 private?)
	uint16_t rxSymbTimeout;             //rx 심볼 타임아웃 시간
}RadioLoRaSettings_t;

//로라 패킷 핸들러
typedef struct                          
{
    int8_t SnrValue;                    //snr 값
    int8_t RssiValue;                   //rssi 값
    uint8_t Size;                       //크기
}RadioLoRaPacketHandler_t;


//라디오 세팅 구조체
typedef struct                          
{
    RadioState               State;
    ModemType                Modem;
    uint32_t                 Channel;       //채널
    RadioLoRaSettings_t      LoRa;
    RadioLoRaPacketHandler_t LoRaPacketHandler;
} RadioSettings_t;

//하드웨어 상태 구조체
typedef struct {                        
    uint8_t boardConnected;             //보드 연결 여부
    RadioSettings_t settings;           
} HWStatus_t;

HWStatus_t getHWStatus_t(void);      //HWStatus를 반환하는 함수

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

