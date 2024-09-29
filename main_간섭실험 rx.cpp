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

uint32_t err = 0;				//잘못온 메세지 개수
int endcnt = 0;				//실험 횟수

static int handle = true;

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

		char str[10];										//아무 변수로 일단 초기화
		char* str2 = (char*)arqMsg_getWord(dataPtr);	
		strcpy(str, str2);									//받은 데이터 str 변수에 복사
		//printf("%s\n", str);								//받은 데이터가 뭔지 보기 (테스트용)

		if (strcmp(str, "Hello") != 0){	//만약  Hello(원래 와야할 메세지)가 아닐때만 출력하도록 이였는데 그냥 출력 안하게 수정
			//pc.printf("\n -------------------------------------------------\nRCVD from %i : %s (length:%i)\n -------------------------------------------------\n", srcId, arqMsg_getWord(dataPtr), size);
			err++;			//틀렸으니까 틀린 개수 증가
		}else{
			RcvdCnt++;
		}

		if(RcvdCnt == 5){	//받은 횟수가 5번이면 실험 시작(타이머 시작)
			timers.start();	//시간 측정 시작
		}
		else if((RcvdCnt % 100) == 5 && (handle)){	//만약 받은 횟수가 5회로부터 100번이 지나가면 ---> 앞에 5회는 연습용, 100개 마다 출력하도록 변경
			timers.stop();												//타이머 멈추고 100개 보내는 동안 걸린 시간 측정한거 출력
			seconds = timers.read_ms();									//밀리초 반환
    				
			pc.printf("Execution time: %ld milli secomds\n", seconds);	//측정한 시간 출력, 밀리초
			pc.printf("error count: %d\n", err);						//틀린 개수 출력
			
			endcnt++;													//실험 횟수 증가

			//만약 10번 else if문 안으로 들어왔다면 (1005개의 패킷을 받았다면) 수신 종료하기
			if (endcnt == 10){
				pc.printf("-----------------------The End-----------------------\n");
				handle = false;
				return; 												//수신 프로그램 종료 => 어떻게 종료하지?
			}

			timers.reset();												//타이머 초기화
			err = 0;													//출력했으므로 0으로 초기화 하고 다시 측정
			timers.start();												//다시 시간 측정 시작
		}
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
	dest_ID = 12;//src와 dest ID 자동 설정 (scanf 입력 X)<---------------------------------------------------------------------------------------------------------------------------------------------------------
	//pc.printf(":: ID for the destination : %d\n",dest_ID);
	//pc.scanf("%d", &dest_ID);
	endNode_ID = 34;					//src id
	//pc.printf(":: ID for this node : %d\n",endNode_ID);
	//pc.scanf("%d", &endNode_ID);	
	//pc.getc();기존 함수 미사용	<---------------------------------------------------------------------------------------------------------------------------------------------------
	pc.printf("endnode : %i, dest : %i\n", endNode_ID, dest_ID);	
	phymac_init(endNode_ID, arqMain_dataCnfFunc, arqMain_dataIndFunc);
	//pc.attach(&arqMain_processInputWord, Serial::RxIrq); 함수 미사용(사용시 에러)<---------------------------------------------------------------------------------------------------------
	while (handle)
	{
		if (prev_state != main_state)
		{
			debug_if(DBGMSG_ARQ, "[ARQ] State transition from %i to %i\n", prev_state, main_state);
			prev_state = main_state;
		}
		switch (main_state)
		{
		case MAINSTATE_IDLE:
			//send ACK first
			if (arqEvent_checkEventFlag(arqEvent_dataRcvd))
			{
				main_state = MAINSTATE_IDLE;					//계속 받을 수 있게 IDLE에 있기
				flag_needPrint = 1;
				arqEvent_clearEventFlag(arqEvent_dataRcvd);
			}
			else if (arqEvent_checkEventFlag(arqEvent_dataToSend)) //no ACK, data to send
			{
				//msg header setting <---------------------------------------------------------------------------------------------------------------------						
				const char* s1 = "Hello";                //고정된 메시지     
				strcpy((char *)originalWord, s1);        //송신될 패킷에 입력    
				originalWord[strlen(s1)] = '\0';	     //null 부착     			
				wordLen = strlen(s1);                    //길이 계산  

				pduSize = arqMsg_encodeData(arqPdu, originalWord, seqNum++, wordLen);    //originalWord-> output_bits, wordLen->output_len으로 변경(fec를 통해 길이가 바뀌었음으로)
				arqMain_sendData(arqPdu, pduSize, dest_ID);		
				TotalTxSend++; //보낸 패킷 횟수 추가 <--- 2024.04.01 추가			
				retxCnt = 0;
				pc.printf("[ARQ] sending to %i (seq:%i)\n", dest_ID, seqNum - 1);
				main_state = MAINSTATE_TX;
				flag_needPrint = 1;
				wordLen = 0;
				arqEvent_clearEventFlag(arqEvent_dataToSend);
			}
			else if (arqEvent_checkEventFlag(arqEvent_dataTxDone))
			{
				pc.printf("[WARNING] cannot happen event %i in the state %i\n", arqEvent_dataTxDone, main_state);
				arqEvent_clearEventFlag(arqEvent_dataTxDone);
			}
			else if (arqEvent_checkEventFlag(arqEvent_ackTxDone))
			{
				pc.printf("[WARNING] cannot happen event %i in the state %i\n", arqEvent_ackTxDone, main_state);
				arqEvent_clearEventFlag(arqEvent_ackTxDone);
			}
			else if (arqEvent_checkEventFlag(arqEvent_ackRcvd))
			{
				pc.printf("[WARNING] cannot happen event %i in the state %i\n", arqEvent_ackRcvd, main_state);
				arqEvent_clearEventFlag(arqEvent_ackRcvd);
			}
			else if (arqEvent_checkEventFlag(arqEvent_arqTimeout))
			{
				pc.printf("[WARNING] cannot happen event %i in the state %i\n", arqEvent_arqTimeout, main_state);
				arqEvent_clearEventFlag(arqEvent_arqTimeout);
			}
			else if (flag_needPrint == 1)
			{//RX 만들 때 주석 처리 , TX 만들 때는 주석 해제하고 위에서 src와 dest ID 반대로 바꾸기<-------------------------------------------------------------------------------------
				flag_needPrint = 0;
			}

			break;

		case MAINSTATE_TX:

			//data TX finished
			if (arqEvent_checkEventFlag(arqEvent_dataTxDone))
			{
				main_state = MAINSTATE_WAIT;
				arqMain_startTimer();

				arqEvent_clearEventFlag(arqEvent_dataTxDone);
			}
			else if (arqEvent_checkEventFlag(arqEvent_ackTxDone))
			{
				if (timerStatus == 1 ||
					arqEvent_checkEventFlag(arqEvent_arqTimeout))
				{
					main_state = MAINSTATE_WAIT;
				}
				else
				{
					main_state = MAINSTATE_IDLE;
				}

				arqEvent_clearEventFlag(arqEvent_ackTxDone);
			}

			break;

		case MAINSTATE_WAIT:

			//data TX finished
			if (arqEvent_checkEventFlag(arqEvent_ackRcvd))
			{
				TotalAckRcvd++; //TX가 Ack받은 횟수  <--- 2024.04.01 추가
				arqMain_stopTimer();
				main_state = MAINSTATE_IDLE;

				arqEvent_clearEventFlag(arqEvent_ackRcvd);
			}
			else if (arqEvent_checkEventFlag(arqEvent_arqTimeout))
			{
				pc.printf("[ARQ] Time out! retransmission (%i)\n", retxCnt);

				if (retxCnt < ARQ_MAXRETRANSMISSION)
				{
					arqMain_sendData(arqPdu, pduSize, dest_ID);
					retxCnt++;
					TotalTxSend++; //재전송 할때도 보낸 패킷 횟수 추가 <--- 2024.04.01 추가
					main_state = MAINSTATE_TX;
				}
				else
				{
					pc.printf("[ARQ] max retransmission reached! give up sending\n");
					main_state = MAINSTATE_IDLE;
				}

				arqEvent_clearEventFlag(arqEvent_arqTimeout);
			}
			else if (arqEvent_checkEventFlag(arqEvent_dataRcvd))
			{
				arqMsg_encodeAck(arqAck, dataSeq);
				arqMain_sendData(arqAck, ARQMSG_ACKSIZE, dataSrc);
				totalAckSend++; 		//RX에서 ack을 보낸 개수 세기<--- 2024.04.01 추가

				main_state = MAINSTATE_TX;
				arqEvent_clearEventFlag(arqEvent_dataRcvd);
			}
			else if (arqEvent_checkEventFlag(arqEvent_dataTxDone))
			{
				debug_if(DBGMSG_ARQ, "[WARNING] cannot happen event %i in the state %i\n", arqEvent_dataTxDone, main_state);
				arqEvent_clearEventFlag(arqEvent_dataTxDone);
			}
			else if (arqEvent_checkEventFlag(arqEvent_ackTxDone))
			{
				debug_if(DBGMSG_ARQ, "[WARNING] cannot happen event %i in the state %i\n", arqEvent_ackTxDone, main_state);
				arqEvent_clearEventFlag(arqEvent_ackTxDone);
			}
			else if (arqEvent_checkEventFlag(arqEvent_dataToSend)) //no ACK, data to send
			{
				debug_if(DBGMSG_ARQ, "[WARNING] cannot happen event %i in the state %i\n", arqEvent_dataToSend, main_state);
				arqEvent_clearEventFlag(arqEvent_dataToSend);
			}
			break;

		default:
			break;
		}
	}
}