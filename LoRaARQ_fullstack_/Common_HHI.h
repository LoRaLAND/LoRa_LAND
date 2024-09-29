#ifndef COMMON_HHI_H
#define COMMON_HHI_H

#include "Common_HAL.h"

typedef enum BoardType
{
    SX1272MB2XAS = 0,
    SX1272MB1DCS,
    NA_MOTE_72,
    MDOT_F411RE,
    UNKNOWN
}BoardType_t;



uint8_t HW_init_radio(const HWStatus_t* hwabs);
void HW_Standby(void);
void HW_Sleep(void);
void HW_SetModem( RadioModems_t modem );
void HW_SetPublicNetwork( bool enable );
void HW_SpiWrite( uint8_t addr, uint8_t data );
uint8_t HW_SpiRead( uint8_t addr );
void HW_SpiReadFifo( uint8_t *buffer, uint8_t size );
void HW_SpiWriteFifo( uint8_t *buffer, uint8_t size );

void HW_SetOpMode( uint8_t opMode );
void HW_SetChannel( uint32_t freq );
uint8_t HW_getModemStatusRx();

#endif
