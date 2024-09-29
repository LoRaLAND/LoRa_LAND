#ifndef TX_HHI_H
#define TX_HHI_H

#include "Common_HAL.h"

void HW_SetRfTxPower( const HWStatus_t hwabs, int8_t power );
void HW_Tx( const HWStatus_t HWabs, uint32_t timeout );
void HW_SetTxConfig( const HWStatus_t hwabs );
void HW_clearTxIrq(void);
uint8_t HW_GetPaSelect(uint8_t boardConnected );
void HW_cfgTxLen(const HWStatus_t hwabs, uint8_t size);
void HW_setLoRaIqInverted(bool IqInverted);
void HW_setTxPayload(uint8_t* buffer, uint8_t size);



#endif
