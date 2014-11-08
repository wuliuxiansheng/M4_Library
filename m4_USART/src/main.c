#include "mGeneral.h"
#include "mUSART.h"
#include "stdio.h"

int main( void )
{
    mInit();
    USART_init(USART1, GPIOA, RCC_APB2Periph_USART1, GPIO_Pin_9, GPIO_Pin_10, GPIO_PinSource9, GPIO_PinSource10);
    USART_set_baudrate(USART1, 9600);
    int data_int = 1260;
    unsigned int data_uint = 1000;
    long data_long = 1000499;
    unsigned long data_ulong = 2400047;
    
    mWaitms(3000);
    while(1)
    {
        USART_transmit_string(USART1, "Hello!\n\r");
        USART_transmit_int(USART1, data_int);
        USART_transmit_char(USART1, '\n');
        USART_transmit_char(USART1, '\r');
        USART_transmit_uint(USART1, data_uint);
        USART_transmit_char(USART1, '\n');
        USART_transmit_char(USART1, '\r');
        USART_transmit_long(USART1, data_long);
        USART_transmit_char(USART1, '\n');
        USART_transmit_char(USART1, '\r');
        USART_transmit_ulong(USART1, data_ulong);
        USART_transmit_char(USART1, '\n');
        USART_transmit_char(USART1, '\r');
    }

}
