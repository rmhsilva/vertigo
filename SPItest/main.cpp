#include "mbed.h"
#include "RFM22nu.cpp"
#include "RTTY.cpp"

DigitalOut led(LED1);

rfm22 rfm434(p5, p6, p7, p8); // mosi, miso, sclk, ss

int main() {
	led = 1;
	//int response = rfm434.write(0xFF);
	rfm434.initSPI();

	rfm434.init();
	
	rfm434.write(0x71, 0x00); // unmodulated carrier
	//This sets up the GPIOs to automatically switch the antenna depending on Tx or Rx state, only needs to be done at start up
	rfm434.write(0x0b,0x12);
	rfm434.write(0x0c,0x15);
  
	rfm434.setFrequency(434.201);
  
	rfm434.write(0x6D, 0x04);// turn tx low power 11db
  
	rfm434.write(0x07, 0x08); // turn tx on
	
	wait(5.0);
	
	rtty_txstring("TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST);
}
