#ifndef PHYHAL_TYPES_H
#define PHYHAL_TYPES_H

typedef enum 
{
    HAL_LRBW_125 = 0,
    HAL_LRBW_250 = 1,
    HAL_LRBW_500 = 2
} HALenum_bw_e;     //대역폭 설정을 위한 구조체

typedef enum 
{
    HAL_LRDatarate_SF7 = 7,
	HAL_LRDatarate_SF8 = 8,
	HAL_LRDatarate_SF9 = 9,
	HAL_LRDatarate_SF10 = 10,
	HAL_LRDatarate_SF11 = 11,
	HAL_LRDatarate_SF12 = 12
} HALenum_dr_e;                     //sf(스프레딩 팩터) 설정을 위한 구조체? (SF는 낮으면 전송량이 늘어나지만 거리가 짧아지는 특징이 있음)

typedef enum 
{
    HAL_LRCoderate_4_5 = 1,
	HAL_LRCoderate_4_6 = 2,
	HAL_LRCoderate_4_7 = 3,
	HAL_LRCoderate_4_8 = 4
} HALenum_cr_e;     //코드 레이트 설정을 위한 구조체 (코드 레이트란 부호화율, 부호화시 실제 정보 비트가 어느 만큼 포함될 수 있는 정도를 의미. 값이 늘어나면 전송시간이 지연됨)

typedef enum{
    HAL_RX_NO_ERR,                  //에러 없음
    HAL_RX_BANDWIDTH_CFG_ERR,       //대역폭 에러
    HAL_RX_DATARATE_CFG_ERR,        //데이터레이트 에러
    HAL_RX_CODERATE_CFG_ERR,        //코드레이트 에러
    HAL_RX_BANDWIDTH_AFC_CFG_ERR,   //BANDWIDTH_AFC? 위에 대역폭 에러랑 무슨 차이?
    HAL_RX_PREAMBLE_LEN_CFG_ERR,    //프리앰블 길이 에러
    HAL_RX_RXTIME_CFG_ERR           //rxtime 에러
}HALtype_rxCfgErr;  //rx 에러 관련 구조체

typedef enum {
    HAL_TX_NO_ERR,
    HAL_TX_POWER_CFG_ERR,           //전송 세기 에러
    HAL_TX_FDEV_CFG_ERR,            //FDEV(formatted device) 에러, 입출력 스트림쪽 에러?
    HAL_TX_BANDWIDTH_CFG_ERR,
    HAL_TX_DATARATE_CFG_ERR,
    HAL_TX_CODERATE_CFG_ERR,
    HAL_TX_PREAMBLE_LEN_CFG_ERR
}HALtype_txCfgErr;  //tx 에러 관련 구조체


#endif
