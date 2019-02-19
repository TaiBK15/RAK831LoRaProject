/*
*   DS1307.ino: Test DS1307
*   Kwang Ng
*/

#include <DS1307.h>

void setup() {
    Serial.begin(9600);
    while (!Serial); 
    delay(200);
    // struct m_time_t tm;
    // tm.second = 3;
    // tm.minute = 41;
    // tm.hour = 10;
    // tm.day = 6;
    // tm.wday = 1;
    // tm.month = 1;
    // tm.year = 2019;
    // DS1307.set(tm);
    Serial.println("DS1307 Test");
}

void loop() {
    struct m_time_t tm;
    // read
    DS1307.get(tm);
    Serial.println(tm.second);
    Serial.println(tm.minute);
    Serial.println(tm.hour);
    Serial.println(tm.day);
    Serial.println(tm.wday);
    Serial.println(tm.month);
    Serial.println(tm.year);
    delay(5000);
    Serial.println("======DS1307 End=====");

}