#include "Common_API.h"
#include "Common_HHI.h"
#include "Tx_HHI.h"
#include "Rx_HHI.h"
#include "Rx_HAL.h"
#include "Tx_HAL.h"

#define DBGMSG_CHAL						0			//0이면 디버그 off, 1이면 디버그 on

#define RX_BUFFER_SIZE                  256



static HWStatus_t HWStatus;

Timeout txTimeoutTimer;						//Timeout 타입의 구조체는 어디서 볼 수 있지?
Timeout rxTimeoutTimer;
Timeout rxTimeoutSyncWord;

uint8_t *rxtxBuffer;


static int HAL_init_radio(void);

HWStatus_t getHWStatus_t(void){return HWStatus;}

/* 1. API functions */

//하드웨어 세팅 상태가 RF_NULL일때 상태를 초기화 하는 함수
int HAL_cfg_init(void (*PHYTxDone)(),void (*PHYTxError)(),void (*PHYRxDone)(uint8_t*,uint16_t,int16_t,int8_t),void (*PHYRxTimeout)(),void (*PHYRxError)())
{
	if (HWStatus.settings.State != RF_NULL)															//하드웨어 세팅 상태가 RF_NULL이 아니라면
	{
		debug("[ERROR] initialization failed : state (%i) is invalid\n", HWStatus.settings.State);	//디버그 메세지 출력
		return -1; //
	}
    HAL_SetTxDone(PHYTxDone);
    HAL_SetTxTimeout(PHYTxError);
    HAL_SetRxError(PHYRxError);
    HAL_SetRxDone(PHYRxDone);
    HAL_SetRxTimeout(PHYRxTimeout);
    
	return HAL_init_radio();  //라디오 초기화 함수 실행하고 종료
}

//하드웨어 세팅 상태를 확인해서 동작중인지 점검하는 함수 (iDLE 상태면 false, 그 이외에 동작 중이면 true)
bool HAL_cmd_isActive(void)
{
    if (HWStatus.settings.State == RF_IDLE)
    {
        return false;
    }
    
    return true;
}

//HAL_Sleep을 작동하는 함수
void HAL_cmd_Sleep(){
    HAL_Sleep();
}

//rx상태 가져오는 함수
uint8_t HAL_cmd_getRxStatus(){
    uint8_t rxStat;
    rxStat=HW_getModemStatusRx();
    return rxStat;
}




/* 2. HAL functions  */


/* -------------- general purpose functions ----------------------- */
//하드웨어 세팅 상태를 가져오는 함수
RadioState HAL_getState(void)
{
	return HWStatus.settings.State;
}

//tx와 rx의 타이머를 모두 종료하고 sleep 후 IDLE 상태로 돌아가는 함수
void HAL_Sleep( void )
{
    txTimeoutTimer.detach( );
    rxTimeoutTimer.detach( );

	HW_Sleep();
	HWStatus.settings.State = RF_IDLE;
}

//tx와 rx의 타이머를 모두 종료하고 하드웨어 standby 후 IDLE 상태로 돌아가는 함수
void HAL_Standby( void )
{
    txTimeoutTimer.detach( );
    rxTimeoutTimer.detach( );

	HW_Standby();
    HWStatus.settings.State = RF_IDLE;
}

//하드웨어 상태 구조체 가져오는 함수
const HWStatus_t HAL_getHwAbst(void)
{
	return HWStatus;
}






/* ---------------- configuration functions ----------------*/ //구성함수, 값 설정을 위한 함수들

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



//tx를 구성하기 위한 함수
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

//rx를 구성하기 위한 함수
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

//버퍼 비우기
void HAL_clearBuffer()
{
	memset( rxtxBuffer, 0, ( size_t )RX_BUFFER_SIZE );
}





/* ---------------- Interrupt handler functions ----------------- */
//소프트웨어 단에서 타임아웃이 걸린경우
void HAL_swTimeout( void )
{
    switch(HWStatus.settings.State)
	{
    case RF_RX_RUNNING:			//rx 실행 상태이면
		HAL_onRxTimeoutIrq();
        
        OnRxTimeout();			//rx가 타임아웃이라고 처리?
     
        break;
    case RF_TX_RUNNING:			//tx 실행상태일때 타임아웃이 걸린다면 (SPI 전송이 손상된 결과 이런 상황이 발생? 플랫폼 디자인에 따라 다름)
        // Tx timeout shouldn't happen.
        // But it has been observed that when it happens it is a result of a corrupted SPI transfer
        // it depends on the platform design.
        // 
        // The workaround is to put the radio in a known state. Thus, we re-initialize it. 

        // BEGIN WORKAROUND

		#if 0
        // Reset the radio
        HW_reset( );			//라디오 리셋
        // Initialize radio default values 
		HW_Sleep();				
        HW_RadioRegistersInit( );
        HW_SetModem( MODEM_FSK );
		#endif
		HW_init_radio(&HWStatus);	//라디오 디폴트 값으로 초기화
        // Restore previous network type setting.
        HW_SetPublicNetwork( HWStatus.settings.LoRa.PublicNetwork );	//이전 네트워크 타입 세팅 복원
        // END WORKAROUND

		//HW_Sleep();
        HWStatus.settings.State = RF_IDLE;								
		
		
        OnTxTimeout();	////rx가 타임아웃이라고 처리?
        
        break;
    default:
        break;
    }
}



//하드웨어 단에서 타임아웃이 걸린 경우?
void HAL_hwTimeout(void)
{
	RadioState state = HWStatus.settings.State;

	if (state == RF_RX_RUNNING)	//rx 실행 상태일때만 해당 함수 동작
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



//실행이 끝남을 알리는 함수?
void HAL_hwDone(void)
{
	RadioState state = HWStatus.settings.State;
	uint16_t size;
	int16_t rssi;
	int8_t snr;
	rxCrc_e crcres;

	if (state == RF_RX_RUNNING)		//rx 러닝 상태일때
	{
        // Clear Irq
        HW_clearRxIrq();			//인터럽트 요청 초기화
		crcres = HW_checkCRC();		//crc 검사?

        if( crcres == rxcrc_nok )	//crc가 nok, 제대로 오지 않았다면?
        {
            // Clear Irq
            HW_clearRxPayloadCrc();
			
           if( HWStatus.settings.LoRa.RxContinuous == false )	//rx지속이 false로 설정되어 있다면
           {
                HWStatus.settings.State = RF_IDLE;				//IDLE 상태로 바꾸기
           }
            rxTimeoutTimer.detach( );							//rx 타이머 종료
			//HW_Sleep();
            
            OnRxError();										//rx에러발생을 알림
        }
		else					//nok가 아니라면, 즉 제대로 왔다면
		{
			snr = HW_readSnr();
			rssi = HW_readRssi(snr);
			size = HW_getRxPayload(rxtxBuffer);
	        

	        if( HWStatus.settings.LoRa.RxContinuous == false )	//rx지속이 false로 설정되어 있다면
            {
                HWStatus.settings.State = RF_IDLE;				//IDLE 상태로 바꾸기
            }
            rxTimeoutTimer.detach( );
			//HW_Sleep();
            
	        OnRxDone(rxtxBuffer, size, rssi, snr);			//rx가 성공적으로 완료됨을 알림
	        
		}
    }
	else if (state == RF_TX_RUNNING)	//tx 러닝 상태일때
	{
	    txTimeoutTimer.detach();
        // TxDone interrupt
        // Clear Irq
        HW_clearTxIrq();
		
		HWStatus.settings.State = RF_IDLE;
		//HW_Sleep();
		
        OnTxDone();						//tx가 성공적으로 완료됨을 알림
	}
	else								//tx도 rx도 아니라면
	{
		debug("[ERROR] state is weird :%i\n", state);	//에러 출력
	}
	
}



//rx 시작 함수?
void HAL_startRx(uint32_t timeout)
{
	HAL_clearBuffer();
	
	HW_Rx( HWStatus ); // HHI code ->header not included
	HWStatus.settings.State = RF_RX_RUNNING;//HW_setState				//상태를 rx 실행 상태로 바꿔줌
	
	if( timeout != 0 )
	{
		rxTimeoutTimer.attach_us( HAL_swTimeout , timeout * 1e3 ); //x	//timeout이 0이 아니면? 뭐하고 있는거지??
	}
}


//tx 시작 함수? 특히 FSK쪽 내용을 모르겠음
int8_t HAL_startTx( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;				//tx타임아웃을 안쓰겠다
	
    switch(HWStatus.settings.Modem)		//모뎀 상태에 따라 다르게 처리
	{
    case MODEM_FSK:			//FSK일때(안씀)
		#if 0
        {
            HWabs.settings.FskPacketHandler.NbBytes = 0;
            HWabs.settings.FskPacketHandler.Size = size;

			HW_cfgTxLen(HAL_getHwAbst(), size);		//TX의 길이 설정?

            if( ( size > 0 ) && ( size <= 64 ) )				//크기가 0과 64 사이라면
            {
                HWStatus.settings.FskPacketHandler.ChunkSize = size;
            }
            else												//크기가 범위를 벗어나면
            {
                memcpy( rxtxBuffer, buffer, size );			//버퍼에 내용 담기
                HWStatus.settings.FskPacketHandler.ChunkSize = 32;		//청크사이즈 조절
            }

            // Write payload buffer			(페이로드 버퍼에 작성)
            HW_SpiWriteFifo( buffer, HWStatus.settings.FskPacketHandler.ChunkSize );
            HWStatus.settings.FskPacketHandler.NbBytes += HWStatus.settings.FskPacketHandler.ChunkSize;
            txTimeout = HWStatus.settings.Fsk.TxTimeout;
        }
        break;
		#endif
		debug("modem state is invalid(FSK, which is not supported now)\n");
		return -1;

    case MODEM_LORA:		//LORA일 경우
        {
           	HW_setLoRaIqInverted(HWStatus.settings.LoRa.IqInverted);		//IRQinverted 세팅?
            HAL_setLoRaPacketHandlerSize(size);								//로라 패킷 핸들러 크기 설정
            HW_setTxPayload(buffer, size);									//tx의 페이로드 설정

            txTimeout = HWStatus.settings.LoRa.TxTimeout;					//tx 타임아웃 설정
        }
        break;
    }

    HW_Tx( HWStatus, txTimeout );											//tx 실행

	HWStatus.settings.State = RF_TX_RUNNING;								//tx 상태로 변경
    txTimeoutTimer.attach_us(HAL_swTimeout, txTimeout*1e3);					//tx 타이머 설정?


	return 0;
}

//tx 파워 값 설정
void HAL_setRfTxPower(int8_t power)
{
    
    
	HW_SetRfTxPower(HWStatus, power);
}


/* ---------------- 3. inner function ----------------*/
//라디오 초기화 함수
static int HAL_init_radio()
{
	debug( "\n	   Initializing Radio Interrupt handlers \n" );

	//HWStatus.isRadioActive = false;
	//HWStatus.boardConnected = UNKNOWN;

	rxtxBuffer = (uint8_t*)malloc(RX_BUFFER_SIZE*sizeof(uint8_t));	//버퍼 동적 할당
	if (rxtxBuffer == NULL) 										//할당 실패시 디버그 메세지
	{
		debug("[ERROR] failed in initiating radio : memory alloc failed\n");
		return -2;
	}

	//initialize the HW abstraction by the initial values in API header
	HWStatus.settings.Channel = HAL_INIT_RF_FREQUENCY;
	HWStatus.settings.Modem = HAL_INIT_MODEM;


	if (HWStatus.settings.Modem == MODEM_LORA)		//모뎀이 로라라면 초기값으로 설정
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
	else											//아니면 디버그 메세지 출력
	{
		debug("[HAL][ERROR] failed to initialize : currently %i modem is disabled!\n", HAL_INIT_MODEM);
		return -1;
	}

	
	HWStatus.boardConnected = HW_init_radio(&HWStatus);		//보드 연결 여부 알리기
	 
	//TX initial config (tx 초기값 구성)
	HAL_TxInitConfig (HWStatus);
	
	//RX initial config	(rx 초기값 구성)
	HAL_RxInitConfig (HWStatus);
		
    HWStatus.settings.State = RF_IDLE;						//초기니까 IDLE 상태로
	
	
    return 0;
}

