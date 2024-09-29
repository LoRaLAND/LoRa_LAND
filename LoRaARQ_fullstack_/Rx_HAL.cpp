#include "PHYHAL_interface.h"
#include "Rx_HHI.h"
#include "Common_HAL.h"

void (*OnRxDone)(uint8_t*,uint16_t,int16_t,int8_t);// OnRxDone function Pointer
void (*OnRxError)();
void (*OnRxTimeout)();


void HAL_SetRxDone( void (*functionPtr)(uint8_t*,uint16_t,int16_t,int8_t) )
{
    OnRxDone = functionPtr;
	//debug (">>>>> HAL received a message : %s (size:%i, rssi:%i, snr:%i)\n", payload, size, rssi, snr);
}

void HAL_SetRxTimeout( void (*functionPtr)() )
{
    OnRxTimeout =functionPtr;
	debug( ">>>>> HAL OnRxTimeout\n\r" );
}

void HAL_SetRxError( void (*functionPtr)() )
{
    OnRxError=functionPtr;
	debug( ">>>>> HAL OnRxError\n\r" );
}


int8_t HAL_cmd_receive(void)
{
	uint32_t timeout = 0;
	 
    if(HAL_getState() != RF_IDLE)
	{
		debug("Failed to accept the HAL cmd : RF is not idle (%i)\n", HAL_getState());
		return -1;
    }

	HAL_startRx(timeout);
	
	return 0;

}

HALtype_rxCfgErr HAL_cmd_SetRxConfig( HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen, uint32_t frequency, uint16_t rxTime)
{
    if (rxTime >= RX_MAX_RXSYMBOLTIME)
	{
		debug("[HAL] error on config parameter : time is too long! (%i, must be less than 1024. if you want longer one, then do continous mode (rxTime = 0) )\n", rxTime);
		return HAL_RX_RXTIME_CFG_ERR;
	}

	HAL_setLoRaBandwidth((uint32_t)bandwidth);	
	HAL_setLoRaCoderate((uint8_t)coderate);
	HAL_setLoRaPreambleLen(preambleLen);
    HAL_setLoRaFrequencyChannel(frequency);

	HAL_setLoRaDatarate(datarate);

    if( ( ( bandwidth == HAL_LRBW_125 ) && ( ( datarate == HAL_LRDatarate_SF11 ) || ( datarate == HAL_LRDatarate_SF12 ) ) ) ||
        ( ( bandwidth == HAL_LRBW_250 ) && ( datarate == HAL_LRDatarate_SF12) ) )
    {
        HAL_setLoRaLowDROpt(0x01);
    }
    else
    {
        HAL_setLoRaLowDROpt(0x00);
    }

	if (rxTime == 0) //set as continuous
	{
		HAL_setLoRaRxContinuous(true);
	}
	else
	{
		HAL_setLoRaRxContinuous(false);
		HAL_setLoRaRxTime(rxTime);
	}

	HAL_SetRxConfig();

	return HAL_RX_NO_ERR;
}




void HAL_onRxTimeoutIrq(void)
{
	
}

void HAL_RxInitConfig (const HWStatus_t hwabs)
{
    if( hwabs.settings.LoRa.Datarate > 12 )
    {
        debug("[HAL][WARNING] data rate %i is truncated", hwabs.settings.LoRa.Datarate);
        HAL_setLoRaDatarate(12);
    }
    else if( hwabs.settings.LoRa.Datarate < 6 )
    {
        debug("[HAL][WARNING] data rate %i is truncated", hwabs.settings.LoRa.Datarate);
        HAL_setLoRaDatarate(6);
    }

    if( ( ( hwabs.settings.LoRa.Bandwidth == 0 ) && ( ( hwabs.settings.LoRa.Datarate == 11 ) || ( hwabs.settings.LoRa.Datarate == 12) ) ) ||
        ( ( hwabs.settings.LoRa.Bandwidth == 1 ) && ( hwabs.settings.LoRa.Datarate == 12 ) ) )
    {
        HAL_setLoRaLowDROpt(0x01);
    }
    else
    {
        HAL_setLoRaLowDROpt(0x00);
    }
    HAL_SetRxConfig();
    
}