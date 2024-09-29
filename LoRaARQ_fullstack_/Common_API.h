#ifndef COMMON_API_H
#define COMMON_API_H

#include "PHYHAL_interface.h"

#define HAL_INIT_RF_FREQUENCY			922100000  //주파수
#define HAL_INIT_MODEM					MODEM_LORA  //모뎀종류
#define HAL_INIT_LORA_FIXED_LENGTH		false      //로라 고정 길이(false=>고정 아님, 가변)
#define HAL_INIT_LORA_FREQHOP			false       //주파수 호핑(=주파수 도약, 주파수를 바꾸어가며 전송하는 기술)
#define HAL_INIT_LORA_HOPPERIOD			4           //호핑 주기
#define HAL_INIT_LORA_CRCON				true        //CRC작동여부
#define HAL_INIT_LORA_IQINV				false       //IQ변조?(IQ 인버터?)
#define HAL_INIT_LORA_TXTIMEOUT			2000        //tx timeout 설정값
#define HAL_INIT_LORA_TXPOWER			14          //tx 파워값(전송세기)
#define HAL_INIT_LORA_BANDWIDTH			HAL_LRBW_125		//LRparam_bw_e (대역폭)
#define HAL_INIT_LORA_DATARATE			HAL_LRDatarate_SF7  //LRparam_dr_e (데이터 레이트, 스프레딩팩터 SF7)
#define HAL_INIT_LORA_CODERATE			HAL_LRCoderate_4_5	//LRparam_cr_e (코드 레이트)
#define HAL_INIT_LORA_PREAMBLELEN		8				//should be fixed for public network LoRaWAN  (프리엠블, 프레임 동기를 위해 프레임 단위 별로 각 프레임의 맨 앞에 붙이는 영역)
#define HAL_INIT_LORA_RXSYMBTIMEOUT		5               //RX 심볼 타임아웃(심볼=통신에서 변조,코딩,전송,검출을 하는 의미있는 단위)
#define HAL_INIT_LORA_PAYLOADLEN		0               //페이로드(전송되는 순수한 데이터, 즉 헤더나 메타데이터를 제외한 데이터를 의미) 길이
#define HAL_INIT_LORA_RXCONTINUOUS		true            //RX지속여부
#define HAL_INIT_LORA_PUBLIC_NETWORK	true			//should be fixed for public network LoRaWAN  (로라 퍼블릭 네트워크)

#endif


//HAL(하드웨어 추상화 계층) => 하드웨어를 조작할 수 있게끔 하는 소프트웨어들의 모임