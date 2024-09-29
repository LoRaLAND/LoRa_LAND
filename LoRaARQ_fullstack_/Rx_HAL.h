#ifndef RX_HAL_H
#define RX_HAL_H

#include "Common_HAL.h"

extern void (*OnRxDone)(uint8_t*,uint16_t,int16_t,int8_t);// OnRxDone function Pointer
extern void (*OnRxError)();
extern void (*OnRxTimeout)();


void HAL_RxInitConfig (const HWStatus_t hwabs);
void HAL_onRxTimeoutIrq(void);

void HAL_SetRxDone( void (*functionPtr)(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) );
void HAL_SetRxTimeout( void (*functionPtr)() );
void HAL_SetRxError( void (*functionPtr)() );



#endif
