#ifndef PHYHAL_INTERFACE_H
#define PHYHAL_INTERFACE_H

#include "mbed.h"
#include "PHYHAL_types.h"

#define RX_MAX_RXSYMBOLTIME			1024




/* ------------- Common functionality primitives ------------ */
int HAL_cfg_init(void (*PHYTxDone)(),void (*PHYTxError)(),void (*PHYRxDone)(uint8_t*,uint16_t,int16_t,int8_t),void (*PHYRxTimeout)(),void (*PHYRxError)());

bool HAL_cmd_isActive(void);
uint8_t HAL_cmd_getRxStatus();
void HAL_cmd_Sleep();


/* ------------- TX commands and configuration primitives ------------ */
HALtype_txCfgErr HAL_cfg_SetRfTxPower( int8_t power );

int8_t HAL_cmd_transmit(uint8_t* buffer, uint8_t buffer_size);
void HAL_cmd_SetTxConfig( int8_t power, HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency );


/* ------------- RX commands and configuration primitives ------------ */
//rxTime : 0 - RX CONTINUOUS mode, 1 ~ 1023 - RX SINGLE mode (value is the number of symbols. actual time will be the number of symbol multiplied by (2^SF / BW).
HALtype_rxCfgErr HAL_cmd_SetRxConfig( HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency, uint16_t rxTime);
int8_t HAL_cmd_receive(void);


#endif