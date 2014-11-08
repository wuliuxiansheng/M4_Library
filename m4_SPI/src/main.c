#include "mGeneral.h"
#include "SPI.h"

#include "stdio.h"




int main( void )
{
    mInit();
    SPI3_Init();
    u16 data;
    int i = 0;
    int count = 1;

    while(1)
    {
      while(count)
	{
	  mGreenON;
	  int data_tx = 0xAB;
	  data = SPIx_ReadWriteByte(data_tx);
	  i++;
	  if(i > 50000)
	    {
	      i = 1;
	      count = 0;
	    }
	}
	  

    }
    
}
