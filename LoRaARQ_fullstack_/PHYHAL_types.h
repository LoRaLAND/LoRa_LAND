#ifndef PHYHAL_TYPES_H
#define PHYHAL_TYPES_H

typedef enum 
{
    HAL_LRBW_125 = 0,
    HAL_LRBW_250 = 1,
    HAL_LRBW_500 = 2
} HALenum_bw_e;

typedef enum 
{
    HAL_LRDatarate_SF7 = 7,
	HAL_LRDatarate_SF8 = 8,
	HAL_LRDatarate_SF9 = 9,
	HAL_LRDatarate_SF10 = 10,
	HAL_LRDatarate_SF11 = 11,
	HAL_LRDatarate_SF12 = 12
} HALenum_dr_e;

typedef enum 
{
    HAL_LRCoderate_4_5 = 1,
	HAL_LRCoderate_4_6 = 2,
	HAL_LRCoderate_4_7 = 3,
	HAL_LRCoderate_4_8 = 4
} HALenum_cr_e;

typedef enum{
    HAL_RX_NO_ERR,
    HAL_RX_BANDWIDTH_CFG_ERR,
    HAL_RX_DATARATE_CFG_ERR,
    HAL_RX_CODERATE_CFG_ERR,
    HAL_RX_BANDWIDTH_AFC_CFG_ERR,
    HAL_RX_PREAMBLE_LEN_CFG_ERR,
    HAL_RX_RXTIME_CFG_ERR
}HALtype_rxCfgErr;

typedef enum {
    HAL_TX_NO_ERR,
    HAL_TX_POWER_CFG_ERR,
    HAL_TX_FDEV_CFG_ERR,
    HAL_TX_BANDWIDTH_CFG_ERR,
    HAL_TX_DATARATE_CFG_ERR,
    HAL_TX_CODERATE_CFG_ERR,
    HAL_TX_PREAMBLE_LEN_CFG_ERR
}HALtype_txCfgErr;


#endif
