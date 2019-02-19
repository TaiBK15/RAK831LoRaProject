/*
*   E32LoRa.h:  lib for E32-TTL-100 UART LoRa module
*   Kwang Ng
*/

#ifndef LORA_H
#define LORA_H

#include <Arduino.h>

#define MODE_NORMAL         0
#define MODE_WAKEUP         1
#define MODE_POWER_SAVING   2
#define MODE_SLEEP          3

#define DATARATE_HIGHEST    19200
#define DATARATE_LOWEST     300  

struct e32_t {
    uint8_t HEADER;
    uint8_t DEVICE_ADDRESS_HIGH;
    uint8_t DEVICE_ADDRESS_LOW;
    uint8_t DEVICE_CHANNEL;
    uint8_t SPEED;
    uint8_t OPTION;
};

struct e32_pins {
    int M0;
    int M1;
    int AUX;
};

class E32LoRaClass {
    public:
        E32LoRaClass(int M0, int M1, int AUX, Stream& e32_serial);
        
        bool set();
        void setPins(int M0, int M1, int AUX);
        bool setMode(uint8_t mode);
        bool setDataRate( int DR );
        bool setAddress( uint8_t addr_h, uint8_t addr_l );
        bool setFrequency( int freq_mhz );

        bool get( struct e32_t para);
        int getDataRate();
        bool getFrequency();
        bool getAddressHigh();
        bool getAddressLow();

        bool begin(uint8_t mode);
        void send( uint8_t *payload, uint8_t nb_byte );
        void fetch( uint8_t *data );
        int listen();

    private:
        e32_t _para;
        e32_pins _pin;
        int _data_length;
        Stream& _e32_serial;
        bool waitAUX();
}

extern E32LoRaClass E32LoRa;

#endif