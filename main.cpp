#include "mbed.h"

#define toggle(x) { if (x) x=0; else x=1; }

// Set up serial port
Serial ftdi(p9, p10);

// Onboard LEDs
DigitalOut statusLED(LED1);


int main() {
	int i;

	for (i=0; i<30; i++) {
		ftdi.printf("Hello, number %d\n", i);
		wait(0.2);
		toggle(statusLED);
	}

	return 0;
}