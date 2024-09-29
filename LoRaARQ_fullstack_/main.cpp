#include "mbed.h"
#include "string.h"
#include "PHYMAC_layer.h"
#include "ARQ_FSMevent.h"
#include "ARQ_msg.h"
#include "ARQ_parameters.h"


#define DBGMSG_ARQ          0 //debug print control

//ARQ FSM state -------------------------------------------------
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
uint8_t endNode_ID=1;
uint8_t dest_ID=0;

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
uint8_t wordLen=0;

//ARQ reception event related variables
uint8_t dataSrc; //source ID
uint8_t dataSeq; //sequence number
uint8_t arqAck[5]; //ARQ ACK message





//Event handler ------------------------------------------------
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
        
        pc.printf("\n -------------------------------------------------\nRCVD from %i : %s (length:%i)\n -------------------------------------------------\n", srcId, arqMsg_getWord(dataPtr), size);

        arqEvent_setEventFlag(arqEvent_dataRcvd);
    }
    else if (arqMsg_checkIfAck(dataPtr))
    {
        pc.printf("\n -------------------------------------------------\n[ARQ] ACK is received (%i)\n -------------------------------------------------\n", arqMsg_getSeq(dataPtr));

        arqEvent_setEventFlag(arqEvent_ackRcvd);
    }
}



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
            if (wordLen >= ARQMSG_MAXDATASIZE-1)
            {
                originalWord[wordLen++] = '\0';
                arqEvent_setEventFlag(arqEvent_dataToSend);
                pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
            }
        }
    }
    
}



//timer related functions ---------------------------
void arqMain_startTimer()
{
    uint8_t waitTime = ARQ_MINWAITTIME + rand()%(ARQ_MAXWAITTIME-ARQ_MINWAITTIME);
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
int main(void){
    uint8_t flag_needPrint=1;
    uint8_t prev_state = 0;

    //initialization
    pc.printf("------------------ ARQ protocol starts! --------------------------\n");
    arqEvent_clearAllEventFlag();
    
    //source & destination ID setting
    pc.printf(":: ID for this node : ");
    pc.scanf("%d", &endNode_ID);
    pc.printf(":: ID for the destination : ");
    pc.scanf("%d", &dest_ID);
    pc.getc();

    pc.printf("endnode : %i, dest : %i\n", endNode_ID, dest_ID);

    phymac_init(endNode_ID, arqMain_dataCnfFunc, arqMain_dataIndFunc);
    pc.attach(&arqMain_processInputWord, Serial::RxIrq);

    while(1)
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

                    main_state = MAINSTATE_TX;
                    flag_needPrint = 1;

                    arqEvent_clearEventFlag(arqEvent_dataRcvd);
                }
                else if (arqEvent_checkEventFlag(arqEvent_dataToSend)) //no ACK, data to send
                {
                    //msg header setting
                    pduSize = arqMsg_encodeData(arqPdu, originalWord, seqNum++, wordLen);
                    arqMain_sendData(arqPdu, pduSize, dest_ID);
                    retxCnt = 0;

                    pc.printf("[ARQ] sending to %i (seq:%i)\n", dest_ID, seqNum-1);

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
                {
                    pc.printf("Give a word to send : ");
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

            default :
                break;
        }
    }
}