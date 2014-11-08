//-----------------------------------------------------------------------------
// MAEVARM M4 STM32F373 USART
// date: July 12, 2014
// author: Chao Liu (chao.liu0307@gmail.com)
//-----------------------------------------------------------------------------

#ifndef M_USART_H_
#define M_USART_H_

#include "mGeneral.h"
#include "stdlib.h"
#include <stdio.h>

void USART_init(USART_TypeDef* USART_Channel, GPIO_TypeDef* GPIOx, uint32_t RCC_Periph, uint16_t TX_Pin, uint16_t RX_Pin, uint16_t TX_Pin_Source, uint16_t RX_Pin_Source);
void USART_set_baudrate(USART_TypeDef* USART_Channel, unsigned int baudrate);
void USART_transmit_char(USART_TypeDef* USART_Channel, unsigned char data);
void USART_transmit_int(USART_TypeDef* USART_Channel, int data);
void USART_transmit_uint(USART_TypeDef* USART_Channel, unsigned int data);
void USART_transmit_long(USART_TypeDef* USART_Channel, long data);
void USART_transmit_ulong(USART_TypeDef* USART_Channel, unsigned long data);
void USART_transmit_string(USART_TypeDef* USART_Channel, char *str);

#endif

