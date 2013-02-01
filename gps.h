#ifndef GPS_H
#define GPS_H

class GPS {
    public:
        int lat;
        int lon;
        int alt;
        int minute;
        int hour;
        int second;

        GPS(PinName tx, PinName rx);

        bool verify_checksum(uint8_t* data, uint8_t len);
        char check_lock();

        int get_position();
        int get_time();
        
    private:
        Serial gps;
        int GPSerror;
};

#endif