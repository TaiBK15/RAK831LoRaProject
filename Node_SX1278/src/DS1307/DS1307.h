/*
*   DS1307.h
*   Kwang Ng
*/

#ifndef LORA_H
#define LORA_H

#include <Arduino.h>

struct m_time_t {   // modified time type
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t wday;
    uint8_t month;
    uint8_t year;   // get 2 last numbers
};

class DS1307Class {
    public:
        DS1307Class();
        bool get(struct m_time_t &tm);
        bool set(m_time_t &tm);
    private:
        uint8_t bcd2dec(byte num);
        uint8_t dec2bcd(byte num);
};

extern DS1307Class DS1307;

#endif  //LORA_H