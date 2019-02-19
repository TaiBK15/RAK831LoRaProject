/*
*   DS1307.cpp
*   Kwang Ng
*/

#include <Wire.h>
#include "DS1307.h"

#define DS1307_ID   0x68


DS1307Class::DS1307Class() {
    Wire.begin();

}

bool DS1307Class::get(struct m_time_t &tm) {

    Wire.beginTransmission(DS1307_ID);
    Wire.write((uint8_t)0x00);   //reset register pointer
    if (Wire.endTransmission() != 0)
        return false;
    Wire.requestFrom(DS1307_ID, 7);

    tm.second = bcd2dec(Wire.read() & 0x7f);
    tm.minute = bcd2dec(Wire.read());
    tm.hour = bcd2dec(Wire.read() & 0x3f);
    tm.wday = bcd2dec(Wire.read());
    tm.day =  bcd2dec(Wire.read());
    tm.month = bcd2dec(Wire.read());
    tm.year = bcd2dec(Wire.read());

    return true;
}

bool DS1307Class::set(struct m_time_t &tm) {
    Wire.beginTransmission(DS1307_ID);
    Wire.write(byte(0x00));     //reset register pointer
    Wire.write(dec2bcd(tm.second));
    Wire.write(dec2bcd(tm.minute));
    Wire.write(dec2bcd(tm.hour));
    Wire.write(dec2bcd(tm.wday)); // day of week: Sunday = 1, Saturday = 7
    Wire.write(dec2bcd(tm.day)); 
    Wire.write(dec2bcd(tm.month));
    Wire.write(dec2bcd(tm.year));

    if (Wire.endTransmission() != 0) {
        return false;   // failed
    }
    return true;
}

uint8_t DS1307Class::bcd2dec(byte num) {
    return ((num/16 * 10) + (num % 16));
}

uint8_t DS1307Class::dec2bcd(byte num) {
    return ((num/10 * 16) + (num % 10));
}

// instance
DS1307Class DS1307;