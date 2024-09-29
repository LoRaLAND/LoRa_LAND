#ifndef COMMON_API_H
#define COMMON_API_H

#include "PHYHAL_interface.h"

#define HAL_INIT_RF_FREQUENCY			922100000
#define HAL_INIT_MODEM					MODEM_LORA
#define HAL_INIT_LORA_FIXED_LENGTH		false
#define HAL_INIT_LORA_FREQHOP			false
#define HAL_INIT_LORA_HOPPERIOD			4
#define HAL_INIT_LORA_CRCON				true
#define HAL_INIT_LORA_IQINV				false
#define HAL_INIT_LORA_TXTIMEOUT			2000
#define HAL_INIT_LORA_TXPOWER			14
#define HAL_INIT_LORA_BANDWIDTH			HAL_LRBW_125		//LRparam_bw_e
#define HAL_INIT_LORA_DATARATE			HAL_LRDatarate_SF7  //LRparam_dr_e
#define HAL_INIT_LORA_CODERATE			HAL_LRCoderate_4_5	//LRparam_cr_e
#define HAL_INIT_LORA_PREAMBLELEN		8				//should be fixed for public network LoRaWAN
#define HAL_INIT_LORA_RXSYMBTIMEOUT		5
#define HAL_INIT_LORA_PAYLOADLEN		0
#define HAL_INIT_LORA_RXCONTINUOUS		true
#define HAL_INIT_LORA_PUBLIC_NETWORK	true			//should be fixed for public network LoRaWAN

#endif
