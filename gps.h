#ifndef GPS_H
#defin GPS_H

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
bool gps_verify_checksum();
bool gps_check_lock();

#endif
