#ifndef FLIGHT_INFO
#define FLIGHT_INFO

    typedef struct flight_info {
        char icao24[1024];
        char callsign[1024];
        char model[1024];
    } flight_info;

#endif
