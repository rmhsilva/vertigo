#include "mbed.h"
#include "gps.h"

/*****[ Defines, constants etc ]*****************************************/

#define CLK_SHORT   10.0    // Short clock (seconds)
#define CLK_LONG    30.0    // Long clock!

#define PC_TX       p9      // Rx / Tx pins for PC (ftdi) comms
#define PC_RX       p10

#define GPS_TX      p13     // Rx / Tx for GPS
#define GPS_RX      p14

#define TEMP_IN     p20       // Analogue temperature sensor in


/*****[ Declarations ]***************************************************/

// Set up serial port
Serial ftdi(PC_TX, PC_RX);

// GPS
gps_position* gps_pos;
gps_time* gps_t;
volatile bool do_gps_update;

// Onboard LEDs
DigitalOut statusLED(LED1);

// Temperature sensor
// .read() -> float[0.0 1.0], .read_u16() -> uint_16[0x0 0xFFFF]
AnalogIn temp(TEMP_IN);
volatile float temperature;

// Clocker thing (it ticks...)
Ticker clk_s;
Ticker clk_l;

/*****[ Functions ]*******************************************************/

/*
 * Functions called on every clock tick
 */
void clkfn_short() {
    // Simple status clocker...
    statusLED = !statusLED;

    temperature = temp.read();
	do_gps_update = true;
}

void clkfn_long() {
    // Main timing loop.  Ie things that must happen over longer time periods
    return;
}

/*
 * PC debugging method over serial
 */
void err(char *msg) {
    ftdi.printf("Error: %s\r\n", msg);
}


/*
 * Retrieves the config variables from config.txt and sets things up appropriately
 */
int getConfig() {
    LocalFileSystem local("local");
    FILE *fp = fopen("/local/config.txt", "r");

    if (!fp) {
        err("Config file not found");
        return 1;
    }

    fclose(fp);
    return 0;
}

/*
 * Gets a list of connected devices and writes to a file
 */
 void getDevices() {
    LocalFileSystem local("local");
    FILE *fp = fopen("/local/devices.txt", "w");

    fprintf(fp, "Devices detected\n");

    fclose(fp);
 }


/*
 * Setup various systems
 */
void setup () {
    // Attach tickers to functions
    clk_s.attach(&clkfn_short, CLK_SHORT);      // short clock
    clk_l.attach(&clkfn_long, CLK_LONG);        // long clock

    temperature = 0;
	do_gps_update = false;

	GPS_setup();
	
    getConfig();
    getDevices();
}


/*
 * Main control loop
 */
int main() {    
    setup();

    for (;;) {
		// Retrieve new GPS values if required
		if (do_gps_update) {
			do_gps_update = false;
			gps_pos = gps_get_position();
			gps_t = get_gps_time();
		}
		
		// Print out the vals
        ftdi.printf("Temperature: %f.  lat:%d, lon:%d, alt:%d, min:%d \r\n", temperature,
																	gps_pos->lat,
																	gps_pos->lon,
																	gps_pos->alt,
																	gps_t->minute);
        wait(0.2);
    }

    return 0;
}