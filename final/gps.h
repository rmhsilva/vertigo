#ifndef GPS_H
#define GPS_H

#define GPS_TX      p28
#define GPS_RX      p27

// Make variables available to main
extern int GPSerror, count434, count868, lock, dgps, lat, lon, alt, sats, hour, minute, second;
extern int last_lat, last_lon, last_alt;

// Function prototypes
void gps_setup();
void gpsPower(int i);
char gps_check_lock();
int gps_get_position();
int gps_get_time();

#endif