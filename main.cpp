#include "mbed.h"
#include "gps.h"

/*****[ Defines, constants etc ]*****************************************/

#define CLK_SHORT   5.0     // Short clock (seconds)
#define CLK_LONG    30.0    // Long clock!

#define PC_TX       p9      // Rx / Tx pins for PC (ftdi) comms
#define PC_RX       p10

#define GPS_TX      p13
#define GPS_RX      p14

#define TEMP_IN     p20       // Analogue temperature sensor in


/*****[ Declarations ]***************************************************/

// Set up serial port
Serial ftdi(PC_TX, PC_RX);

// GPS
GPS gps(GPS_TX, GPS_RX);
static bool volatile do_gps_update;
//Serial gps(GPS_TX, GPS_RX);
//volatile char gpsout[100];

// Onboard LEDs
DigitalOut statusLED(LED1);

// Temperature sensor
// .read() -> float[0.0 1.0], .read_u16() -> uint_16[0x0 0xFFFF]
AnalogIn temp(TEMP_IN);
static uint16_t volatile temperature;

// Clocker thing (it ticks...)
Ticker clk_s;
Ticker clk_l;

/*****[ Functions ]*******************************************************/

/*
 * Functions called on every clock tick
 */
void clkfn_short() {
    // Simple status clocker...

    temperature = temp.read_u16();
    do_gps_update = true;
}

void clkfn_long() {
    // Main timing loop.  Ie things that must happen over longer time periods
    return;
}

/*
 * Serial logging method over serial
 */
void slog(char *msg) {
    ftdi.printf("Log: %s\r\n", msg);
}


/*
 * Retrieves the config variables from config.txt and sets things up appropriately
 */
int getConfig() {
    LocalFileSystem local("local");
    FILE *fp = fopen("/local/config.txt", "r");

    if (!fp) {
        slog("Error: Config file not found");
        return 1;
    }
    slog("Config found :)");

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
    // Set initial variable values
    temperature = temp.read_u16();
    do_gps_update = true;
    
    // Do some config stuff
    getConfig();
    getDevices();

    // Attach tickers to functions
    clk_s.attach(&clkfn_short, CLK_SHORT);      // short clock
    clk_l.attach(&clkfn_long, CLK_LONG);        // long clock
}


/*
 * Main control loop
 */
int main() {    
    setup();

    for (;;) {

        // Retrieve new GPS values if required
        /* */
        if (do_gps_update) {
            do_gps_update = false;
            gps.get_position();
            gps.get_time();
        }
        
        // Print out the vals
        ftdi.printf("Temperature: 0x%x,  lock:%d, lat:%x, lon:%x, alt:%x, min:, done.\r\n", temperature,
                                                                    (int)gps.check_lock(),
                                                                    0,//gps.lat,//gps_pos->lat,
                                                                    0,//gps.lon,//gps_pos->lon,
                                                                    0,//gps.alt,//gps_pos->alt,
                                                                    0);//gps.minute);//gps_t->minute);
        
        
        //gps.scanf("%s", &gpsout);
        //ftdi.printf("Temp: 0x%x \r\n", temperature);
        
        wait(1.0);
        statusLED = !statusLED;
    }

    return 0;
}