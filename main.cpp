#include "mbed.h"

/*****[ Declarations ]***************************************************/

// Set up serial port
Serial ftdi(p9, p10);

// Onboard LEDs
DigitalOut statusLED(LED1);

// Temperature sensor
// .read() -> float[0.0 1.0], .read_u16() -> uint_16[0x0 0xFFFF]
AnalogIn temp(p20);
volatile float temperature;

// Clocker thing (it ticks...)
Ticker clk1;
Ticker clk30;

/*****[ Functions ]*******************************************************/

/*
 * Functions called on every clock tick
 */
void clkfn1() {
    // Simple status clocker...
    statusLED = !statusLED;

    temperature = temp.read();
}

void clkfn30() {
    // Main timing loop.  Ie things can happen in 30s intervals
}


/*
 * Setup various systems
 */
void setup () {
    // Attach tickers to functions
    clk1.attach(&clkfn1, 1.0);      // 1 second clock
    clk30.attach(&clkfn30, 30.0);   // 30 second clock

    temperature = 0;
}


/*
 * Main control loop
 */
int main() {
    int i;
    
    setup();

    for (;;) {
        ftdi.printf("Temperature: %f \r\n", temperature);
        wait(0.2);
    }

    return 0;
}