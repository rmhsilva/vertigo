#include "mbed.h"
#include "common.h"
#include "gps.h"
#include "radio.h"
#include "config.h"
#include "wdt.cpp"

#define CUTDOWN     LED4
#define TEMP_IN     p16        // Analogue temperature sensor in
#define BATTERY     p18

#define CUTDOWN_ALT 50000    // Cutdown altitude
#define BATT_MUL    100
#define WDT_KICK    60.0    // WDT kick interval

//Cutdown and Temp pins
DigitalOut cutdown(CUTDOWN);
AnalogIn temp(TEMP_IN);
volatile int temperature;
int cutdown_enabled;

// Watchdog timer
Watchdog wdt;

// Local filesystem setup
LocalFileSystem local("local");

//Battery monitor
AnalogIn battery(BATTERY);
int battery_voltage;

// Status LEDs
#undef debug
#ifdef debug
DigitalOut status_led(LED1);
DigitalOut gps_led(LED2);
DigitalOut radio_led(LED3);
#else
int status_led, gps_led, radio_led;
#endif

// Config stuff
int config_err;

// Events
int send_gps, send_temp;

// Radio config things
int rssi, enable_868;

/*******[ Program ]******************************************************/

void save_434id(int id) {
    FILE *fp = fopen("/local/434id.txt", "w");
    fprintf(fp, "%d", id);
    fclose(fp);
}
void save_868id(int id) {
    FILE *fp = fopen("/local/868id.txt", "w");
    fprintf(fp, "%d", id);
    fclose(fp);
}

void setup() {
    // Initialise stuff!
    status_led = 0; gps_led = 0; radio_led = 0;
    send_gps = 0; send_temp = 0;
    cutdown_enabled = 0; cutdown = 0;

    ftdi.printf("\r\n[+] Reading Config \r\n");
    ftdi.printf("Config error: %d \r\n", (config_err = Config()));

    ftdi.printf("Initiliasing GPS..\r\n");
    gps_setup();
    gps_led = 1;

    temperature = (int16_t)((temp.read_u16()-9930)/199.0);    // Must read temp before turning on rfm

    ftdi.printf("Initiliasing RFM22... ");
    rfm22_434_setup();
    rfm22_868_setup();

    setFrequency_rfm434(434.201);
    write_rfm434(0x6D, 0x04);        // turn tx low power 11db
    write_rfm434(0x07, 0x08);        // turn tx on
    
    setFrequency_rfm868(869.5);
    
    rssi = ((int)read_rfm868(26)*51 - 12400)/100; // returned in dBm
    if (rssi > -50) { // if 868 channel is busy
        enable_868 = 0;
    } else {
        enable_868 = 1;
        write_rfm868(0x6D, 0x07);// turn tx high power 17db
        write_rfm868(0x07, 0x08); // turn tx on
    }
    radio_led = 1;
    ftdi.printf("Complete \r\n");
    
    ftdi.printf("Loading old packet IDs \r\n");
    FILE *fp = fopen("/local/434id.txt", "r");
    fscanf(fp, "%d", &count434);
    fclose(fp);
    fp = fopen("/local/868id.txt", "r");
    fscanf(fp, "%d", &count868);
    fclose(fp);
    ftdi.printf("count434: %d, count868: %d \r\n", count434, count868);

    wdt.kick(WDT_KICK); // Hard reset after 60seconds of no kick()
    ftdi.printf("Setup complete \r\n");
}


int main() {
    char state;
    int nlock_count = 0, n;

    setup();
    
    for (;;) {
        wdt.kick();
        ftdi.printf("Entering Loop.\r\n");
        status_led = !status_led;
        
        write_rfm868(0x07, 0x01); // turn tx off - needed for below code
        write_rfm434(0x07, 0x01);

        // If GPS does not have lock for > 8 loops, restart it.
        if (nlock_count>8) {
            gpsPower(0);
            wait(2);
            gpsPower(1);
            nlock_count = 0;
        }
        
        wait(0.1); // wait for tx to turn off

        // Get temp while radio off
        temperature = (int16_t)((temp.read_u16()-9930)/19.9);
        battery_voltage = (int16_t)(battery.read()*BATT_MUL);

        rssi = ((int)read_rfm868(26)*51 - 12400)/100; // returned in dBm
        if (rssi > -50) { // if 868 channel is busy
            enable_868 = 0;
        } else {
            enable_868 = 1;
            write_rfm868(0x07, 0x08); // turn tx on
        }
        
        write_rfm434(0x07, 0x08); // turn tx on

        // Check for GPS lock
        gps_check_lock();
        if (lock==0) {
            state='L';
            nlock_count++;
        } else if (!enable_868) {
            state='O';
        } else {
            nlock_count = 0;
            state='F';
        }

        gps_get_position();
        gps_get_time();
        
        // Process triggers here
        if (config_err == 0) {
            ftdi.printf("Running triggers... ");
            runTrigger(0);
            runTrigger(1);
            runTrigger(2);
            ftdi.printf("Finished. \r\n");
        }
        
        // Cutdown check
        if (cutdown_enabled && (alt>CUTDOWN_ALT)) {
            // Double check the altitude!
            gps_get_position();
            if (alt>CUTDOWN_ALT) {
                ftdi.printf("Cutting wire!!!! \r\n");
                cutdown = 1;    // burn that wire!!!!!.   ..   !
                wait(3);
                cutdown = 0;
                cutdown_enabled = 0;
                state = 'C';
            }
        }

        // Do TX (always true, triggers not quite working)
        //if (send_gps) {
        if (1) {
            n = sprintf (buffer434, "$$VERTIGO,%d,%02d%02d%02d,%ld,%ld,%ld,%d,%c", count434, hour, minute, second, lat, lon, alt, sats, state);
            n = sprintf (buffer434, "%s*%04X\n", buffer434, (CRC16_checksum(buffer434) & 0xFFFF));

            rtty_434_txstring(buffer434);
            save_434id(count434++);
            send_gps = 0;
        }
        
        // And for 868 if enabled
        if(enable_868) {
            ftdi.printf("Doing 868 now \r\n");
            for (int count=0; count<10; count++) {
                ftdi.printf("\t loop %d\r\n", count);
                last_lon = lon;
                last_lat = lat;
                last_alt = alt;
                gps_get_position();
                gps_get_time();
                if ((lat==last_lat)&&(lon==last_lon)&&(alt==last_alt)) {
                    gps_check_lock();
                } else {
                    state = 'F'; // cheat a second here.
                }
                n = sprintf (buffer868, "$$8VERTIGO,%d,%02d%02d%02d,%ld,%ld,%ld,%d,%c,%d,%d,%d", count868, hour, minute, second, lat, lon, alt, sats, state, temperature, battery_voltage, rssi);
                n = sprintf (buffer868, "%s*%04X\n", buffer868, (CRC16_checksum(buffer868) & 0xFFFF));
        
                rtty_868_txstring(buffer868);
                save_868id(count868++);
            }
        }
    }

    return 0;
}