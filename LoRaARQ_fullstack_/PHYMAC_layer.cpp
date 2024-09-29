#include "PHYHAL_interface.h"
#include "PHYMAC_layer.h"

#define DBGMSG_PHYMAC                   0       //0이면 디버그 off, 1이면 디버그 on

// ------------- PHYMAC parameters              파이맥 파라미터들
#define PHYMAC_PARAM_PWR                14                      //전원 세기 기본값 14
#define PHYMAC_PARAM_BW                 HAL_LRBW_125            //대역폭
#define PHYMAC_PARAM_DR                 HAL_LRDatarate_SF7      //데이터 레이트(data rate), 기본값은 SF9였으나 수정함, "실험에서 이 파라미터를 주로 수정하여 바이너리 파일 생성"
#define PHYMAC_PARAM_CR                 HAL_LRCoderate_4_5      //코드 레이트
#define PHYMAC_PARAM_PREAMBLELEN        8                       //프리앰블(전문 길이)
#define PHYMAC_PARAM_FREQ               922100000               //주파수
#define PHYMAC_PARAM_RXTIME             0                       //rx time

// ------------- PHYMAC PDU 
/* PHYMAC PDU format 

PHYMAC header ------>|  SDU --->
  Type   srcID  dstID  Len    Contents
|--1B--|--1B--|--1B--|--1B--|----var----|
*/

#define PHYMAC_MAX_BUFFERSIZE           32                      //최대 버퍼 크기
#define PHYMAC_PDUOFFSET_TYPE           0                       //타입
#define PHYMAC_PDUOFFSET_SRCID          1                       //srcID
#define PHYMAC_PDUOFFSET_DSTID          2                       //dstID
#define PHYMAC_PDUOFFSET_LEN            3                       //길이
#define PHYMAC_PDUOFFSET_DATA           4                       //데이터 내용

#define PHYMAC_PDUSIZE_HEADER           4                       //헤더 크기(총 4바이트, 위에 주석 참고)

#define PHYMAC_PDUTYPE_DATA             0                       //PDUTYPE 데이터

//메인 함수에 파라미터 출력을 위한 용도
int getPHYMAC_PARAM_PWR(void){return PHYMAC_PARAM_PWR;}
int getPHYMAC_PARAM_DR(void){return PHYMAC_PARAM_DR;}


static uint8_t phymac_state = 0; //0 - RX, 1 - TX
static uint8_t phymac_id;
static void (*phymac_dataCnfFuncPtr)(int);                                                      //tx가 데이터를 보냈는지 ack을 보냈는지 확인하는 함수?(arqLLI_dataCnfFunc 이름만 바뀐듯함)
static void (*phymac_dataIndFuncPtr)(uint8_t srcId, uint8_t* ptr, uint8_t size);                //rx가 데이터를 받았는지 ack을 받았는지 인지하는 함수?
static int16_t rssiRcvd;
static int8_t snrRcvd;
//buffer for TX /RX
static uint8_t phymac_dataRxBuffer[PHYMAC_MAX_BUFFERSIZE];      //rx버퍼
static uint8_t phymac_dataTxBuffer[PHYMAC_MAX_BUFFERSIZE];      //tx버퍼

//rx 구동함수
void phymac_startRx(void)
{
    HAL_cmd_SetRxConfig(PHYMAC_PARAM_BW, PHYMAC_PARAM_DR, PHYMAC_PARAM_CR, 
                        PHYMAC_PARAM_PREAMBLELEN, PHYMAC_PARAM_FREQ, PHYMAC_PARAM_RXTIME);      //rx 세팅
    HAL_cmd_receive();                                                                          //수신 함수, 데이터 받기?
    phymac_state = PHYMAC_STATE_RX;                                                             //rx로 파이맥 상태 설정    
}


/* PHY - HAL interface functions (파이-할 인터페이스 함수들) */

//1. TX DONE
void phymac_txDoneHandler(void)     //tx 완료를 알리는 핸들러(tx가 완료되면)
{
    debug_if(DBGMSG_PHYMAC, "[PHYMAC] msg is transmitted\n");
    HAL_cmd_Sleep();                //잠시 대기시키고

    //change state to RX
    phymac_startRx();               //rx 시작
    phymac_dataCnfFuncPtr(PHYMAC_ERR_NONE); //에러 없이 보냈다고 알리기
}

//2. TX ERROR                       
void phymac_txErrorHandler(void)    //tx 에러 핸들러(tx가 에러가 났다면)
{
    HAL_cmd_Sleep();                //대기했다가
    
    //change state to RX
    phymac_startRx();               //rx 시작
    phymac_dataCnfFuncPtr(PHYMAC_ERR_HWERROR); //하드웨어 에러라고 알리기
}

//3. RX DONE : something is received
void phymac_rxDoneHandler(uint8_t* dataPtr, uint16_t size, int16_t rssi, int8_t snr)    //rx 완료 핸들러(뭔가 받으면)
{
    uint8_t dataIndFlag = 0;        //데이터 인지 플래그
    debug_if(DBGMSG_PHYMAC, "\n[PHYMAC] RX - type - %i, id : %i/%i, size : %i, rssi : %i, snr : %i\n\n", 
                dataPtr[PHYMAC_PDUOFFSET_TYPE], dataPtr[PHYMAC_PDUOFFSET_SRCID], dataPtr[PHYMAC_PDUOFFSET_DSTID], size, rssi, snr);

    if (size > PHYMAC_MAX_BUFFERSIZE)           //만약 받은 데이터 크기가 버퍼사이즈를 넘어간다면
    {
        debug("[PHYMAC] WARNING : PDU size is too large!! PDU will be truncated...\n");
        size = PHYMAC_MAX_BUFFERSIZE;           //오류 메세지 출력 후 버퍼 최대 사이즈로 변경
    }
    memcpy(phymac_dataRxBuffer, dataPtr, size*sizeof(uint8_t)); //데이터 버퍼에 담기
    rssiRcvd = rssi;                                            //받은 rssi 값 
    snrRcvd = snr;                                              //받은 snr 값

    if (phymac_dataRxBuffer[PHYMAC_PDUOFFSET_TYPE] == PHYMAC_PDUTYPE_DATA &&        //만약 받은 데이터가 존재한다면(pdu타입이 같고 도착 id가 파이맥 id와 같다면)
        phymac_dataRxBuffer[PHYMAC_PDUOFFSET_DSTID] == phymac_id)
    {
        dataIndFlag = 1;                                                            //플래그를 1로 변경(받은게 존재한다는 뜻)
    }
    
    //change state to RX
    if (phymac_state == 0)  //파이맥 상태가 0(rx)라면 rx 상태 구동
    {
        HAL_cmd_Sleep();
        phymac_startRx();   //rx 시작
    }
    else                    //0이 아니라 tx가 된다면 오류 메세지 출력
    {
        debug("[PHYMAC] WARNING : keep the state as TX, since the TX is on-going right now\n");
    }

    if (dataIndFlag == 1)                                                           //메세지 받은게 있다면
    {
        phymac_dataIndFuncPtr(phymac_dataRxBuffer[PHYMAC_PDUOFFSET_SRCID], &phymac_dataRxBuffer[PHYMAC_PDUOFFSET_DATA], size-PHYMAC_PDUSIZE_HEADER); //데이터가 왔다고 알리기(크기는 헤더 크기를 제외하고 데이터 길이만)
    }
}

//4-5. RX TIMEOUT & ERROR
void phymac_rxTimeoutErrorHandler(void)     //rx 타임아웃 에러 핸들러(타임아웃 에러가 발생한다면)
{
    //change state to RX
    if (phymac_state == 0)                  //rx 상태면 rx 시작
    {
        HAL_cmd_Sleep();
        phymac_startRx();
    }
    else                                    //아니라면 에러메세지 출력
    {
        debug("[PHYMAC] WARNING : keep the state as TX, since the TX is on-going right now\n");
    }
}

//파이맥 초기화 함수
void phymac_init(uint8_t id, void (*dataCnfFunc)(int), void (*dataIndFunc)(uint8_t, uint8_t*, uint8_t))
{
    //받은 인자로 초기화    
    phymac_id = id;
    phymac_dataCnfFuncPtr = dataCnfFunc;
    phymac_dataIndFuncPtr = dataIndFunc;

    debug_if(DBGMSG_PHYMAC, "[PHYMAC] ID : %i\n", phymac_id);

    //initialize HAL layer and HW  (HAL 계층과 하드웨어 초기화)
    HAL_cfg_init(phymac_txDoneHandler, 
                 phymac_txErrorHandler,
                 phymac_rxDoneHandler,
                 phymac_rxTimeoutErrorHandler,
                 phymac_rxTimeoutErrorHandler);

    phymac_dataTxBuffer[PHYMAC_PDUOFFSET_TYPE] = PHYMAC_PDUTYPE_DATA; 
    phymac_dataTxBuffer[PHYMAC_PDUOFFSET_SRCID] = id;

    //entry to RX state
    HAL_cmd_Sleep();

    //start RX
    phymac_startRx();
}



//보낼 데이터 제작하는 함수
int phymac_dataReq(uint8_t* dataPtr, uint8_t size, uint8_t destId)
{
    if (phymac_state != PHYMAC_STATE_RX)                            //rx에서는 할 수 없으므로 오류 메세지 출력 후 wrongstate 에러 반환
    {
        debug("[PHYMAC] ERROR : failed to handle DATA_REQ : invalid state (%i)\n", phymac_state);
        return PHYMAC_ERR_WRONGSTATE;
    }
    else if (size > PHYMAC_MAX_BUFFERSIZE-PHYMAC_PDUSIZE_HEADER)    // 크기가 넘어가면 안되므로 오류 메세지 출력 후 사이즈 에러 반환
    {
        debug("[PHYMAC] ERROR : failed to handle DATA_REQ : size is too large (%i > %i)\n", size, PHYMAC_MAX_BUFFERSIZE);
        return PHYMAC_ERR_SIZE; 
    }

    debug_if(DBGMSG_PHYMAC,"[PHYMAC] Data Req (size:%i, dest:%i)\n", size,destId);

    //TX data setting (tx 데이터 세팅)
    phymac_dataTxBuffer[PHYMAC_PDUOFFSET_TYPE] = PHYMAC_PDUTYPE_DATA;
    phymac_dataTxBuffer[PHYMAC_PDUOFFSET_DSTID] = destId;
    phymac_dataTxBuffer[PHYMAC_PDUOFFSET_LEN] = size+PHYMAC_PDUSIZE_HEADER;
    memcpy(&phymac_dataTxBuffer[PHYMAC_PDUOFFSET_DATA], dataPtr, size*sizeof(uint8_t)); //한번에 이동

    //RX -> TX state transition 
    //TX start
    HAL_cmd_SetTxConfig(PHYMAC_PARAM_PWR, PHYMAC_PARAM_BW, PHYMAC_PARAM_DR, PHYMAC_PARAM_CR, PHYMAC_PARAM_PREAMBLELEN, PHYMAC_PARAM_FREQ);  //tx 값 세팅(여기를 건드리면)
    HAL_cmd_transmit(phymac_dataTxBuffer, size+PHYMAC_PDUSIZE_HEADER);                                                                      //전송 명령
    phymac_state = PHYMAC_STATE_TX;                                                                                                         //파이맥  상태 tx로 전환

    return PHYMAC_ERR_NONE;                                                                                                                 //문제 없이 끝났다면 에러 없다고 반환하기
}

int16_t phymac_getDataRssi(void)            //rssi 반환
{
    return rssiRcvd;
}

int8_t phymac_getDataSnr(void)              //snr 반환
{
    return snrRcvd;
}

//config source ID only when the state is in RX
//rx 상태에서 파이맥 id가 id와 같다면 에러가 없다고 판단
int phymac_configSrcId(uint8_t id)
{
    //validity check
    if (phymac_state == 1) //if phymac state is in TX
    {
        debug("[PHYMAC] ERROR : failed to handle configuration : source ID cannot be changed in TX state (%i)\n", phymac_state);    //tx 상태라면 디버그 메세지 출력(rx에서만 작동하는 함수)
        return PHYMAC_ERR_WRONGSTATE;
    }

    phymac_id = id;                         //rx 상태라면 입력 값을 파이맥 id와 같게 하고 에러가 없다고 반환
    return PHYMAC_ERR_NONE;
}
