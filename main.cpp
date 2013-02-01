#include "mbed.h"

// Set up serial port
Serial ftdi(p9, p10);

// Onboard LEDs
DigitalOut statusLED(LED1);

// Temperature sensor
// .read() -> float[0.0 1.0], .read_u16() -> uint_16[0x0 0xFFFF]
AnalogIn temp(p20);


int main() {
	int i;

	for (;;) {
		ftdi.printf("Temperature: %f \r\n", temp.read());
		wait(0.2);
		statusLED = !statusLED;
	} 

	return 0;
}