#include "mbed.h"
#include "gps.h"

// ------ GPS CODE
int GPSerror = 0, lon, lat, alt, hour, minute, second, total_time;
Serial gps(GPS_TX, GPS_RX);

// GPS SETUP
void GPS_setup()
{
  uint8_t setNMEAoff[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA9};
  gps.printf("%s", (char *)setNMEAoff);
}

// GET POSITION
gps_position* gps_get_position()
{
  char position[60]
  gps_position* position_str;
  GPSerror = 0;

  // Request data
  uint8_t request[8] = {0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03, 0x0A};
  gps.printf("%s", request);

  // getting data
  gps.scanf("%s", &position);

  if( position[0] != 0xB5 ||position[1] != 0x62 )
    GPSerror = 21;
  if( position[2] != 0x01 || position[3] != 0x02 )
    GPSerror = 22;

  if( !_gps_verify_checksum(&position[2], 32) ) {
    GPSerror = 23;
  }

  // GPS data
  if(GPSerror == 0) {
    // 4 bytes of longitude (1e-7)
    lon = (int32_t)position[10] | (int32_t)position[11] << 8 |
    (int32_t)position[12] << 16 | (int32_t)position[13] << 24;
    lon /= 1000;

    // 4 bytes of latitude (1e-7)
    lat = (int32_t)position[14] | (int32_t)position[15] << 8 |
    (int32_t)position[16] << 16 | (int32_t)position[17] << 24;
    lat /= 1000;

    // 4 bytes of altitude above MSL (mm)
    alt = (int32_t)position[22] | (int32_t)position[23] << 8 |
    (int32_t)position[24] << 16 | (int32_t)position[25] << 24;
    alt /= 1000;

    position_str->lon = lon;
    position_str->lat = lat;
    position_str->alt = alt;
  }

  position_str->GPSerror = GPSerror;
  return position_str;
}

gps_time* gps_get_time()
{
  char time[60];
  gps_time* time_str;
  GPSerror = 0;

  uint8_t request[8] = {0xB5, 0x62, 0x01, 0x21, 0x00, 0x00, 0x22, 0x67, 0x0};
  gps.printf("%s", request);

  // getting data
  gps.scanf("%s", &time);

  // Verify the sync and header bits
  if( time[0] != 0xB5 || time[1] != 0x62 )
    GPSerror = 31;
  if( time[2] != 0x01 || time[3] != 0x21 )
    GPSerror = 32;

  if( !_gps_verify_checksum(&time[2], 24) ) {
    GPSerror = 33;
  }

  if(GPSerror == 0) {
    hour = time[22];
    minute = time[23];
    second = time[24];

    // Check for errors in the value
    if(hour > 23 || minute > 59 || second > 59)
    {
      GPSerror = 34;
    }
    else {
      time_str->hour = hour;
      time_str->minute = minute;
      time_str->second = second;
    }
  }

  // Send error back
  time_str->GPSerror = GPSerror;
  return time_str;
}

bool gps_check_lock()
{
  GPSerror = 0;
  bool check;
  char[60] lock;

  // Construct the request to the GPS
  uint8_t request[8] = {0xB5, 0x62, 0x01, 0x06, 0x00, 0x00, 0x07, 0x16};
  gps.printf("%s", request);

  // getting data
  gps.scanf("%s", &lock);

  // Verify the sync and header bits
  if( lock[0] != 0xB5 || lock[1] != 0x62 ) {
    GPSerror = 11;
  }
  if( lock[2] != 0x01 || lock[3] != 0x06 ) {
    GPSerror = 12;
  }

  // Check 60 bytes minus SYNC and CHECKSUM (4 bytes)
  if( !_gps_verify_checksum(&lock[2], 56) ) {
    GPSerror = 13;
  }

  if(GPSerror == 0){
    // Return the value if GPSfixOK is set in 'flags'
    if( lock[17] & 0x01 ) {
    //check = buf[16];
    //dgps = buf[18];
      check = lock[16];
    } else
    check = 0;

  //sats = buf[53];
  }
  else {
    check = 0;
  }

  return check;
}

bool gps_verify_checksum(uint8_t* data, uint8_t len)
{
  uint8_t a, b;
  gps_ubx_checksum(data, len, &a, &b);
  if( a != *(data + len) || b != *(data + len + 1))
    return false;
  else
    return true;
}