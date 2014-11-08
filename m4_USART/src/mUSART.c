//-----------------------------------------------------------------------------
// MAEVARM M4 STM32F373 USART
// date: July 12, 2014
// author: Chao Liu (chao.liu0307@gmail.com)
//-----------------------------------------------------------------------------

#include "mUSART.h"

USART_InitTypeDef USART_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;

void USART_init(USART_TypeDef* USART_Channel, GPIO_TypeDef* GPIOx, uint32_t RCC_Periph, uint16_t TX_Pin, uint16_t RX_Pin, uint16_t TX_Pin_Source, uint16_t RX_Pin_Source)
{
  
  /* Enable USART APB clock */
    switch (RCC_Periph) {
        case RCC_APB2Periph_USART1:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            break;
        case RCC_APB1Periph_USART2:
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
            break;
        case RCC_APB1Periph_USART3:
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
            break;
        default:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            break;
    }
  //RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  /* USART Pins configuration **************************************************/
 
  /* Connect pin to Periph */
  GPIO_PinAFConfig(GPIOx, TX_Pin_Source, GPIO_AF_7);
  GPIO_PinAFConfig(GPIOx, RX_Pin_Source, GPIO_AF_7);
  
  /* Configure pins as AF pushpull */
  GPIO_InitStructure.GPIO_Pin = TX_Pin | RX_Pin;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOx, &GPIO_InitStructure);
   
  /* USARTx configured as follow:
  - Default baudrate = 9600
  - Word Length = 8 Bits
  - Stop Bit = 1 Stop Bit
  - Parity = No Parity
  - Hardware flow control disabled (RTS and CTS signals)
  - Receive and transmit enabled
  */
  
  //USART_DeInit(USART2);
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART_Channel, &USART_InitStructure);
  
    /* USART enable */
  USART_Cmd(USART_Channel, ENABLE);
 
}

void USART_set_baudrate(USART_TypeDef* USART_Channel, unsigned int baudrate)
{
    USART_DeInit(USART_Channel);
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART_Channel, &USART_InitStructure);
    USART_Cmd(USART_Channel, ENABLE);
}

void USART_transmit_char(USART_TypeDef* USART_Channel, unsigned char data)
{
    USART_SendData(USART_Channel, data);
    while(USART_GetFlagStatus(USART_Channel,USART_FLAG_TXE)==RESET);
}

void USART_transmit_int(USART_TypeDef* USART_Channel, int data)
{
    char string[7] = {0,0,0,0,0,0,0};
    snprintf(string, 10, "%d", data);
	for(int i=0; i<7; i++)
	{
		if(string[i])
		{
			USART_transmit_char(USART_Channel, string[i]);
		}
	}
}

void USART_transmit_uint(USART_TypeDef* USART_Channel, unsigned int data)
{
	char string[6] = {0,0,0,0,0,0};
    snprintf(string, 10, "%ud", data);
	for(int i=0; i<5; i++)
	{
		if(string[i])
		{
			USART_transmit_char(USART_Channel, string[i]);
		}
	}
}

void USART_transmit_long(USART_TypeDef* USART_Channel, long data)
{
	char string[11] = {0,0,0,0,0,0,0,0,0,0,0};
    snprintf(string, 10, "%l", data);
	for(int i=0; i<11; i++)
	{
		if(string[i])
		{
			USART_transmit_char(USART_Channel, string[i]);
		}
	}
}

void USART_transmit_ulong(USART_TypeDef* USART_Channel, unsigned long data)
{
	char string[11] = {0,0,0,0,0,0,0,0,0,0,0};
	snprintf(string, 10, "%ul", data);
	for(int i=0; i<10; i++)
	{
		if(string[i])
		{
			USART_transmit_char(USART_Channel, string[i]);
		}
	}
}



void USART_transmit_string(USART_TypeDef* USART_Channel, char *str)
{
    while(*str)
    {
        USART_SendData(USART_Channel,*str++);
        
        while(USART_GetFlagStatus(USART_Channel,USART_FLAG_TXE)==RESET);
        
    }
}

