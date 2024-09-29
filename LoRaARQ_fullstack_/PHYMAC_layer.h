//파이맥 상태
#define PHYMAC_STATE_RX             0
#define PHYMAC_STATE_TX             1

//파이맥 에러 관련 상수
#define PHYMAC_ERR_NONE             0   //에러 없음
#define PHYMAC_ERR_WRONGSTATE       1   //상태가 틀린 에러
#define PHYMAC_ERR_HWERROR          2   //하드웨어 웨어
#define PHYMAC_ERR_SIZE             3   //에러 크기



int phymac_dataReq(uint8_t* dataPtr, uint8_t size, uint8_t destId);                                         //보낼 데이터 제작 함수
void phymac_init(uint8_t id, void (*dataCnfFunc)(int), void (*dataIndFunc)(uint8_t, uint8_t*, uint8_t));    //파이맥 초기화 함수

int getPHYMAC_PARAM_PWR(void);              //tx파워 가져오기
int getPHYMAC_PARAM_DR(void);               //DR 가져오기(SF도 볼 수 있음)

int16_t phymac_getDataRssi(void);                                                                           //dataRSSI 가져오기
int8_t phymac_getDataSnr(void);                                                                             //dataSNR 가져오기
int phymac_configSrcId(uint8_t id);                                                                         //rx에서만 source id 확인