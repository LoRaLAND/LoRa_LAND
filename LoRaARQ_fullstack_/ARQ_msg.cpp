#include "mbed.h"
#include "ARQ_msg.h"

int arqMsg_checkIfData(uint8_t* msg)
{
    return (msg[ARQMSG_OFFSET_TYPE] == ARQMSG_TYPE_DATA);
}

int arqMsg_checkIfAck(uint8_t* msg)
{
    return (msg[ARQMSG_OFFSET_TYPE] == ARQMSG_TYPE_ACK);
}

int arqMsg_checkIfNAck(uint8_t* msg)                            //새로 추가함
{
    return (msg[ARQMSG_OFFSET_TYPE] == ARQMSG_TYPE_NACK);
}

uint8_t arqMsg_encodeAck(uint8_t* msg_ack, uint8_t seq)
{
    msg_ack[ARQMSG_OFFSET_TYPE] = ARQMSG_TYPE_ACK;            //메인함수에서 건드리므로 주석처리함
    msg_ack[ARQMSG_OFFSET_SEQ] = seq;
    msg_ack[ARQMSG_OFFSET_DATA] = 1;

    return ARQMSG_ACKSIZE;
}

uint8_t arqMsg_encodeData(uint8_t* msg_data, uint8_t* data, int seq, int len)
{
    msg_data[ARQMSG_OFFSET_TYPE] = ARQMSG_TYPE_DATA;
    msg_data[ARQMSG_OFFSET_SEQ] = seq;
    memcpy(&msg_data[ARQMSG_OFFSET_DATA], data, len*sizeof(uint8_t));

    return len+ARQMSG_OFFSET_DATA;
}
                    

uint8_t arqMsg_getSeq(uint8_t* msg)
{
    return msg[ARQMSG_OFFSET_SEQ];
}

uint8_t* arqMsg_getWord(uint8_t* msg)
{
    return &msg[ARQMSG_OFFSET_DATA];
}