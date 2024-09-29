#include "Tx_HHI.h"
#include "Common_HHI.h"
#include "sx1272Regs-LoRa.h"
#include "sx1272Regs-FSK.h"

/*
SX1272( events, D11, D12, D13, D10, A0, //spi
				D2, D3, D4, D5, D8, D9 ), //dio0~5
*/


void HW_SetRfTxPower( const HWStatus_t hwabs, int8_t power )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = HW_SpiRead( REG_PACONFIG );
    paDac = HW_SpiRead( REG_PADAC );


    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | HW_GetPaSelect( hwabs.boardConnected );

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
    {
        if( power > 17 )
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
        }
        else
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        }
        if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            paConfig = ( paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            paConfig = ( paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        paConfig = ( paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
	
    HW_SpiWrite( REG_PACONFIG, paConfig );
    HW_SpiWrite( REG_PADAC, paDac );
}


void HW_Tx( const HWStatus_t HWabs, uint32_t timeout )
{
    
    switch(HWabs.settings.Modem)
    {
    case MODEM_FSK:
		#if 0
        {
            // DIO0=PacketSent
            // DIO1=FifoEmpty
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            HW_SpiWrite( REG_DIOMAPPING1, ( HW_SpiRead( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO1_01 );

            HW_SpiWrite( REG_DIOMAPPING2, ( HW_SpiRead( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) );
            HWabs.settings.FskPacketHandler.FifoThresh = HW_SpiRead( REG_FIFOTHRESH ) & 0x3F;
        }
		#endif
        break;
    case MODEM_LORA:
        {
            if( HWabs.settings.LoRa.FreqHopOn == true )
            {
                HW_SpiWrite( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone, DIO2=FhssChangeChannel
                HW_SpiWrite( REG_DIOMAPPING1, ( HW_SpiRead( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK ) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                HW_SpiWrite( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone
                HW_SpiWrite( REG_DIOMAPPING1, ( HW_SpiRead( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
            }
        }
        break;
    }

	HW_SetOpMode( RF_OPMODE_TRANSMITTER );

}

void HW_SetTxConfig( const HWStatus_t hwabs )
{
    HW_SetRfTxPower( hwabs, hwabs.settings.LoRa.Power );
    HW_SetChannel(hwabs.settings.Channel);
            if( hwabs.settings.LoRa.FreqHopOn == true )
            {
                HW_SpiWrite( REG_LR_PLLHOP, ( HW_SpiRead( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                HW_SpiWrite( REG_LR_HOPPERIOD, hwabs.settings.LoRa.HopPeriod );
            }

            HW_SpiWrite( REG_LR_MODEMCONFIG1,
                         ( HW_SpiRead( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK &
                           RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK &
                           RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK ) |
                           ( hwabs.settings.LoRa.Bandwidth << 6 ) | ( hwabs.settings.LoRa.Coderate << 3 ) |
                           ( hwabs.settings.LoRa.FixLen << 2 ) | ( hwabs.settings.LoRa.CrcOn << 1 ) |
                           hwabs.settings.LoRa.LowDatarateOptimize );

            HW_SpiWrite( REG_LR_MODEMCONFIG2,
                        ( HW_SpiRead( REG_LR_MODEMCONFIG2 ) &
                          RFLR_MODEMCONFIG2_SF_MASK ) |
                          ( hwabs.settings.LoRa.Datarate << 4 ) );


            HW_SpiWrite( REG_LR_PREAMBLEMSB, ( hwabs.settings.LoRa.PreambleLen >> 8 ) & 0x00FF );
            HW_SpiWrite( REG_LR_PREAMBLELSB, hwabs.settings.LoRa.PreambleLen & 0xFF );

            if( hwabs.settings.LoRa.Datarate == 6 )
            {
                HW_SpiWrite( REG_LR_DETECTOPTIMIZE,
                             ( HW_SpiRead( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                HW_SpiWrite( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                HW_SpiWrite( REG_LR_DETECTOPTIMIZE,
                             ( HW_SpiRead( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                HW_SpiWrite( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }

}



uint8_t HW_GetPaSelect(uint8_t boardConnected )
{
    if( boardConnected == SX1272MB1DCS || boardConnected == MDOT_F411RE )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}




void HW_clearTxIrq(void)
{
	HW_SpiWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );
}




void HW_cfgTxLen(const HWStatus_t hwabs, uint8_t size)
{	
	#if 0
    if( hwabs.settings.Fsk.FixLen == false )
    {
        HW_SpiWriteFifo( ( uint8_t* )&size, 1 );
    }
    else
	
    {
        HW_SpiWrite( REG_PAYLOADLENGTH, size );
    }
	#endif
}



void HW_setLoRaIqInverted(bool IqInverted)
{		
	if( IqInverted == true )
	{

		HW_SpiWrite( REG_LR_INVERTIQ, ( ( HW_SpiRead( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
		HW_SpiWrite( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
	}
	else
	{
		HW_SpiWrite( REG_LR_INVERTIQ, ( ( HW_SpiRead( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
		HW_SpiWrite( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
	}
}



void HW_setTxPayload(uint8_t* buffer, uint8_t size)
{
    // Initializes the payload size
    HW_SpiWrite( REG_LR_PAYLOADLENGTH, size );

    // Full buffer used for Tx
    HW_SpiWrite( REG_LR_FIFOTXBASEADDR, 0 );
    HW_SpiWrite( REG_LR_FIFOADDRPTR, 0 );

    // FIFO operations can not take place in Sleep mode
    if( ( HW_SpiRead( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
    {
        HAL_Standby( );
        wait_ms( 1 );
    }
    // Write payload buffer
	HW_SpiWriteFifo( buffer, size );
}

