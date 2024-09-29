#ifndef TX_HAL_H
#define TX_HAL_H

#include "Common_HAL.h"

extern void (*OnTxTimeout)();
extern void (*OnTxDone)();


void HAL_TxInitConfig (const HWStatus_t hwabs);
void HAL_SetTxDone( void (*functionPtr)() );
void HAL_SetTxTimeout( void (*functionPtr)() );
#endif
