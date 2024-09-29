#ifndef RX_HHI_H
#define RX_HHI_H
#include "Common_HHI.h"



typedef enum {
	rxcrc_unknown = -1,
	rxcrc_ok = 0,
	rxcrc_nok = 1

} rxCrc_e;

void HW_Rx( const HWStatus_t );
void HW_SetRxConfig( const HWStatus_t hwabs );
void HW_clearRxTimeoutIrq(void);
void HW_clearFskRxIrq(bool RxContinuous);
void HW_clearRxIrq(void);
rxCrc_e HW_checkCRC(void);
void HW_clearRxPayloadCrc(void);
int8_t HW_readSnr(void);
int16_t HW_readRssi(int8_t snr);
uint16_t HW_getRxPayload(uint8_t* buf);


#endif

