#include "Rx_HHI.h"
#include "Common_HHI.h"

#include "sx1272Regs-LoRa.h"
#include "sx1272Regs-FSK.h"


void HW_Rx( const HWStatus_t hwabs )
{
    bool rxContinuous = false;

	switch(hwabs.settings.Modem)
	{
        default:
        case MODEM_LORA:
        {
            if( hwabs.settings.LoRa.IqInverted == true )
            {
                HW_SpiWrite( REG_LR_INVERTIQ, ( ( HW_SpiRead( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
                HW_SpiWrite( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                HW_SpiWrite( REG_LR_INVERTIQ, ( ( HW_SpiRead( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                HW_SpiWrite( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            rxContinuous = hwabs.settings.LoRa.RxContinuous;

            if( hwabs.settings.LoRa.FreqHopOn == true )
            {
                HW_SpiWrite( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone, DIO2=FhssChangeChannel
                HW_SpiWrite( REG_DIOMAPPING1, ( HW_SpiRead( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                HW_SpiWrite( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone
                HW_SpiWrite( REG_DIOMAPPING1, ( HW_SpiRead( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            }
            HW_SpiWrite( REG_LR_FIFORXBASEADDR, 0 );
            HW_SpiWrite( REG_LR_FIFOADDRPTR, 0 );
        }
        break;
    }

    if (hwabs.settings.Modem == MODEM_LORA)
	{
        if( rxContinuous == true )
        {
            HW_SetOpMode( RFLR_OPMODE_RECEIVER );
        }
        else
        {
            HW_SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
    }
}

void HW_SetRxConfig (const HWStatus_t hwabs)
{
    HW_SetChannel(hwabs.settings.Channel);
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
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                           ( hwabs.settings.LoRa.Datarate << 4 ) |
                           ( ( hwabs.settings.LoRa.rxSymbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

            HW_SpiWrite( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( hwabs.settings.LoRa.rxSymbTimeout & 0xFF ) );

            HW_SpiWrite( REG_LR_PREAMBLEMSB, ( uint8_t )( ( hwabs.settings.LoRa.PreambleLen >> 8 ) & 0xFF ) );
            HW_SpiWrite( REG_LR_PREAMBLELSB, ( uint8_t )( hwabs.settings.LoRa.PreambleLen & 0xFF ) );

            if( hwabs.settings.LoRa.FixLen == 1 )
            {
                HW_SpiWrite( REG_LR_PAYLOADLENGTH, hwabs.settings.LoRa.PayloadLen );
            }

            if( hwabs.settings.LoRa.FreqHopOn == true )
            {
                HW_SpiWrite( REG_LR_PLLHOP, ( HW_SpiRead( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                HW_SpiWrite( REG_LR_HOPPERIOD, hwabs.settings.LoRa.HopPeriod );
            }

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


void HW_clearFskRxIrq(bool RxContinuous)
{
	HW_SpiWrite( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
                                        RF_IRQFLAGS1_PREAMBLEDETECT |
                                        RF_IRQFLAGS1_SYNCADDRESSMATCH );
    HW_SpiWrite( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN );

    if( RxContinuous == true )
    {
        // Continuous mode restart Rx chain
        HW_SpiWrite( REG_RXCONFIG, HW_SpiRead( REG_RXCONFIG ) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK );
    }
}



void HW_clearRxTimeoutIrq(void)
{
	// Clear Irq
	HW_SpiWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT );
}



void HW_clearRxIrq(void)
{
	HW_SpiWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );
}


rxCrc_e HW_checkCRC(void)
{
	volatile uint8_t irqFlags = HW_SpiRead( REG_LR_IRQFLAGS );
	if( ( irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
	{
		return rxcrc_nok;
	}
	else
	{
		return rxcrc_ok;
	}
}


void HW_clearRxPayloadCrc(void)
{
	HW_SpiWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR );
}


int8_t HW_readSnr(void)
{
	int8_t snr;
	uint8_t snrValue = HW_SpiRead( REG_LR_PKTSNRVALUE );
	if( snrValue & 0x80 ) // The SNR sign bit is 1
	{
		// Invert and divide by 4
		snr = ( ( ~snrValue + 1 ) & 0xFF ) >> 2;
		snr = -snr;
	}
	else
	{
		// Divide by 4
		snr = ( snrValue & 0xFF ) >> 2;
	}

	return snr;
}

int16_t HW_readRssi(int8_t snr)
{
	int16_t rssi;
	uint8_t rssiValue = HW_SpiRead( REG_LR_PKTRSSIVALUE );
    if( snr < 0 )
    {
        rssi = RSSI_OFFSET + rssiValue + ( rssiValue >> 4 ) + snr;
    }
    else
    {
        rssi = RSSI_OFFSET + rssiValue + ( rssiValue >> 4 );
    }

	return rssi;
}

uint16_t HW_getRxPayload(uint8_t* buf)
{
	uint16_t size = HW_SpiRead( REG_LR_RXNBBYTES );
	HW_SpiReadFifo( buf, size );

	return size;
}
