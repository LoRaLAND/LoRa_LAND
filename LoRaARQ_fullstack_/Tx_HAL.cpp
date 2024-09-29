//#include "Tx_API.h"
#include "PHYHAL_interface.h"
#include "Tx_HAL.h"
#include "Tx_HHI.h"
#include "Common_HAL.h"
/*
SX1272( events, D11, D12, D13, D10, A0, //spi
				D2, D3, D4, D5, D8, D9 ), //dio0~5
*/

void (*OnTxTimeout)() = NULL;
void (*OnTxDone)() = NULL;



int8_t HAL_cmd_transmit(uint8_t* buffer, uint8_t buffer_size)
{
    if(buffer == NULL)
    {
        debug("Failed to accept HAL cmd: packet is empty!\n");
        return -1;
    }
    
    if(buffer_size > 64)
    {
        debug("Failed to accept HAL cmd: packet size is too large!\n");
        return -2;
    }
	HAL_Sleep();
	wait_ms( 10 );
	HAL_startTx( buffer, buffer_size );
    return 0;
}

void HAL_cmd_SetTxConfig( int8_t power, HALenum_bw_e bandwidth, HALenum_dr_e datarate, HALenum_cr_e coderate, uint16_t preambleLen,uint32_t frequency )
{
	HAL_setLoRaPower(power);
	HAL_setLoRaBandwidth((uint32_t)bandwidth);
	HAL_setLoRaCoderate((uint8_t)coderate);
	HAL_setLoRaPreambleLen(preambleLen);
    HAL_setLoRaFrequencyChannel(frequency);

	HAL_setLoRaDatarate(datarate);

    if( ( ( bandwidth == HAL_LRBW_125) && ( ( datarate == HAL_LRDatarate_SF11 ) || ( datarate == HAL_LRDatarate_SF12 ) ) ) ||
        ( ( bandwidth == HAL_LRBW_250 ) && ( datarate == HAL_LRDatarate_SF12 ) ) )
    {
        HAL_setLoRaLowDROpt(0x01);
    }
    else
    {
        HAL_setLoRaLowDROpt(0x00);
    }
    HAL_SetTxConfig();
    
}


HALtype_txCfgErr HAL_cfg_SetRfTxPower( int8_t power )
{
    //validity check code (TBD)
    
	HAL_setLoRaPower(power);
    HAL_setRfTxPower(power);

    return HAL_TX_NO_ERR;
}


void HAL_SetTxDone( void (*functionPtr)() )
{
    OnTxDone = functionPtr;
	debug( ">>>>> HAL OnTxDone Function\n\r" );
}

void HAL_SetTxTimeout( void (*functionPtr)() )
{
    OnTxTimeout = functionPtr;
    debug( ">>>>> HAL OnTxTimeout\n\r" );
}







void HAL_TxInitConfig (const HWStatus_t hwabs)
{

	HAL_setRfTxPower(hwabs.settings.LoRa.Power);

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
    HAL_SetTxConfig();   
}