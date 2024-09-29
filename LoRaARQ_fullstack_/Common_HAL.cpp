#include "Common_API.h"
#include "Common_HHI.h"
#include "Tx_HHI.h"
#include "Rx_HHI.h"
#include "Rx_HAL.h"
#include "Tx_HAL.h"

#define DBGMSG_CHAL						0

#define RX_BUFFER_SIZE                  256



static HWStatus_t HWStatus;

Timeout txTimeoutTimer;
Timeout rxTimeoutTimer;
Timeout rxTimeoutSyncWord;

uint8_t *rxtxBuffer;


static int HAL_init_radio(void);


/* 1. API functions */

int HAL_cfg_init(void (*PHYTxDone)(),void (*PHYTxError)(),void (*PHYRxDone)(uint8_t*,uint16_t,int16_t,int8_t),void (*PHYRxTimeout)(),void (*PHYRxError)())
{
	if (HWStatus.settings.State != RF_NULL)
	{
		debug("[ERROR] initialization failed : state (%i) is invalid\n", HWStatus.settings.State);
		return -1; //
	}
    HAL_SetTxDone(PHYTxDone);
    HAL_SetTxTimeout(PHYTxError);
    HAL_SetRxError(PHYRxError);
    HAL_SetRxDone(PHYRxDone);
    HAL_SetRxTimeout(PHYRxTimeout);
    
	return HAL_init_radio();
}


bool HAL_cmd_isActive(void)
{
    if (HWStatus.settings.State == RF_IDLE)
    {
        return false;
    }
    
    return true;
}

void HAL_cmd_Sleep(){
    HAL_Sleep();
}

uint8_t HAL_cmd_getRxStatus(){
    uint8_t rxStat;
    rxStat=HW_getModemStatusRx();
    return rxStat;
}




/* 2. HAL functions  */


/* -------------- general purpose functions ----------------------- */
RadioState HAL_getState(void)
{
	return HWStatus.settings.State;
}

void HAL_Sleep( void )
{
    txTimeoutTimer.detach( );
    rxTimeoutTimer.detach( );

	HW_Sleep();
	HWStatus.settings.State = RF_IDLE;
}


void HAL_Standby( void )
{
    txTimeoutTimer.detach( );
    rxTimeoutTimer.detach( );

	HW_Standby();
    HWStatus.settings.State = RF_IDLE;
}

const HWStatus_t HAL_getHwAbst(void)
{
	return HWStatus;
}






/* ---------------- configuration functions ----------------*/

void HAL_setLoRaLowDROpt(bool input)
{
	debug_if(DBGMSG_CHAL, "[HAL] config LoRa.LowDatarateOptimze : %i\n", input);
	HWStatus.settings.LoRa.LowDatarateOptimize = input;
}

void HAL_setLoRaDatarate(uint32_t input)
{
	debug_if(DBGMSG_CHAL, "[HAL] config LoRa.Datarate : %i\n", input);
	HWStatus.settings.LoRa.Datarate = input;
}


void HAL_setLoRaBandwidth(uint32_t input)
{
	debug_if(DBGMSG_CHAL, "[HAL] config LoRa.Bandwidth : %i\n", input);
	HWStatus.settings.LoRa.Bandwidth = input;
}


void HAL_setLoRaPower(uint32_t input)
{
	debug_if(DBGMSG_CHAL, "[HAL] config LoRa.Power : %i\n", input);
	HWStatus.settings.LoRa.Power = input;
}


void HAL_setLoRaCoderate(int8_t input)
{
	debug_if(DBGMSG_CHAL, "[HAL] config LoRa.coderate : %i\n", input);
	HWStatus.settings.LoRa.Coderate = input;
}

void HAL_setLoRaPreambleLen(uint16_t input)
{
	debug_if(DBGMSG_CHAL, "[HAL] config LoRa.preambleLen : %i\n", input);
	HWStatus.settings.LoRa.PreambleLen = input;
}

void HAL_setLoRaPacketHandlerSize(uint8_t size)
{
	debug_if(DBGMSG_CHAL, "[HAL] config LoRa.LoRaPacketHandler.Size : %i\n", size);
	HWStatus.settings.LoRaPacketHandler.Size = size;
}

void HAL_setLoRaFrequencyChannel(uint32_t frequency)
{
    debug_if(DBGMSG_CHAL, "[HAL] config LoRa.Channel : %i\n", frequency);
    HWStatus.settings.Channel=frequency;
}

void HAL_setLoRaRxContinuous(bool option)
{
    debug_if(DBGMSG_CHAL, "[HAL] config LoRa.RxContinuous : %i\n", option);
    HWStatus.settings.LoRa.RxContinuous=option;
}

void HAL_setLoRaRxTime(uint16_t time)
{
	debug_if(DBGMSG_CHAL, "[HAL] config rxSymbTimeout : %i\n", time);
	HWStatus.settings.LoRa.rxSymbTimeout=time;
}




void HAL_SetTxConfig(void)
{
		debug_if(DBGMSG_CHAL, "[HAL][HW EVENT] HW config for TX ::: lowdatarate optimize : %i, data rate : %i, bw : %i, power : %i, cr : %i, preamblelen : %i, size : %i, freq : %i, rxCont : %i, rxSymbTimeout : %i\n", 
													HWStatus.settings.LoRa.LowDatarateOptimize,
													HWStatus.settings.LoRa.Datarate,
													HWStatus.settings.LoRa.Bandwidth,
													HWStatus.settings.LoRa.Power,
													HWStatus.settings.LoRa.Coderate,
													HWStatus.settings.LoRa.PreambleLen,
													HWStatus.settings.LoRaPacketHandler.Size,
													HWStatus.settings.Channel,
													HWStatus.settings.LoRa.RxContinuous,
													HWStatus.settings.LoRa.rxSymbTimeout
													);
	HW_SetModem( MODEM_LORA );
	HW_SetTxConfig(HWStatus);
}


void HAL_SetRxConfig(void)
{
	debug_if(DBGMSG_CHAL, "[HAL][HW EVENT] HW config for RX ::: lowdatarate optimize : %i, data rate : %i, bw : %i, power : %i, cr : %i, preamblelen : %i, size : %i, freq : %i, rxCont : %i, rxSymbTimeout : %i\n", 
													HWStatus.settings.LoRa.LowDatarateOptimize,
													HWStatus.settings.LoRa.Datarate,
													HWStatus.settings.LoRa.Bandwidth,
													HWStatus.settings.LoRa.Power,
													HWStatus.settings.LoRa.Coderate,
													HWStatus.settings.LoRa.PreambleLen,
													HWStatus.settings.LoRaPacketHandler.Size,
													HWStatus.settings.Channel,
													HWStatus.settings.LoRa.RxContinuous,
													HWStatus.settings.LoRa.rxSymbTimeout
													);

    HW_SetModem( MODEM_LORA);
	HW_SetRxConfig(HWStatus);
}


void HAL_clearBuffer()
{
	memset( rxtxBuffer, 0, ( size_t )RX_BUFFER_SIZE );
}





/* ---------------- Interrupt handler functions ----------------- */
void HAL_swTimeout( void )
{
    switch(HWStatus.settings.State)
	{
    case RF_RX_RUNNING:
		HAL_onRxTimeoutIrq();
        
        OnRxTimeout();
     
        break;
    case RF_TX_RUNNING:
        // Tx timeout shouldn't happen.
        // But it has been observed that when it happens it is a result of a corrupted SPI transfer
        // it depends on the platform design.
        // 
        // The workaround is to put the radio in a known state. Thus, we re-initialize it.

        // BEGIN WORKAROUND

		#if 0
        // Reset the radio
        HW_reset( );
        // Initialize radio default values
		HW_Sleep();
        HW_RadioRegistersInit( );
        HW_SetModem( MODEM_FSK );
		#endif
		HW_init_radio(&HWStatus);
        // Restore previous network type setting.
        HW_SetPublicNetwork( HWStatus.settings.LoRa.PublicNetwork );
        // END WORKAROUND

		//HW_Sleep();
        HWStatus.settings.State = RF_IDLE;
		
		
        OnTxTimeout();
        
        break;
    default:
        break;
    }
}




void HAL_hwTimeout(void)
{
	RadioState state = HWStatus.settings.State;

	if (state == RF_RX_RUNNING)
	{
		// Sync time out
		rxTimeoutTimer.detach( );
		HW_clearRxTimeoutIrq();
		
		//Radio.HandleExternalIrq_resetState();
		HWStatus.settings.State = RF_IDLE;
		//HW_Sleep();
		
		OnRxTimeout();
	}
}




void HAL_hwDone(void)
{
	RadioState state = HWStatus.settings.State;
	uint16_t size;
	int16_t rssi;
	int8_t snr;
	rxCrc_e crcres;

	if (state == RF_RX_RUNNING)
	{
        // Clear Irq
        HW_clearRxIrq();
		crcres = HW_checkCRC();

        if( crcres == rxcrc_nok )
        {
            // Clear Irq
            HW_clearRxPayloadCrc();
			
           if( HWStatus.settings.LoRa.RxContinuous == false )
           {
                HWStatus.settings.State = RF_IDLE;
           }
            rxTimeoutTimer.detach( );
			//HW_Sleep();
            
            OnRxError();
        }
		else
		{
			snr = HW_readSnr();
			rssi = HW_readRssi(snr);
			size = HW_getRxPayload(rxtxBuffer);
	        

	        if( HWStatus.settings.LoRa.RxContinuous == false )
            {
                HWStatus.settings.State = RF_IDLE;
            }
            rxTimeoutTimer.detach( );
			//HW_Sleep();
            
	        OnRxDone(rxtxBuffer, size, rssi, snr);
	        
		}
    }
	else if (state == RF_TX_RUNNING)
	{
	    txTimeoutTimer.detach();
        // TxDone interrupt
        // Clear Irq
        HW_clearTxIrq();
		
		HWStatus.settings.State = RF_IDLE;
		//HW_Sleep();
		
        OnTxDone();
	}
	else
	{
		debug("[ERROR] state is weird :%i\n", state);
	}
	
}




void HAL_startRx(uint32_t timeout)
{
	HAL_clearBuffer();
	
	HW_Rx( HWStatus ); // HHI code ->header not included
	HWStatus.settings.State = RF_RX_RUNNING;//HW_setState
	
	if( timeout != 0 )
	{
		rxTimeoutTimer.attach_us( HAL_swTimeout , timeout * 1e3 ); //x
	}
}



int8_t HAL_startTx( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;
	
    switch(HWStatus.settings.Modem)
	{
    case MODEM_FSK:
		#if 0
        {
            HWabs.settings.FskPacketHandler.NbBytes = 0;
            HWabs.settings.FskPacketHandler.Size = size;

			HW_cfgTxLen(HAL_getHwAbst(), size);

            if( ( size > 0 ) && ( size <= 64 ) )
            {
                HWStatus.settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
                memcpy( rxtxBuffer, buffer, size );
                HWStatus.settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            HW_SpiWriteFifo( buffer, HWStatus.settings.FskPacketHandler.ChunkSize );
            HWStatus.settings.FskPacketHandler.NbBytes += HWStatus.settings.FskPacketHandler.ChunkSize;
            txTimeout = HWStatus.settings.Fsk.TxTimeout;
        }
        break;
		#endif
		debug("modem state is invalid(FSK, which is not supported now)\n");
		return -1;

    case MODEM_LORA:
        {
           	HW_setLoRaIqInverted(HWStatus.settings.LoRa.IqInverted);			
            HAL_setLoRaPacketHandlerSize(size);
            HW_setTxPayload(buffer, size);

            txTimeout = HWStatus.settings.LoRa.TxTimeout;
        }
        break;
    }

    HW_Tx( HWStatus, txTimeout );

	HWStatus.settings.State = RF_TX_RUNNING;
    txTimeoutTimer.attach_us(HAL_swTimeout, txTimeout*1e3);


	return 0;
}


void HAL_setRfTxPower(int8_t power)
{
    
    
	HW_SetRfTxPower(HWStatus, power);
}


/* ---------------- 3. inner function ----------------*/
static int HAL_init_radio()
{
	debug( "\n	   Initializing Radio Interrupt handlers \n" );

	//HWStatus.isRadioActive = false;
	//HWStatus.boardConnected = UNKNOWN;

	rxtxBuffer = (uint8_t*)malloc(RX_BUFFER_SIZE*sizeof(uint8_t));
	if (rxtxBuffer == NULL)
	{
		debug("[ERROR] failed in initiating radio : memory alloc failed\n");
		return -2;
	}

	//initialize the HW abstraction by the initial values in API header
	HWStatus.settings.Channel = HAL_INIT_RF_FREQUENCY;
	HWStatus.settings.Modem = HAL_INIT_MODEM;


	if (HWStatus.settings.Modem == MODEM_LORA)
	{
	    HWStatus.settings.LoRa.FixLen = HAL_INIT_LORA_FIXED_LENGTH;
	    HWStatus.settings.LoRa.FreqHopOn = HAL_INIT_LORA_FREQHOP;
	    HWStatus.settings.LoRa.HopPeriod = HAL_INIT_LORA_HOPPERIOD;
	    HWStatus.settings.LoRa.CrcOn = HAL_INIT_LORA_CRCON;
	    HWStatus.settings.LoRa.IqInverted = HAL_INIT_LORA_IQINV;
	    HWStatus.settings.LoRa.TxTimeout = HAL_INIT_LORA_TXTIMEOUT;
		HWStatus.settings.LoRa.Power = HAL_INIT_LORA_TXPOWER;
		HWStatus.settings.LoRa.Bandwidth = HAL_INIT_LORA_BANDWIDTH;
		HWStatus.settings.LoRa.Datarate = HAL_INIT_LORA_DATARATE;
		HWStatus.settings.LoRa.Coderate = HAL_INIT_LORA_CODERATE;
		HWStatus.settings.LoRa.PreambleLen = HAL_INIT_LORA_PREAMBLELEN;
		HWStatus.settings.LoRa.rxSymbTimeout = HAL_INIT_LORA_RXSYMBTIMEOUT;
		HWStatus.settings.LoRa.PayloadLen = HAL_INIT_LORA_PAYLOADLEN;
		HWStatus.settings.LoRa.RxContinuous = HAL_INIT_LORA_RXCONTINUOUS;
		HWStatus.settings.LoRa.PublicNetwork = HAL_INIT_LORA_PUBLIC_NETWORK;
	}
	else
	{
		debug("[HAL][ERROR] failed to initialize : currently %i modem is disabled!\n", HAL_INIT_MODEM);
		return -1;
	}

	
	HWStatus.boardConnected = HW_init_radio(&HWStatus);
	 
	//TX initial config
	HAL_TxInitConfig (HWStatus);
	
	//RX initial config
	HAL_RxInitConfig (HWStatus);
		
    HWStatus.settings.State = RF_IDLE;
	
	
    return 0;
}

