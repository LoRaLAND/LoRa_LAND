#ifndef PHYHAL_INTERFACE_H
#define PHYHAL_INTERFACE_H

#include "mbed.h"
#include "PHYHAL_types.h"

#define RX_MAX_RXSYMBOLTIME			1024    //rx 최대 심볼타임, 심볼은 전송 및 검출하는 의미있는 시간




/* ------------- Common functionality primitives ------------ */
//공통 기능 요소
int HAL_cfg_init(void (*PHYTxDone)(),void (*PHYTxError)(),void (*PHYRxDone)(uint8_t*,uint16_t,int16_t,int8_t),void (*PHYRxTimeout)(),void (*PHYRxError)()); //HAL 구동을 위한 초기화 함수

bool HAL_cmd_isActive(void);    //동작여부 확인
uint8_t HAL_cmd_getRxStatus();  //rx 상태 가져오기
void HAL_cmd_Sleep();           //대기(sleep)


/* ------------- TX commands and configuration primitives ------------ */
//tx 명령 및 구성 요소
HALtype_txCfgErr HAL_cfg_SetRfTxPower( int8_t power );  //tx 파워 설정(전송 세기 결정)

int8_t HAL_cmd_transmit(uint8_t* buffer, uint8_t buffer_size);  //버퍼에 있는 내용 전송
void HAL_cmd_SetTxConfig( int8_t power, HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency );    //tx 상세 설정


/* ------------- RX commands and configuration primitives ------------ */
//rx 명령 및 구성 요소(rxTime이 0이면 continuous모드, 1~1023이면 rx single모드 / rxTime 값은 심볼의 수이며, 실제 시간은 (2^SF / BW)를 곱한 심볼의 수?)
//rxTime : 0 - RX CONTINUOUS mode, 1 ~ 1023 - RX SINGLE mode (value is the number of symbols. actual time will be the number of symbol multiplied by (2^SF / BW).
HALtype_rxCfgErr HAL_cmd_SetRxConfig( HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency, uint16_t rxTime);  //rx 상세 설정
int8_t HAL_cmd_receive(void);   //수신 함수?


#endif