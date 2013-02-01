#include "mbed.h"

DigitalOut ss_port_434(p8);
SPI rfm434(p5, p6, p7);

#include "RFM22nu.cpp"


DigitalOut led(LED1);



void rtty_txbit (int bit)
{
		if (bit)
		{
		  // high
                  write_rfm434(0x73, 0x03);
		}
		else
		{
		  // low
                  write_rfm434(0x73, 0x00);
		}
					 // 50 Baud:
                wait_us(19500); // 10000 = 100 BAUD 20150
                // Theoretical 100 Baud:
                //wait_us(9500); // (20000)/2 = 10000 - 500 as that seems to be how the one above is calculated
                // Theoretical 300 Baud:
                //wait_us(2850); // (20000)/6 = 3333.3 - 500 as that seems to be how the one above is calculated

}


void rtty_txbyte (char c)
{
	/* Simple function to sent each bit of a char to 
	** rtty_txbit function. 
	** NB The bits are sent Least Significant Bit first
	**
	** All chars should be preceded with a 0 and 
	** proceded with a 1. 0 = Start bit; 1 = Stop bit
	**
	** ASCII_BIT = 7 or 8 for ASCII-7 / ASCII-8
	*/
	int i;
	rtty_txbit (0); // Start bit
	// Send bits for for char LSB first	
	for (i=0;i<7;i++)
	{
		if (c & 1) rtty_txbit(1); 
			else rtty_txbit(0);	
		c = c >> 1;
	}
	rtty_txbit (1); // Stop bit
        rtty_txbit (1); // Stop bit
}

void rtty_txstring (char * string)
{

	/* Simple function to sent a char at a time to 
	** rtty_txbyte function. 
	** NB Each char is one byte (8 Bits)
	*/
	char c;
	c = *string++;
	while ( c != '\0')
	{
		rtty_txbyte (c);
		c = *string++;
	}
}

int crc_xmodem_update (int crc, char data)
    {
        int i;

        crc = crc ^ ((int)data << 8);
        for (i=0; i<8; i++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }

        return crc;
    }

int CRC16_checksum (char *string)
{
	int i;
	int crc;
	char c;
 
	crc = 0xFFFF;
 
	// Calculate checksum ignoring the first two $s
	for (i = 2; i < strlen(string); i++)
	{
		c = string[i];
		crc = crc_xmodem_update (crc, c);
	}
 
	return crc;
}

int main() {
	led = 1;
	//int response = rfm434.write(0xFF);
	initSPI_434();

	init_rfm434();
	
	write_rfm434(0x71, 0x00); // unmodulated carrier
	//This sets up the GPIOs to automatically switch the antenna depending on Tx or Rx state, only needs to be done at start up
	write_rfm434(0x0b,0x12);
	write_rfm434(0x0c,0x15);
  
	setFrequency_rfm434(434.201);
  
	write_rfm434(0x6D, 0x04);// turn tx low power 11db
  
	write_rfm434(0x07, 0x08); // turn tx on
	
	wait(5.0);
	
	rtty_txstring("TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST");
}