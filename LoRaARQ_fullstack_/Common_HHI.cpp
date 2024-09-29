#include "sx1272Regs-LoRa.h"
#include "sx1272Regs-FSK.h"

#include "Common_HHI.h"
#define REG_MODEM_STAT 0x18         //레지스터 모뎀 스탯
#define MASK_REG_RX_STAT 0x04       //마스크 레지스터 rx 스탯
typedef struct                      //라디오 레지스터 구조체(모뎀, 주소, 값으로 구성)
{
    ModemType   Modem;
    uint8_t     Addr;
    uint8_t     Value;
}RadioRegisters_t;  


/*
SX1272( events, D11, D12, D13, D10, A0, //spi => 시리얼 통신?
				D2, D3, D4, D5, D8, D9 ), //dio0~5 (dio=디지털 인풋 아웃풋?)
*/
//이 부분은 잘 모르겠음
InterruptIn dio0(D2);                       //인터럽트 인 객체 생성? (핀 D2, D3)
InterruptIn dio1(D3);

SPI hw_spi(D11, D12, D13);                  //D11, D12, D13 끼리 시리얼 통신?
DigitalOut hw_nss(D10);                     //디지털 아웃(디지털 출력) 객체 생성? (디지털 출력핀 D10)
DigitalInOut hw_reset(A0);                  //디지털 인아웃(디지털 입출력) 객체 생성? (핀 A0)
DigitalInOut hw_AntSwitch(A4);              //(핀 A4)

#if 0
static const FskBandwidth_t FskBandwidths[] =               //FSK 대역폭들?
{
    { 2600  , 0x17 },
    { 3100  , 0x0F },
    { 3900  , 0x07 },
    { 5200  , 0x16 },
    { 6300  , 0x0E },
    { 7800  , 0x06 },
    { 10400 , 0x15 },
    { 12500 , 0x0D },
    { 15600 , 0x05 },
    { 20800 , 0x14 },
    { 25000 , 0x0C },
    { 31300 , 0x04 },
    { 41700 , 0x13 },
    { 50000 , 0x0B },
    { 62500 , 0x03 },
    { 83333 , 0x12 },
    { 100000, 0x0A },
    { 125000, 0x02 },
    { 166700, 0x11 },
    { 200000, 0x09 },
    { 250000, 0x01 },
    { 300000, 0x00 }, // Invalid Bandwidth
};
size_t sizeOfFskBandwidths=sizeof( FskBandwidths );
//Bandwidth configuration (FSK 대역폭 구성)
#endif

//라디오 초기 레지스터 값들
#define RADIO_INIT_REGISTERS_VALUE                \
{                                                 \
{ MODEM_FSK , REG_LNA                , 0x23 },\
{ MODEM_FSK , REG_RXCONFIG           , 0x1E },\
{ MODEM_FSK , REG_RSSICONFIG         , 0xD2 },\
{ MODEM_FSK , REG_AFCFEI             , 0x01 },\
{ MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },\
{ MODEM_FSK , REG_OSC                , 0x07 },\
{ MODEM_FSK , REG_SYNCCONFIG         , 0x12 },\
{ MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },\
{ MODEM_FSK , REG_SYNCVALUE2         , 0x94 },\
{ MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },\
{ MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },\
{ MODEM_FSK , REG_FIFOTHRESH         , 0x8F },\
{ MODEM_FSK , REG_IMAGECAL           , 0x02 },\
{ MODEM_FSK , REG_DIOMAPPING1        , 0x00 },\
{ MODEM_FSK , REG_DIOMAPPING2        , 0x30 },\
{ MODEM_LORA, REG_LR_DETECTOPTIMIZE  , 0x43 },\
{ MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },\
}

static const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE; //라디오 레지스터에 초기값 저장
size_t sizeOfRadioRegsInit=sizeof( RadioRegsInit );                         //크기 계산

bool isRadioActive;                                                         //라디오 활성 여부








//inner function prototyping (내부 함수 선언)

static uint8_t HW_DetectBoardType( const HWStatus_t* hwabs );
static void HW_AntSwInit( void );
static void HW_AntSwDeInit( void );
static void HW_SetAntSw( uint8_t opMode );
static void HW_SetAntSwLowPower( bool status );


static void HW_SpiInit( void );
static void HW_SpiWriteAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size );
static void HW_SpiReadAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size );
void HW_SpiWriteFifo( uint8_t *buffer, uint8_t size );
void HW_SpiReadFifo( uint8_t *buffer, uint8_t size );

static void HW_reset(void);
static void HW_RadioRegistersInit( ) ;







//하드웨어 구동 초기화 함수?
uint8_t HW_init_radio(const HWStatus_t* hwabs)
{
	uint8_t boardType;
	isRadioActive = false;                      //라디오 꺼져있음
	
	HW_reset();                                 //하드위어 리셋
	boardType = HW_DetectBoardType(hwabs);      //입력 받은 값으로 보드 타입 결정
	
	HW_AntSwInit( );                            //AntSw(안테나 스위치) 초기화
	HW_SpiInit();                               //spi 초기화
	HW_SetOpMode( RF_OPMODE_SLEEP );            //OPMODE를 SLEEP으로 설정 (OPMODE는 속도와 전류를 조절하는 모드)

	//interrupt handler function    (인터럽트 핸들러 함수)
	dio0.rise(&HAL_hwDone);
	dio1.rise(&HAL_hwTimeout);

	HW_RadioRegistersInit( );                   //레지스터 초기화

	HW_SetModem( hwabs->settings.Modem );       //모뎀 설정
	HW_SetPublicNetwork( hwabs->settings.LoRa.PublicNetwork );  //퍼블릭 네트워크 설정
	
	
	// verify the connection with the board     (보드 연결 확인)
	while( HW_SpiRead( REG_VERSION ) == 0x00  )
	{
		debug( "  - Radio could not be detected!\n");
		wait( 1 );
	}

	HW_SetChannel( hwabs->settings.Channel );   //채널 설정

	return boardType;                           //보드 타입 반환

}

//opmode를 stanby로 바꾸는 함수
void HW_Standby(void)
{
	HW_SetOpMode( RF_OPMODE_STANDBY );
}

//opmode를 sleep으로 바꾸는 함수
void HW_Sleep(void)
{
    HW_SetOpMode( RF_OPMODE_SLEEP );
}

//modem 설정 함수?
void HW_SetModem( RadioModems_t modem )
{
	RadioModems_t current_modem;        //현재 모뎀

	if( ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) != 0 )  //REG_OPMODE를 읽어서 RFLR_OPMODE_LONGRANGEMODE_ON과 AND 연산한 값이 0이 아니라면(이게 뭘 의미하는거지)
    {
        current_modem = MODEM_LORA;                                         //현재 모뎀을 로라로
    }
    else
    {
        current_modem = MODEM_FSK;                                          //그렇지 않다면 FSK로
    }

    if( current_modem != modem )        //만약 입력값이 현재 모뎀과 다르다면
    {
	    //HWStatus.settings.Modem = modem;
	    switch( modem )             //?스위치문 이해가 부족함
	    {
		    default:
		    case MODEM_FSK:
		        HAL_Sleep( );
		        HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );   //<-뭘 의미하는거지  

		        HW_SpiWrite( REG_DIOMAPPING1, 0x00 );       //REG_DIOMAPPING1에 들어가 있던 초기값
		        HW_SpiWrite( REG_DIOMAPPING2, 0x30 );       //REG_DIOMAPPING2에 들어가 있던 초기값
		        break;
		    case MODEM_LORA:    //LORA일 경우 FSK와 다르게 REG_DIOMAPPING2에 0x00를 넣음
		        HAL_Sleep( );
		        HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

		        HW_SpiWrite( REG_DIOMAPPING1, 0x00 );
		        HW_SpiWrite( REG_DIOMAPPING2, 0x00 );
		        break;
	    }
    }
}

//하드웨어 퍼블릭 네트워크 설정
void HW_SetPublicNetwork( bool enable )
{
    HW_SetModem( MODEM_LORA );      //모뎀 로라로 설정
    if( enable == true )
    {
        // Change LoRa modem SyncWord (LoRa 모뎀 싱크 워드 변경)
        HW_SpiWrite( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD );
    }
    else
    {
        // Change LoRa modem SyncWord
        HW_SpiWrite( REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD );
    }
}

//모뎀 스테이터스 가져오기
uint8_t HW_getModemStatusRx(){
    uint8_t rxStat;
    rxStat=HW_SpiRead(REG_MODEM_STAT);  //모뎀 스탯 읽어오기
    if ((rxStat&MASK_REG_RX_STAT) == 4) //rxStat이 4라면?
        return 1;       //1 반환
    else
        return 0;       //아니면 0 반환

}







//SPI functions(SPI 함수)
static void HW_SpiInit( void )      //SPI 초기화
{
    hw_nss = 1;                     //디지털 출력 레지스터 값 1로
    hw_spi.format( 8,0 );           //format함수 어떻게 작동하지?
    //uint32_t frequencyToSet = 8000000;
	
    wait(0.1); 
}

//SPI쓰기 함수(addr에 버퍼 내용을 size만큼?)
static void HW_SpiWriteAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    hw_nss = 0;                     //디지털 출력 레지스터 값 0으로
    hw_spi.write( addr | 0x80 );    //addr의 맨 앞자리만 1로 만들어서 write
    for( i = 0; i < size; i++ )
    {
        hw_spi.write( buffer[i] );
    }
    hw_nss = 1;                     //작업 끝나면 1로 바꿔주기
}

//SPI읽기 함수(addr에 버퍼 내용을 size만큼?)
static void HW_SpiReadAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    hw_nss = 0;                     //디지털 출력 레지스터 값 0으로
    hw_spi.write( addr & 0x7F );    //addr
    for( i = 0; i < size; i++ )
    {
        buffer[i] = hw_spi.write( 0 );
    }
    hw_nss = 1;                     //작업 끝나면 1로 바꿔주기
}

//spi 쓰기
void HW_SpiWrite( uint8_t addr, uint8_t data )
{
    HW_SpiWriteAddrDataSize( addr, &data, 1 );
}

//spi 읽기
uint8_t HW_SpiRead( uint8_t addr )  
{
    uint8_t data;
	
    HW_SpiReadAddrDataSize( addr, &data, 1 );
	
    return data;
}

//spi 선입선출로 쓰기
void HW_SpiWriteFifo( uint8_t *buffer, uint8_t size )
{
    HW_SpiWriteAddrDataSize( 0, buffer, size );
}

//spi 선입선출로 읽기
void HW_SpiReadFifo( uint8_t *buffer, uint8_t size )
{
    HW_SpiReadAddrDataSize( 0, buffer, size );
}







//register map initialization (레지스터 맵 초기화)
static void HW_RadioRegistersInit( ) 
{
    uint8_t i = 0;
    for( i = 0; i < sizeOfRadioRegsInit / sizeof( RadioRegisters_t ); i++ ) //레지스터 개수 만큼 반복
    {
        HW_SetModem( RadioRegsInit[i].Modem );                              //각 레지스터의 모뎀 설정
        HW_SpiWrite( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );       //각 레지스터의 주소에 값 쓰기
    }    
}





//RF HW control functions (하드웨어 제어 함수들)
void HW_SetChannel( uint32_t freq )     //하드웨어 채널 설정
{
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );        //프리퀀시 계산
    HW_SpiWrite( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );    //freq의 가장 왼쪽 8비트만 가져와서 MSB에 저장
    HW_SpiWrite( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );     //freq의 가운데 8비트만 가져와서 MID에 저장
    HW_SpiWrite( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );              //freq의 가장 오른쪽 8비트만 가져와서 LSB에 저장
}

//OPMODE 설정 (OPMODE는 시스템이나 장치가 작동하는 특정 상태나 모드를 나타냄)
void HW_SetOpMode( uint8_t opMode )
{	
    if( opMode == RF_OPMODE_SLEEP )         //OPMODE가 sleep이면 안테나 스위치 저전력 활성화 
    {
        HW_SetAntSwLowPower( true );
    }
    else                                    //OPMODE가 sleep이 아니라면 안테나 스위치 저전력 비활성화, 안테나스위치를 OPMODE에 맞게 설정 
    {
        HW_SetAntSwLowPower( false );
        HW_SetAntSw( opMode );
    }
    HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );  //REG_OPMODE를 읽어서 RF_OPMODE_MASK와 AND 연산 후 opMode와 OR 연산 한 값을 REG_OPMODE에 저장? => 비트 연산?
}





#if 0
static uint8_t HW_GetFskBandwidthRegValue( uint32_t bandwidth ) //FSK 대역폭 레지스터값 가져오기

{
    uint8_t i;
    
    for( i = 0; i < ( sizeOfFskBandwidths/ sizeof( FskBandwidth_t ) ) - 1; i++ )    //(대역폭 개수-1) 만큼 반복
    {
        if( ( bandwidth >= FskBandwidths[i].bandwidth ) && ( bandwidth < FskBandwidths[i + 1].bandwidth ) ) //입력 값인 bandwidth이 FskBandwidths의 i번째 주파수 대역 사이에 해당되면
        {
            return FskBandwidths[i].RegValue;                                                               //그 때의 레지스터 값을 반환
        }
    }
    // ERROR: Value not found   //값을 찾지 못하면 오류(무한 반복으로 대기)
    while( 1 );
}
#endif

//HW reset  (하드웨어 리셋)
static void HW_reset(void)
{
	hw_reset.output( ); 
	hw_reset = 0;       //핀 A0값을 0으로?
	wait_ms( 1 );
	hw_reset.input( );
	wait_ms( 6 );
}

//보트 타입 결정하는 함수
static uint8_t HW_DetectBoardType( const HWStatus_t* hwabs )
{
	uint8_t res = hwabs->boardConnected;    
	
    if( hwabs->boardConnected == UNKNOWN )  //보드연결 여부가 UNKNOWN이면
    {
        hw_AntSwitch.input( );              //입력으로 설정?
        wait_ms( 1 );                       //1초 대기
        if( hw_AntSwitch == 1 )             //hw_AntSwitch값이 1이면
        {
            res = SX1272MB1DCS;             //보드 타입을 SX1272MB1DCS로
        }
        else                                //1이 아니면
        {   
            res = SX1272MB2XAS;             //보드 타입을 SX1272MB2XAS로
        }
        hw_AntSwitch.output( );             //출력으로 설정?
        wait_ms( 1 );
    }
    return ( res );                         //설정한 보드 타입 출력
}


//Antenna-related functions (안테나 관련 함수들)
static void HW_AntSwInit( void )            //안테나 초기화
{
    hw_AntSwitch = 0;
}

static void HW_AntSwDeInit( void )          //저전력일때 안테나 초기화
{
    hw_AntSwitch = 0;
}

//OPMODE에 맞게 안테나 스위치 설정
static void HW_SetAntSw( uint8_t opMode )
{
    switch( opMode )
    {
    case RFLR_OPMODE_TRANSMITTER:           //TRANSMITTER면 스위치 값을 1로
        hw_AntSwitch = 1;
        break;
    case RFLR_OPMODE_RECEIVER:
    case RFLR_OPMODE_RECEIVER_SINGLE:
    case RFLR_OPMODE_CAD:                   //RECEIVER, RECEIVER싱글, CAD의 경우 스위치 값을 0으로
        hw_AntSwitch = 0;
        break;
    default:                                //디폴트 값 0
        hw_AntSwitch = 0;
        break;
    }
}

//안테나 low power 설정 함수
static void HW_SetAntSwLowPower( bool status )
{
    if( isRadioActive != status )   //라디오 활성이 status와 다르다면 같게 설정
    {
        isRadioActive = status;
    
        if( status == false )   //안테나 low power를 끄면
        {
            HW_AntSwInit( );    //안테나 스위치 초기화
        }
        else
        {
            HW_AntSwDeInit( );  //안테나 스위치 초기화(저전력으로)
        }
    }
}

