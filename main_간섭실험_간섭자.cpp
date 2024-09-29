#include "mbed.h"
#include "string.h"
#include "PHYMAC_layer.h"
#include "ARQ_FSMevent.h"
#include "ARQ_msg.h"
#include "ARQ_parameters.h"


#define DBGMSG_ARQ          0 //debug print control 0이면 디버그 off, 1이면 디버그 on
//ARQ FSM state 
#define MAINSTATE_IDLE      0
#define MAINSTATE_TX        1
#define MAINSTATE_WAIT      2

//serial port interface
Serial pc(USBTX, USBRX);
//ARQ variables
uint8_t main_state = MAINSTATE_IDLE; //protocol state
//ARQ retransmission timer
Timeout timer;
uint8_t timerStatus = 0;
//ARQ source/destination ID
uint8_t endNode_ID = 1;
uint8_t dest_ID = 0;
//ARQ sequence number
uint8_t seqNum = 0;
//ARQ retransmission counter
uint8_t retxCnt = 0;
//ARQ PDU context/size
uint8_t arqPdu[200];
uint8_t pduSize;
uint8_t txType;
//ARQ SDU (input)
uint8_t originalWord[200];
uint8_t wordLen = 0;
//ARQ reception event related variables
uint8_t dataSrc; //source ID
uint8_t dataSeq; //sequence number
uint8_t arqAck[5]; //ARQ ACK message
uint8_t TxCnt = 0;//새롭게 추가(100회 반복 용도) <--------------------------------------------------------------------------------------------------------------
uint32_t RcvdCnt = 0;//수신하면 증가
uint8_t CrtRcvdCnt = 0;//수신 후 무결성까지 확보되면 증가

// ========================== 2024.04.01 추가 ==========================
uint8_t CRCbreak = 0; //CRC 깨진거 누적 기록
uint8_t TotalRxRcvd = 0; // RX 총 수신횟수 
uint8_t TotalTxSend = 0; // TX 총 송신 횟수
uint8_t TotalAckRcvd = 0; // TX에서 105회 받을때까지 계속 보내는식으로 바꾸기로 해서 Ack 받은 횟수 세야할거 같아서 추가
uint8_t totalAckSend = 0;	//RX에서 ACK을 보낸 횟수도 세서 출력하기 위해서
int16_t totalRssi = 0; 		//rssi 값들의 합을 구해 평균 내기 위해서
time_t seconds = 0;			//시간 측정을 위한 밀리초 변수
Timer timers;				//시간 측정을 위한 타이머 객체

//Event handler 
//timer event : ARQ timeout
void arqMain_timeoutHandler(void)
{
	timerStatus = 0;
	arqEvent_setEventFlag(arqEvent_arqTimeout);
}
//interface event : DATA_CNF, TX done event
void arqMain_dataCnfFunc(int err)
{
	//if (arqMsg_checkIfData(arqPdu))
	if (txType == ARQMSG_TYPE_DATA)
	{
		arqEvent_setEventFlag(arqEvent_dataTxDone);
	}
	//else if (arqMsg_checkIfAck(arqPdu))
	else if (txType == ARQMSG_TYPE_ACK)
	{
		arqEvent_setEventFlag(arqEvent_ackTxDone);
	}
}
//interface event : DATA_IND, RX data has arrived
void arqMain_dataIndFunc(uint8_t srcId, uint8_t* dataPtr, uint8_t size)
{
	debug_if(DBGMSG_ARQ, "\n --> DATA IND : src:%i, size:%i\n", srcId, size);

	//ready for ACK TX
	if (arqMsg_checkIfData(dataPtr))
	{
		dataSeq = arqMsg_getSeq(dataPtr);
		dataSrc = srcId;

		pc.printf("\n -------------------------------------------------\nRCVD from %i : %s (length:%i)\n -------------------------------------------------\n", arqMsg_getWord(dataPtr), size);

		arqEvent_setEventFlag(arqEvent_dataRcvd);
	}
	else if (arqMsg_checkIfAck(dataPtr))
	{
		pc.printf("\n -------------------------------------------------\n[ARQ] ACK is received (%i)\n -------------------------------------------------\n", arqMsg_getSeq(dataPtr));

		arqEvent_setEventFlag(arqEvent_ackRcvd);
	}
}
/* 기존에 있던 함수지만 쓰지 않음<----------------------------------------------------------------------------------------------------------------------------------------------------------------
//application event : SDU generation
void arqMain_processInputWord(void)
{
	char c = pc.getc();
	if (main_state == MAINSTATE_IDLE &&
		!arqEvent_checkEventFlag(arqEvent_dataToSend))
	{
		if (c == '\n' || c == '\r')
		{
			originalWord[wordLen++] = '\0';
			arqEvent_setEventFlag(arqEvent_dataToSend);
			pc.printf("word is ready! ::: %s\n", originalWord);
		}
		else
		{
			originalWord[wordLen++] = c;
			if (wordLen >= ARQMSG_MAXDATASIZE - 1)
			{
				originalWord[wordLen++] = '\0';
				arqEvent_setEventFlag(arqEvent_dataToSend);
				pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
			}
		}
	}

}
*/
//timer related functions
void arqMain_startTimer()
{
	uint8_t waitTime = ARQ_MINWAITTIME + rand() % (ARQ_MAXWAITTIME - ARQ_MINWAITTIME);
	timer.attach(arqMain_timeoutHandler, waitTime);
	timerStatus = 1;
}
void arqMain_stopTimer()
{
	timer.detach();
	timerStatus = 0;
}
//TX function
void arqMain_sendData(uint8_t* msg, uint8_t size, uint8_t dest)
{
	phymac_dataReq(msg, size, dest);
	txType = msg[ARQMSG_OFFSET_TYPE];
}
//FSM operation
int main(void) {
	uint8_t flag_needPrint = 1;
	uint8_t prev_state = 0;
	//initialization
	pc.printf("------------------ ARQ protocol starts! --------------------------\n");
	arqEvent_clearAllEventFlag();
	//source & destination ID setting
	dest_ID = 17;//src와 dest ID 자동 설정 (scanf 입력 X)<---------------------------------------------------------------------------------------------------------------------------------------------------------
	//pc.printf(":: ID for the destination : %d\n",dest_ID);
	//pc.scanf("%d", &dest_ID);
	endNode_ID = 7;					//src id
	//pc.printf(":: ID for this node : %d\n",endNode_ID);
	//pc.scanf("%d", &endNode_ID);	
	//pc.getc();기존 함수 미사용	<---------------------------------------------------------------------------------------------------------------------------------------------------
	pc.printf("endnode : %i, dest : %i\n", endNode_ID, dest_ID);	
	phymac_init(endNode_ID, arqMain_dataCnfFunc, arqMain_dataIndFunc);
	//pc.attach(&arqMain_processInputWord, Serial::RxIrq); 함수 미사용(사용시 에러)<---------------------------------------------------------------------------------------------------------
	
	//연속 전송할거면 굳이 얘 매번 처리할 필요 없지 않나? 싶어서 일단 밖으로 뺌
	//msg header setting <---------------------------------------------------------------------------------------------------------------------				
	const char* s1 = "Hello";                //고정된 메시지     
	strcpy((char *)originalWord, s1);        //송신될 패킷에 입력    
	originalWord[strlen(s1)] = '\0';	     //null 부착     			
	wordLen = strlen(s1);                    //길이 계산  
	pduSize = arqMsg_encodeData(arqPdu, originalWord, seqNum++, wordLen);    //originalWord-> output_bits, wordLen->output_len으로 변경(fec를 통해 길이가 바뀌었음으로)

	while (1)
	{
		if (prev_state != main_state)
		{
			debug_if(DBGMSG_ARQ, "[ARQ] State transition from %i to %i\n", prev_state, main_state);
			prev_state = main_state;
		}
		if (arqEvent_checkEventFlag(arqEvent_dataToSend)) //no ACK, data to send
		{

			arqMain_sendData(arqPdu, pduSize, dest_ID);	
			
			srand(time(NULL));																									//(오류 뜨면 +1초 정도 씩 더 해주면 될듯?)
			int random = ((rand() % 57) + 57);			//무한 전송하는 간섭자의 경우 delay를 최솟값~최솟값*2 사이의 랜덤 값으로 설정 (sf7: 57~113, sf10: 371~741, sf12: 1483~2965)
			wait_ms(random);							//random ms 만큼 대기
			//딜레이 설정(sf 별로 다르게 설정해야함- sf7: 56.6 ms, sf8: 102.9 ms, sf9: 205.8 ms, sf10: 370.7 ms, sf11: 741.4 ms, sf12: 1482.8 ms(overhead size 12byte, payload size 10byte 기준? 계산해주는 사이트를 참고함))

			retxCnt = 0;
			flag_needPrint = 1;
		}
		else if (flag_needPrint == 1)
			{//-------------------------------------------------------------------------------------
				arqEvent_setEventFlag(arqEvent_dataToSend);
				flag_needPrint = 0;
			}
	}
}