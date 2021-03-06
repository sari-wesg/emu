#include "stm32l4xx.h"
#include <string.h>
#include <stdlib.h>
#include "hw.h"
#include "radio.h"
#include "timeServer.h"
#include "low_power_manager.h"
//#include "malloc.h" 
#include "sx1276.h"
#include "sx1276mb1mas.h"

#include "timer.h"
#include "fast_spi.h"
#include "rtc.h"
#include "delay.h"
#include "usart.h"
#include "Simulated_LoRa.h"
#include "LoRa_Channel_Coding.h"
#include "control_GPIO.h"

//#include "Timer_Calibration.h"
#include "Timer_Calibration_From_SX1276.h"

#include "energest.h"

//#define CODING

#define BUFFER_SIZE                                 255 // Define the payload size here
#define PACKET_COUNT																100	//
#define INTERVAL_TIME																1000  // ms


#define MODE  																			1 // #1 LOOK, LOOK run as normal lora
																											// #2 LOOK_BLANK, insert blank in symbols
																											// #3 LOOK_DOUBLE, transmit two packets in diffenert channels at the same time

#if (MODE==1) 																			
	#define LOOK
#elif (MODE==2)
	#define LOOK_BLANK
	#define LOOK_BLANK_RATIO														0.85  // This para means x% turn on PA and (1-x)% turn off PA.
#elif (MODE==3)
	#define LOOK_DOUBLE
#endif

#define ENABLE_USART

static RadioEvents_t RadioEvents;
																											
extern TIM_HandleTypeDef TIM2_Handler;
extern uint32_t time_count;
extern uint32_t Time_temp;

uint8_t data_1[21] = {0x80, 0x37, 0x00, 0x1e, 0x00, 0x00, 0x01, 0x00, 0x02, 0x79, 0x91, 0x05, 0x55, 0x29, 0x3b, 0x6b, 0xab, 0xd7, 0x47, 0xc0, 0x49};
//uint8_t data_2[21] = {0x80, 0x2c, 0x00, 0x42, 0x00, 0x00, 0x01, 0x00, 0x02, 0xbe, 0x52, 0xf5, 0x2c, 0xe3, 0xf6, 0xc0, 0xe4, 0x11, 0xd1, 0xb1, 0x6b};

//uint8_t data_2[235] = {0x40, 0x4E, 0x00, 0x1D, 0x00, 0x00, 0x02, 0x00, 0x02, 0xBC, 0x78, 0x8B, 0xD3, 0x19, 0x15, 0x85, 0xA6, 0x76, 0x89, 0xD2, 0xFE, 0xE6, 0xFA, 0x57, 0x7E, 0x96, 0x2C, 0x78, 0x74, 0xDF, 0x3B, 0xB3, 0x4A, 0xBA, 0xC5, 0xD4, 0x5C, 0xA2, 0x09, 0xBA, 0xBB, 0xE8, 0x1E, 0x20, 0x1B, 0x70, 0x40, 0x70, 0x78, 0xA9, 0x4B, 0xF9, 0xB2, 0x26, 0xD1, 0x59, 0x7F, 0x96, 0x79, 0x1A, 0x3E, 0xE3, 0xAF, 0xE7, 0x4D, 0x69, 0x8A, 0x16, 0x3C, 0xD5, 0xC8, 0x90, 0xE1, 0xDE, 0xCE, 0xAC, 0x9B, 0x08, 0x73, 0x62, 0xC0, 0xD6, 0xC2, 0x42, 0x9F, 0xCB, 0x9E, 0x3E, 0x63, 0x86, 0xD3, 0x6A, 0x92, 0x4C, 0x3E, 0x43, 0xA8, 0xE9, 0xBE, 0xAD, 0x66, 0x6C, 0x19, 0xED, 0x06, 0x0C, 0xD9, 0xC4, 0xC7, 0x45, 0x3C, 0x99, 0xEB, 0xFC, 0x07, 0x41, 0xF3, 0x70, 0xD7, 0x15, 0xB2, 0xD2, 0xD3, 0x01, 0xD4, 0x23, 0xAB, 0xE1, 0xE4, 0xD7, 0x80, 0x71, 0x27, 0x35, 0xF6, 0x29, 0x0E, 0x30, 0x3F, 0xEA, 0x0C, 0xB9, 0x8D, 0x4A, 0xAC, 0x4F, 0x13, 0xEB, 0xEC, 0x42, 0x65, 0x5B, 0x9F, 0x70, 0x99, 0x34, 0x07, 0x53, 0x65, 0x19, 0x04, 0xA1, 0x0E, 0x99, 0x2A, 0xB1, 0xB0, 0x39, 0xBE, 0x8F, 0xB7, 0x2D, 0x33, 0x61, 0x3A, 0x41, 0xC2, 0xB3, 0xC6, 0x07, 0x32, 0x17, 0x08, 0x07, 0x22, 0x9F, 0xB6, 0x5A, 0xC3, 0x04, 0x1E, 0xA2, 0x2E, 0xC9, 0xFE, 0xC9, 0x5D, 0x11, 0xD3, 0x11, 0xA5, 0x78, 0xE4, 0xC5, 0x17, 0x8E, 0xF5, 0xC9, 0x1C, 0xE1, 0x83, 0xE6, 0xB9, 0xA0, 0x94, 0x0C, 0x8A, 0xB5, 0x75, 0x89, 0xA7, 0x38, 0x04, 0xCB, 0x36, 0xB2, 0x95, 0xF6, 0xC7, 0x6D, 0x5F, 0x2F, 0xD0, 0xC5, 0x1F};


uint8_t Tx_Buffer[BUFFER_SIZE]={0x01,0x02};

uint16_t BufferSize = BUFFER_SIZE;

int main(void)
{
//	uint8_t data_0x001D004E[235]={
//0x40, 0x4E, 0x00, 0x1D, 0x00, 0x00, 0x02, 0x00, 0x02, 0xBC, 0x78, 0x8B, 0xD3, 0x19, 0x15, 0x85, 0xA6, 0x76, 0x89, 0xD2, 0xFE, 0xE6, 0xFA, 0x57, 0x7E, 
//0x96, 0x2C, 0x78, 0x74, 0xDF, 0x3B, 0xB3, 0x4A, 0xBA, 0xC5, 0xD4, 0x5C, 0xA2, 0x09, 0xBA, 0xBB, 0xE8, 0x1E, 0x20, 0x1B, 0x70, 0x40, 0x70, 0x78, 0xA9, 
//0x4B, 0xF9, 0xB2, 0x26, 0xD1, 0x59, 0x7F, 0x96, 0x79, 0x1A, 0x3E, 0xE3, 0xAF, 0xE7, 0x4D, 0x69, 0x8A, 0x16, 0x3C, 0xD5, 0xC8, 0x90, 0xE1, 0xDE, 0xCE, 
//0xAC, 0x9B, 0x08, 0x73, 0x62, 0xC0, 0xD6, 0xC2, 0x42, 0x9F, 0xCB, 0x9E, 0x3E, 0x63, 0x86, 0xD3, 0x6A, 0x92, 0x4C, 0x3E, 0x43, 0xA8, 0xE9, 0xBE, 0xAD, 
//0x66, 0x6C, 0x19, 0xED, 0x06, 0x0C, 0xD9, 0xC4, 0xC7, 0x45, 0x3C, 0x99, 0xEB, 0xFC, 0x07, 0x41, 0xF3, 0x70, 0xD7, 0x15, 0xB2, 0xD2, 0xD3, 0x01, 0xD4, 
//0x23, 0xAB, 0xE1, 0xE4, 0xD7, 0x80, 0x71, 0x27, 0x35, 0xF6, 0x29, 0x0E, 0x30, 0x3F, 0xEA, 0x0C, 0xB9, 0x8D, 0x4A, 0xAC, 0x4F, 0x13, 0xEB, 0xEC, 0x42, 
//0x65, 0x5B, 0x9F, 0x70, 0x99, 0x34, 0x07, 0x53, 0x65, 0x19, 0x04, 0xA1, 0x0E, 0x99, 0x2A, 0xB1, 0xB0, 0x39, 0xBE, 0x8F, 0xB7, 0x2D, 0x33, 0x61, 0x3A, 
//0x41, 0xC2, 0xB3, 0xC6, 0x07, 0x32, 0x17, 0x08, 0x07, 0x22, 0x9F, 0xB6, 0x5A, 0xC3, 0x04, 0x1E, 0xA2, 0x2E, 0xC9, 0xFE, 0xC9, 0x5D, 0x11, 0xD3, 0x11, 
//0xA5, 0x78, 0xE4, 0xC5, 0x17, 0x8E, 0xF5, 0xC9, 0x1C, 0xE1, 0x83, 0xE6, 0xB9, 0xA0, 0x94, 0x0C, 0x8A, 0xB5, 0x75, 0x89, 0xA7, 0x38, 0x04, 0xCB, 0x36, 
//0xB2, 0x95, 0xF6, 0xC7, 0x6D, 0x5F, 0x2F, 0xD0, 0xC5, 0x1F};
////	
//uint8_t data_0x00420029[235]={
//	0x40, 0x29, 0x00, 0x42, 0x00, 0x00, 0x02, 0x00, 0x02, 0xE3, 0xE7, 0xA0, 0x0B, 0x29, 0x5A, 0xE4, 0x54, 0xA1, 0x66, 0x53, 0x8B, 0x41, 0xD7, 0x14, 0x9C, 
//	0x86, 0x87, 0x4D, 0xA7, 0x30, 0x92, 0x32, 0x98, 0xF8, 0xEC, 0x9C, 0xD9, 0xFA, 0x0D, 0x88, 0x45, 0x1B, 0xCC, 0x2C, 0xA9, 0x15, 0x08, 0x04, 0xCB, 0xBC, 
//	0xFE, 0x6D, 0xDF, 0x16, 0xCF, 0x26, 0x0D, 0x3C, 0x30, 0x66, 0xBA, 0x61, 0xCF, 0xD0, 0x89, 0xE1, 0x06, 0x8D, 0xD5, 0x12, 0x0B, 0x79, 0x04, 0x5B, 0xDD, 
//	0x47, 0x23, 0x13, 0x3F, 0xCC, 0xFE, 0x6E, 0x49, 0x0F, 0xA0, 0xB7, 0xAD, 0xC3, 0x65, 0x02, 0xB7, 0x48, 0x07, 0x96, 0x81, 0x9D, 0xEF, 0xBE, 0xE1, 0xB4, 
//	0x45, 0x09, 0xAD, 0x2B, 0x2C, 0x12, 0x83, 0x2C, 0x24, 0xE3, 0x49, 0x3C, 0xE1, 0x61, 0x34, 0x4B, 0x9E, 0x6F, 0xAE, 0xAD, 0x2C, 0x1A, 0x8B, 0x1C, 0x85, 
//	0xCA, 0x2B, 0x40, 0x14, 0x8D, 0xD0, 0x2A, 0x3E, 0x5F, 0x25, 0x8D, 0x03, 0x5F, 0x02, 0x13, 0x76, 0xB8, 0x52, 0x17, 0xF6, 0x06, 0xF1, 0x02, 0xA0, 0xD5, 
//	0xDF, 0x98, 0x2D, 0x74, 0xDC, 0xA7, 0xF9, 0x7E, 0x27, 0x97, 0x23, 0x8F, 0xF4, 0xEA, 0x8A, 0x07, 0x10, 0x2C, 0x64, 0xE2, 0x69, 0x60, 0xCF, 0x78, 0x85, 
//	0x57, 0x07, 0x75, 0x0B, 0xA8, 0x18, 0xB5, 0x38, 0x2B, 0x8A, 0xD2, 0xE8, 0xC7, 0xFF, 0x0D, 0x76, 0xCD, 0xAC, 0x18, 0x4C, 0x6E, 0x97, 0xB1, 0x3E, 0x40, 
//	0x05, 0xF1, 0x07, 0xC5, 0x16, 0x6C, 0x34, 0x00, 0xDB, 0x88, 0x0E, 0xAE, 0x48, 0x2F, 0x1D, 0x08, 0x95, 0x21, 0xEB, 0xEE, 0x8C, 0x30, 0x99, 0x6D, 0x2B, 
//	0x7F, 0x39, 0xD5, 0x92, 0x64, 0xAC, 0x35, 0xF6, 0x98, 0xF0
//};
//	
//uint8_t data_0x001D004E[128]={
//0x40, 0x4E, 0x00, 0x1D, 0x00, 0x00, 0x02, 0x00, 0x02, 0xBC, 0x78, 0x8B, 0xD3, 0x19, 0x15, 0x85, 0xA6, 0x76, 0x89, 0xD2, 0xFE, 0xE6, 0xFA, 0x57, 0x7E, 0x96, 0x2C, 0x78, 0x74, 0xDF, 0x3B, 0xB3, 0x4A, 0xBA, 0xC5, 0xD4, 0x5C, 0xA2, 0x09, 0xBA, 0xBB, 0xE8, 0x1E, 0x20, 0x1B, 0x70, 0x40, 0x70, 0x78, 0xA9, 0x4B, 0xF9, 0xB2, 0x26, 0xD1, 0x59, 0x7F, 0x96, 0x79, 0x1A, 0x3E, 0xE3, 0xAF, 0xE7, 0x4D, 0x69, 0x8A, 0x16, 0x3C, 0xD5, 0xC8, 0x90, 0xE1, 0xDE, 0xCE, 0xAC, 0x9B, 0x08, 0x73, 0x62, 0xC0, 0xD6, 0xC2, 0x42, 0x9F, 0xCB, 0x9E, 0x3E, 0x63, 0x86, 0xD3, 0x6A, 0x92, 0x4C, 0x3E, 0x43, 0xA8, 0xE9, 0xBE, 0xAD, 0x66, 0x6C, 0x19, 0xED, 0x06, 0x0C, 0xD9, 0xC4, 0xC7, 0x45, 0x3C, 0x99, 0xEB, 0xFC, 0x07, 0x41, 0xF3, 0x70, 0xD7, 0x15, 0xB2, 0xD2, 0xD3, 0x01, 0x19, 0x0F, 0xA0, 0x00
//};	
//uint8_t data_0x00420029[128]={
//0x40, 0x29, 0x00, 0x42, 0x00, 0x00, 0x04, 0x00, 0x02, 0x8E, 0x76, 0xEF, 0x89, 0xDD, 0xFB, 0x00, 0x5F, 0x6A, 0xAC, 0x1D, 0x40, 0x59, 0x88, 0x76, 0x6D, 0xF4, 0xC5, 0xCD, 0x43, 0x08, 0xC0, 0xEA, 0x76, 0x67, 0xE3, 0xCA, 0x9A, 0x00, 0x03, 0xB8, 0x12, 0xCF, 0xD5, 0x35, 0xB6, 0x7F, 0x8F, 0xD7, 0x59, 0xBB, 0x52, 0x45, 0x67, 0x92, 0x80, 0x89, 0x92, 0xE3, 0x2B, 0x10, 0x89, 0xE7, 0xC2, 0x60, 0xE8, 0xA4, 0x0E, 0xA4, 0x5A, 0x91, 0x5A, 0x10, 0x59, 0x5F, 0x79, 0x8B, 0x11, 0xF2, 0xFA, 0x0C, 0xF7, 0xCB, 0x39, 0x7F, 0xF1, 0x3C, 0xFF, 0x73, 0x16, 0x0B, 0xB5, 0xB1, 0x4F, 0xB3, 0xC9, 0x1C, 0x06, 0x4E, 0x87, 0x34, 0xEA, 0x7B, 0xE3, 0xE6, 0x92, 0x7C, 0xF3, 0x91, 0xA8, 0x9C, 0x9B, 0xBD, 0x3E, 0x3E, 0xEE, 0x4A, 0xCC, 0x01, 0x28, 0xCF, 0x75, 0x0E, 0x88, 0xFC, 0x84, 0x54, 0x94, 0x03
//};


uint8_t data_0x001D004E[64] = {0x40, 0x4E, 0x00, 0x1D, 0x00, 0x00, 0x01, 0x00, 0x02, 0x50, 0xEE, 0xEC, 0x5F, 0x44, 0x0C, 0xFA, 0xC3, 0x36, 0xC8, 0xD6, 0x28, 0x90, 0xBC, 0x6E, 0xAC, 0x66, 0x03, 0xEF, 0x19, 0xCC, 0x36, 0x9C, 0x5A, 0xF9, 0xF0, 0x22, 0x02, 0xB3, 0xEE, 0x4D, 0x7B, 0x77, 0x70, 0xBB, 0x12, 0xAE, 0x85, 0x73, 0xA6, 0x5C, 0x4A, 0x46, 0xEA, 0x13, 0x61, 0x8E, 0x19, 0xA2, 0xC5, 0x81, 0x53, 0x65, 0x1F, 0x3F};
uint8_t data_0x00420029[64] = {0x40, 0x29, 0x00, 0x42, 0x00, 0x00, 0x01, 0x00, 0x02, 0xF2, 0x54, 0x99, 0xDE, 0x08, 0xC8, 0x64, 0xCC, 0x89, 0xB8, 0x1E, 0x3E, 0x3D, 0x54, 0xE9, 0x70, 0x55, 0x66, 0x22, 0x95, 0x31, 0x81, 0x97, 0x2C, 0xB3, 0xC7, 0x43, 0x75, 0xB4, 0x56, 0x4B, 0x40, 0xBB, 0xAB, 0x93, 0xB3, 0x03, 0x8A, 0xA0, 0x7D, 0xBE, 0xB2, 0xDB, 0x51, 0x50, 0xD0, 0x61, 0x25, 0xA5, 0xEA, 0x50, 0xF7, 0x82, 0x4F, 0xC9};
	
  uint16_t datarate,i;
	
	int *packet_freq_points_No1 = NULL;
	int *packet_freq_points_No2 = NULL;
	
	int symbol_len_No1 = NULL;
	int symbol_len_No2 = NULL;
	
	int *LoRa_ID_Start_Freq_No1 = NULL;
	int *LoRa_Payload_Start_Freq_No1 = NULL;

	int *LoRa_ID_Start_Freq_No2 = NULL;
	int *LoRa_Payload_Start_Freq_No2 = NULL;
	
	HAL_Init();

  SystemClock_Config();
	
  HW_Init();
	
	SPI1_Init();
	delay_init(80);
#ifdef ENABLE_USART
	uart_init(115200);
#endif
//	
	Control_GPIO_Init();
	LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);
	
	
	energest_init();
	int CF_list[8]={486300000,486500000,486700000,486900000,487100000,487300000,487500000,487700000};
	
	int CF = CF_list[0];
	int CF1= CF_list[0];
	int CF2= CF1 + 400000;
	
	for(int k=0;k<BUFFER_SIZE;k++)
	{
//		Tx_Buffer[k]=0x31;
		Tx_Buffer[k]=rand()%255;
	}
	
	
//		CF = CF_list[rand()%8];
		CF1 = CF_list[rand()%5];
		CF2= CF1 + 400000;
		
		printf("CR=4/%d, CRC=%s, IMPL_HEAD=%s, LDR=%s\n",4+LORA_CR_NO1,LORA_HAS_CRC_NO1?"ON":"OFF",LORA_IMPL_HEAD_NO1?"ON":"OFF",LORA_LOWDATERATEOPTIMIZE_NO1?"ON":"OFF");
		printf("FREQ1:%d,sf1:%d,\r\nFREQ2:%d,sf2:%d\r\n",CF,LORA_SF_NO1,CF2,LORA_SF_NO2);
		
		Radio.Init(&RadioEvents);
		Radio.SetChannel(CF);
		Radio.SetTxContinuousWave(CF,TX_OUTPUT_POWER,3);

		SX1276Write( REG_OSC, RF_OSC_CLKOUT_1_MHZ );
		
		SX1276Write( REG_PLLHOP, ( SX1276Read( REG_PLLHOP ) & RF_PLLHOP_FASTHOP_MASK ) | RF_PLLHOP_FASTHOP_ON );
		SX1276Write( REG_PLL, ( SX1276Read( REG_PLL ) & RF_PLL_BANDWIDTH_MASK ) | RF_PLL_BANDWIDTH_150 );
		SX1276Write( REG_PARAMP, ( SX1276Read( REG_PARAMP ) & RF_PARAMP_MASK ) | RF_PARAMP_0010_US );
		SX1276Write( REG_PARAMP, ( SX1276Read( REG_PARAMP ) & RF_PARAMP_MODULATIONSHAPING_MASK ) | RF_PARAMP_MODULATIONSHAPING_00 );
		SX1276Write( REG_OCP, ( SX1276Read( REG_OCP ) & RF_OCP_MASK ) | RF_OCP_OFF );
		
		datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )DATA_RATE );
		SX1276Write( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
		SX1276Write( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );
#ifdef ENABLE_USART
//	printf("Tx\r\n");
//	printf("CR=4/%d, CRC=%s, IMPL_HEAD=%s, LDR=%s\n",4+LORA_CR_NO1,LORA_HAS_CRC_NO1?"ON":"OFF",LORA_IMPL_HEAD_NO1?"ON":"OFF",LORA_LOWDATERATEOPTIMIZE_NO1?"ON":"OFF");
//	printf("FREQ1:%d,sf1:%d,\r\nFREQ2:%d,sf2:%d\r\n",CF1,LORA_SF_NO1,CF2,LORA_SF_NO2);
#endif

	for(i=0;i<PACKET_COUNT;i++)
	{
		packet_freq_points_No1 = LoRa_Channel_Coding(data_0x001D004E, 235, LORA_BW, LORA_SF_NO1, LORA_CR_NO1, LORA_HAS_CRC_NO1, LORA_IMPL_HEAD_NO1, &symbol_len_No1, LORA_LOWDATERATEOPTIMIZE_NO1);

		#ifdef LOOK_DOUBLE
		packet_freq_points_No2 = LoRa_Channel_Coding(data_0x00420029, 64, LORA_BW, LORA_SF_NO2, LORA_CR_NO2, LORA_HAS_CRC_NO2, LORA_IMPL_HEAD_NO2, &symbol_len_No2, LORA_LOWDATERATEOPTIMIZE_NO2);
		#endif
		
		#ifdef LOOK
		LoRa_Generate_Signal(packet_freq_points_No1,symbol_len_No1,CF);
		#endif
		#ifdef LOOK_BLANK
		LoRa_Generate_Signal_With_Blank(packet_freq_points_No1,symbol_len_No1,LOOK_BLANK_RATIO,RF_FREQUENCY_NO1);
		#endif
		#ifdef LOOK_DOUBLE
		LoRa_Generate_Double_Packet(packet_freq_points_No1,symbol_len_No1,packet_freq_points_No2,symbol_len_No2,CF1,CF2);
		#endif
		
		free(packet_freq_points_No1);
		#ifdef LOOK_DOUBLE
		free(packet_freq_points_No2);
		#endif
		
		#ifdef ENABLE_USART
		printf("Tx done, Count:%d\r\n",i+1);
		#endif
		delay_ms(INTERVAL_TIME);
	}
#ifdef ENABLE_USART
	printf("energyest tx:%lu",energest_type_time(ENERGEST_TYPE_TRANSMIT));
	printf("finish!!\r\n");
#endif

}


