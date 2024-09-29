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
uint8_t RcvdCnt = 0;//수신하면 증가
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

//crc 파트
#define BITS_PER_BYTE 8                 
#define FEC_REPLICATION_FACTOR 3
#define CRC8_POLY 0x07//crc 다항식
uint8_t output_bits[200];//fec 인코딩 때 사용하는 전역변수
// CRC 계산 함수
uint8_t calculate_crc8(const uint8_t *data, size_t len) {
	uint8_t crc = 0;
	for (size_t i = 0; i < len; ++i) {
		crc ^= data[i];
		for (int j = 0; j < 8; ++j) {
			crc = crc & 0x80 ? (crc << 1) ^ CRC8_POLY : crc << 1;
		}
	}
	return crc;
}
//fec 생성 함수
size_t encode_fec(uint8_t *word, size_t word_len) {
	size_t bit_index = 0;
	// 단어+CRC 를 비트로 변환하여 저장
	for (size_t i = 0; i < word_len; ++i) {
		uint8_t byte = word[i];
		uint32_t output = 0;
		for (int j = 7; j >= 0; --j) {
			uint8_t bit = byte & (1 << j);
			// bit가 1이면 마지막 3개의 비트를 1로 초기화 후 FEC_REPLICATION_FACTOR만큼 왼쪽 shift
			// bit가 0이면 초기화 없이 shift연산만 처리
			if (bit) {
				output = output | 0x7;          //기본값 0x7, 0x5 = 1비트 에러낸 상태 
			}
			// bit가 0이면 초기화 없이 shift연산만 처리
			output = output << FEC_REPLICATION_FACTOR;
		}
		output_bits[bit_index++] = (output & 0x07F80000) >> 19;
		output_bits[bit_index++] = (output & 0x0007F800) >> 11;
		output_bits[bit_index++] = (output & 0x000007F8) >> 3;
	}
	output_bits[bit_index] = '\0'; // 문자열의 끝에 NULL 추가
	return bit_index;
}
//fec 디코딩 함수
size_t decode_fec(uint8_t * input_bits, uint8_t *compressed_bits, size_t wordlen) {
	size_t output_index = 0;
	for (size_t i = 0; i < wordlen; i += FEC_REPLICATION_FACTOR) {
		uint32_t input_bit = 0;
		input_bit += input_bits[i] << 16;
		input_bit += input_bits[i + 1] << 8;
		input_bit += input_bits[i + 2];
		uint8_t output_bit = 0;
		for (int j = 0; j < 8; j++) {
			uint8_t count_zero = 0;
			uint8_t count_one = 0;
			uint8_t bit = input_bit & 0x7;
			input_bit = input_bit >> 3;
			for (int k = 0; k < 3; k++) {
				if (bit & 0x1) {
					count_one++;
				}
				else {
					count_zero++;
				}
			}
			if (count_one > count_zero) {
				output_bit = output_bit | (1 << j);
			}
		}
		//printf("%2x\n", output_bit);		//결과 확인용
		compressed_bits[output_index++] = output_bit;
	}
	compressed_bits[output_index] = '\0'; // 문자열의 끝에 NULL 추가
	// 결과를 출력합니다.
	/*printf("\nfec 디코딩된 결과: ");
	for (uint8_t i = 0; i < output_index; i++) {
		printf("%2x", compressed_bits[i]);
	}
	printf("\n");*/
	return output_index;
}
//crc 파트 끝
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
		RcvdCnt++;
		TotalRxRcvd++; //일단 받았으면 수신횟수 누적 (RX쪽에서 받은 횟수이기 때문에 TX에서도 세줘야함 = TX에서 RX로 도착 안한경우)  <--- 2024.04.01 추가
		//fec, crc 디코딩 하고 출력-------------------------------------
        uint8_t* word; 
        word = arqMsg_getWord(dataPtr);                                     //받은 데이터 가져오기
        size_t outputLen = size-2;                                          //헤더값 2개 뺀 게 word의 길이

        uint8_t decode_bit[(uint8_t)(outputLen / FEC_REPLICATION_FACTOR) + 1];    //fec 디코딩한 결과 저장할 배열
        outputLen = decode_fec(word, decode_bit, outputLen);                //fec 디코딩
        //crc 디코딩    
        if(calculate_crc8(decode_bit, outputLen) == 0){                     //crc 값에 이상이 없다면
            pc.printf("crc 무결성 확인, ACK를 보냅니다.");
			CrtRcvdCnt++;//무결성 검사 통과
        }
        else{                                                               //crc 값에 이상이 있다면
            pc.printf("crc 무결성 오류.");
			CRCbreak++;//수신했지만 crc 깨진값 횟수 추가  <--- 2024.04.01 추가
        }
        //crc,fec 디코딩 파트 끝---------------------------------------
        decode_bit[outputLen-1] = '\0';                                     //crc 값 한 바이트 떼기

		dataSeq = arqMsg_getSeq(dataPtr);
		dataSrc = srcId;

		pc.printf("\n -------------------------------------------------\nRCVD from %i : %s (length:%i)\n -------------------------------------------------\n", srcId, decode_bit, size);

		if(RcvdCnt >= 5){		//6번째 수신부터 rssi의 값들을 더하기
			totalRssi += phymac_getDataRssi();
		}
		if(RcvdCnt >= 105){	//만약 받은 횟수가 105회면 ---> 앞에 5회는 연습용 
			//pc.printf("PDR : %f \n", 100.0 * (double)CrtRcvdCnt / (double)RcvdCnt);//pdr 출력해 보기 (재전송한 횟수도 pdr에 들어가게 하기->tx가 보낸거랑 rx가 받은거 보고 직접계산하기로)
			pc.printf("Total Rcvd: %d, CRC error: %d\n", TotalRxRcvd, CRCbreak); //총 패킷 수신횟수(받는데 성공하긴한 경우), crc오류횟수(데이터가 깨진 경우) <--- 2024.04.01 추가
			pc.printf("Average RSSI: %d\n", (totalRssi/(RcvdCnt-5)));		//rssi 값들의 평균 출력(맨 앞의 5번은 제외시키고)
			pc.printf("Total ACK Send: %d\n", totalAckSend+1);	//보낸 ack의 개수, 마지막 1개는 출력문이 나오고나서 반영되어 미리 1을 더함(rx에서 자기가 몇개 보냈는지 세서 출력할 수 있도록)
			pc.printf("power: %d, DR: %d\n", getPHYMAC_PARAM_PWR(), getPHYMAC_PARAM_DR());	//RX 관련 파라미터 보여주기	
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
	while (1)
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
				arqMsg_encodeAck(arqAck, dataSeq);
				arqMain_sendData(arqAck, ARQMSG_ACKSIZE, dataSrc);
				totalAckSend++; 		//RX에서 ack을 보낸 개수 세기<--- 2024.04.01 추가

				main_state = MAINSTATE_TX;
				flag_needPrint = 1;
				arqEvent_clearEventFlag(arqEvent_dataRcvd);
			}
			else if (arqEvent_checkEventFlag(arqEvent_dataToSend)) //no ACK, data to send
			{
				//msg header setting <---------------------------------------------------------------------------------------------------------------------
				uint8_t crc = 0; 						
				const char* s1 = "Hello";                //고정된 메시지     
				strcpy((char *)originalWord, s1);        //송신될 패킷에 입력    
				originalWord[strlen(s1)] = '\0';	     //null 부착     			
				wordLen = strlen(s1);                    //길이 계산  

				crc = calculate_crc8((uint8_t *)originalWord, wordLen);       //crc 만들기  
				//printf("CRC-8 값: %x\n", crc);                                 //확인용 출력, 실제모듈에선 주석처리할것

				//crc 붙이기
                originalWord[wordLen++] = crc;
                originalWord[wordLen] = '\0';

				//fec 인코딩
                uint8_t output_len = 0;                                          //fec 인코딩이 끝나고 나서의 길이
                output_len = encode_fec((uint8_t *)originalWord, wordLen);       //fec 만들기

				//fec 인코딩 후 확인용 출력, 실제모듈에선 주석처리할것
                /*printf("\nCRC, FEC 이후 결과값: ");
                for(int i = 0; i< output_len; i++){
                    printf("%2x", output_bits[i]);
                }
                printf("\n");
                pc.printf("길이: %d\n", output_len);*/

				pduSize = arqMsg_encodeData(arqPdu, output_bits, seqNum++, output_len);    //originalWord-> output_bits, wordLen->output_len으로 변경(fec를 통해 길이가 바뀌었음으로)
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
			    if (TotalAckRcvd < 105)  //<--- 2024.04.01 추가	TxCnt < 100에서 TotalAckRcvd < 105로 (ack을 100번 다 받을때까지를 조건으로 설정했음, 앞의 5번은 테스트 용)
				{
					arqEvent_setEventFlag(arqEvent_dataToSend);
					TxCnt++;
					if (TotalAckRcvd == 5) 										//100회 전송에 걸린 총 시간(totalACKRcvd가 5일때 타이머 켜서 105회일때 끄기)
					{
					timers.start();												//시간 측정 시작
					}
				}				
				else if (TotalAckRcvd == 105){									//TX가 105회 ack을 다 받아 전송이 끝나면 
					timers.stop();												//시간 측정 끝
					seconds = timers.read_ms();									//밀리초 반환
    				
					pc.printf("Execution time: %ld milli secomds\n", seconds);	//측정한 시간 출력, 밀리초
					pc.printf("Total Send Packet: %d\n", TotalTxSend);			//재전송까지 고려한 총 보낸 패킷 갯수 출력
					pc.printf("Total Rcvd ACK: %d\n", TotalAckRcvd);			//받은 ACK 개수 출력 (105개 받을때까지 해서 105개로 값이 고정이긴함)
					pc.printf("power: %d, DR: %d\n", getPHYMAC_PARAM_PWR(), getPHYMAC_PARAM_DR());	//TX 관련 파라미터 보여주기		
				}
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