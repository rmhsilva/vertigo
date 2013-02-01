#include "mbed.h"

// Set up serial port
Serial ftdi(p9, p10);

// Onboard LEDs
DigitalOut statusLED(LED1);

// Temperature sensor
AnalogIn temperature(p20);


int main() {
	int i;

	for (;;) {
		ftdi.printf("Temp: %d \r\n", temperature.read());
		wait(0.2);
		statusLED = !statusLED;
	}

	return 0;
}