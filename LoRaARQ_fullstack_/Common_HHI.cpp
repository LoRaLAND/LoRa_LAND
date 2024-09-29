#include "sx1272Regs-LoRa.h"
#include "sx1272Regs-FSK.h"

#include "Common_HHI.h"
#define REG_MODEM_STAT 0x18
#define MASK_REG_RX_STAT 0x04
typedef struct
{
    ModemType   Modem;
    uint8_t     Addr;
    uint8_t     Value;
}RadioRegisters_t;


/*
SX1272( events, D11, D12, D13, D10, A0, //spi
				D2, D3, D4, D5, D8, D9 ), //dio0~5
*/
InterruptIn dio0(D2);
InterruptIn dio1(D3);

SPI hw_spi(D11, D12, D13);
DigitalOut hw_nss(D10);
DigitalInOut hw_reset(A0);
DigitalInOut hw_AntSwitch(A4);

#if 0
static const FskBandwidth_t FskBandwidths[] =
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
//Bandwidth configuration
#endif


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

static const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;
size_t sizeOfRadioRegsInit=sizeof( RadioRegsInit );

bool isRadioActive;








//inner function prototyping

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








uint8_t HW_init_radio(const HWStatus_t* hwabs)
{
	uint8_t boardType;
	isRadioActive = false;
	
	HW_reset();
	boardType = HW_DetectBoardType(hwabs);
	
	HW_AntSwInit( );
	HW_SpiInit();
	HW_SetOpMode( RF_OPMODE_SLEEP );

	//interrupt handler function
	dio0.rise(&HAL_hwDone);
	dio1.rise(&HAL_hwTimeout);

	HW_RadioRegistersInit( );

	HW_SetModem( hwabs->settings.Modem );
	HW_SetPublicNetwork( hwabs->settings.LoRa.PublicNetwork );
	
	
	// verify the connection with the board
	while( HW_SpiRead( REG_VERSION ) == 0x00  )
	{
		debug( "  - Radio could not be detected!\n");
		wait( 1 );
	}

	HW_SetChannel( hwabs->settings.Channel ); 

	return boardType;

}


void HW_Standby(void)
{
	HW_SetOpMode( RF_OPMODE_STANDBY );
}

void HW_Sleep(void)
{
    HW_SetOpMode( RF_OPMODE_SLEEP );
}


void HW_SetModem( RadioModems_t modem )
{
	RadioModems_t current_modem;

	if( ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) != 0 ) 
    {
        current_modem = MODEM_LORA;
    }
    else
    {
        current_modem = MODEM_FSK;
    }

    if( current_modem != modem )
    {
	    //HWStatus.settings.Modem = modem;
	    switch( modem )
	    {
		    default:
		    case MODEM_FSK:
		        HAL_Sleep( );
		        HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );

		        HW_SpiWrite( REG_DIOMAPPING1, 0x00 );
		        HW_SpiWrite( REG_DIOMAPPING2, 0x30 );
		        break;
		    case MODEM_LORA:
		        HAL_Sleep( );
		        HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

		        HW_SpiWrite( REG_DIOMAPPING1, 0x00 );
		        HW_SpiWrite( REG_DIOMAPPING2, 0x00 );
		        break;
	    }
    }
}


void HW_SetPublicNetwork( bool enable )
{
    HW_SetModem( MODEM_LORA );
    if( enable == true )
    {
        // Change LoRa modem SyncWord
        HW_SpiWrite( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD );
    }
    else
    {
        // Change LoRa modem SyncWord
        HW_SpiWrite( REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD );
    }
}


uint8_t HW_getModemStatusRx(){
    uint8_t rxStat;
    rxStat=HW_SpiRead(REG_MODEM_STAT);
    if ((rxStat&MASK_REG_RX_STAT) == 4)
        return 1;
    else
        return 0;

}







//SPI functions
static void HW_SpiInit( void )
{
    hw_nss = 1;
    hw_spi.format( 8,0 );   
    //uint32_t frequencyToSet = 8000000;
	
    wait(0.1); 
}

static void HW_SpiWriteAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    hw_nss = 0;
    hw_spi.write( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        hw_spi.write( buffer[i] );
    }
    hw_nss = 1;
}

static void HW_SpiReadAddrDataSize( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    hw_nss = 0;
    hw_spi.write( addr & 0x7F );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = hw_spi.write( 0 );
    }
    hw_nss = 1;
}

void HW_SpiWrite( uint8_t addr, uint8_t data )
{
    HW_SpiWriteAddrDataSize( addr, &data, 1 );
}

uint8_t HW_SpiRead( uint8_t addr )
{
    uint8_t data;
	
    HW_SpiReadAddrDataSize( addr, &data, 1 );
	
    return data;
}

void HW_SpiWriteFifo( uint8_t *buffer, uint8_t size )
{
    HW_SpiWriteAddrDataSize( 0, buffer, size );
}

void HW_SpiReadFifo( uint8_t *buffer, uint8_t size )
{
    HW_SpiReadAddrDataSize( 0, buffer, size );
}







//register map initialization
static void HW_RadioRegistersInit( ) 
{
    uint8_t i = 0;
    for( i = 0; i < sizeOfRadioRegsInit / sizeof( RadioRegisters_t ); i++ )
    {
        HW_SetModem( RadioRegsInit[i].Modem );
        HW_SpiWrite( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }    
}





//RF HW control functions
void HW_SetChannel( uint32_t freq )
{
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    HW_SpiWrite( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    HW_SpiWrite( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    HW_SpiWrite( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

void HW_SetOpMode( uint8_t opMode )
{	
    if( opMode == RF_OPMODE_SLEEP )
    {
        HW_SetAntSwLowPower( true );
    }
    else
    {
        HW_SetAntSwLowPower( false );
        HW_SetAntSw( opMode );
    }
    HW_SpiWrite( REG_OPMODE, ( HW_SpiRead( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
}





#if 0
static uint8_t HW_GetFskBandwidthRegValue( uint32_t bandwidth )

{
    uint8_t i;
    
    for( i = 0; i < ( sizeOfFskBandwidths/ sizeof( FskBandwidth_t ) ) - 1; i++ )
    {
        if( ( bandwidth >= FskBandwidths[i].bandwidth ) && ( bandwidth < FskBandwidths[i + 1].bandwidth ) )
        {
            return FskBandwidths[i].RegValue;
        }
    }
    // ERROR: Value not found
    while( 1 );
}
#endif

//HW reset
static void HW_reset(void)
{
	hw_reset.output( );
	hw_reset = 0;
	wait_ms( 1 );
	hw_reset.input( );
	wait_ms( 6 );
}


static uint8_t HW_DetectBoardType( const HWStatus_t* hwabs )
{
	uint8_t res = hwabs->boardConnected;
	
    if( hwabs->boardConnected == UNKNOWN )
    {
        hw_AntSwitch.input( );
        wait_ms( 1 );
        if( hw_AntSwitch == 1 )
        {
            res = SX1272MB1DCS;
        }
        else
        {
            res = SX1272MB2XAS;
        }
        hw_AntSwitch.output( );
        wait_ms( 1 );
    }
    return ( res );
}


//Antenna-related functions
static void HW_AntSwInit( void )
{
    hw_AntSwitch = 0;
}

static void HW_AntSwDeInit( void )
{
    hw_AntSwitch = 0;
}

static void HW_SetAntSw( uint8_t opMode )
{
    switch( opMode )
    {
    case RFLR_OPMODE_TRANSMITTER:
        hw_AntSwitch = 1;
        break;
    case RFLR_OPMODE_RECEIVER:
    case RFLR_OPMODE_RECEIVER_SINGLE:
    case RFLR_OPMODE_CAD:
        hw_AntSwitch = 0;
        break;
    default:
        hw_AntSwitch = 0;
        break;
    }
}


static void HW_SetAntSwLowPower( bool status )
{
    if( isRadioActive != status )
    {
        isRadioActive = status;
    
        if( status == false )
        {
            HW_AntSwInit( );
        }
        else
        {
            HW_AntSwDeInit( );
        }
    }
}

