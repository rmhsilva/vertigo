#ifndef GPS_H
#define GPS_H

#define GPS_TX  p13
#define GPS_RX  p14

// Useful structs

typedef struct {
    int lat;
    int lon;
    int alt;
    int GPSerror;
} gps_position;

typedef struct {
    int second;
    int minute;
    int hour;
    int GPSerror;
} gps_time;


// Function prototypes

void GPS_setup();
gps_position* gps_get_position();
gps_time* gps_get_time();
bool gps_verify_checksum(uint8_t* data, uint8_t len);
bool gps_check_lock();

#endif
