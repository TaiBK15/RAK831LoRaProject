/*
*   E32LoRa.h: lib for E32-TTL-100 UART LoRa module
*   Kwang Ng
*/

#include "E32LoRa.h"
#include <SoftwareSerial.h>

E32LoRaClass::E32LoRaClass(int M0, int M1, int AUX, Stream& e32_serial) {
    _pin.M0 = M0;
    _pin.M1 = M1;
    _pin.AUX = AUX;
    _e32_serial = e32_serial;
    // SoftwareSerial LoraSerial(RX, TX);

    pinMode(_pin.M0, OUTPUT);
    pinMode(_pin.M1, OUTPUT);
    pinMode(_pin.AUX, INPUT);
}

bool E32LoRaClass::setParas(struct e32_t para) {
    _para.HEADER = para.HEADER;
    _para.DEVICE_ADDRESS_HIGH = para.DEVICE_ADDRESS_HIGH;
    _para.DEVICE_ADDRESS_LOW = para.DEVICE_ADDRESS_LOW;
    _para.DEVICE_CHANNEL = para.DEVICE_CHANNEL;
    _para.SPEED = para.SPEED;
    _para.OPTION = para.OPTION;

    return set();
}

bool E32LoRaClass::set() {
    setMode(MODE_SLEEP);
    if (!waitAUX()) return false;
    uint8_t setting_cmd[6] = {_para.HEADER, _para.DEVICE_ADDRESS_HIGH, _para.DEVICE_ADDRESS_LOW,
                                _para.DEVICE_CHANNEL, _para.SPEED, _para.OPTION};
    _e32_serial.write(setting_cmd, 6);
    if (!waitAUX()) return false;
    // Check setting paras
    while(_e32_serial.available()){_e32_serial.read();} //Clean Uart Buffer
    uint8_t read_cmd[3] = {0xC1, 0xC1, 0xC1};
    _e32_serial.write(read_cmd, 3);
    if (!waitAUX()) return false;
    uint8_t buff[6];
    uint8_t count = 0;
    count = _e32_serial.available();
    if (count) {
        for(int i=0; i<count; i++) {
            buff[i] = _e32_serial.read();
            if (buff[i] != setting_cmd[i]) {
                return false;
            }
            delay(10);
        }
    }
    return true;
}

void E32LoRaClass::setPins(int M0, int M1, int AUX) {
    _pin.M0 = M0;
    _pin.M1 = M1;
    _pin.AUX = AUX;
}

bool E32LoRaClass::setMode(uint8_t mode) {
    switch (mode) {
        case MODE_NORMAL:
            digitalWrite(_pin.M0, LOW);
            digitalWrite(_pin.M1, LOW);
            return true;
        case MODE_WAKEUP:
            digitalWrite(_pin.M0, HIGH);
            digitalWrite(_pin.M1, LOW);
            return true;
        case MODE_POWER_SAVING:
            digitalWrite(_pin.M0, LOW);
            digitalWrite(_pin.M1, HIGH);
            return true;
        case MODE_SLEEP:
            digitalWrite(_pin.M0, HIGH);
            digitalWrite(_pin.M1, HIGH);
            return true;
        default:
            return false;
    }
}

bool E32LoRaClass::waitAUX() {
    int ret_var = 0;
    uint8_t cnt = 0;
    while ((analogRead(_pin.AUX)) < 50 && (cnt++ < 100)) {
        delay(100);
    }

    if (cnt >= 100) {
        return false;
    }
    return true;
}

bool E32LoRaClass::begin(uint8_t mode) {
    switch (mode) {
        case MODE_NORMAL:
            digitalWrite(_pin.M0, LOW);
            digitalWrite(_pin.M1, LOW);
            return true;
        case MODE_WAKEUP:
            digitalWrite(_pin.M0, HIGH);
            digitalWrite(_pin.M1, LOW);
            return true;
        case MODE_POWER_SAVING:
            digitalWrite(_pin.M0, LOW);
            digitalWrite(_pin.M1, HIGH);
            return true;
        default:
            return false;
    }
}

int E32LoRaClass::listen() {
    delay(100);
    int len = 0;
    len = _e32_serial.available();
    _data_length = len;
    return len;
}

void E32LoRaClass::fetch( uint8_t *data ) {
    if (_data_length > 0) {
        for(int i=0; i<_data_length; i++) {
            data[i] = _e32_serial.read();
        }
    }
}

void E32LoRaClass::send( uint8_t *payload, uint8_t nb_byte ) {
    _e32_serial.write(payload, nb_byte);
    delay(10);
}

bool E32LoRaClass::setDataRate( int DR ) {
    switch (DR) {
        case 300:
            _para.SPEED = 0x18;
            break;
        case 1200:
            _para.SPEED = 0x19;
            break;
        case 2400:
            _para.SPEED = 0x1A;
            break;
        case 4800:
            _para.SPEED = 0x1B;
            break;
        case 9600:
            _para.SPEED = 0x1C;
            break;
        case 19200:
            _para.SPEED = 0x1D;
            break;
        default:
            return false;
    }

    return set();
}

bool E32LoRaClass::setAddress( uint8_t addr_h, uint8_t addr_l ) {
    _para.DEVICE_ADDRESS_HIGH = addr_h;
    _para.DEVICE_ADDRESS_LOW = addr_l;

    return set();

}

bool E32LoRaClass::setFrequency( int freq_mhz ) {
    // convert to hex
    _para.DEVICE_CHANNEL = String((freq_mhz - 410), HEX);
    
    return set();
}

bool E32LoRaClass::get( struct e32_t para) {
    setMode(MODE_SLEEP);

    if (!waitAUX()) return false;
    // Check setting paras
    while(_e32_serial.available()){_e32_serial.read();} //Clean Uart Buffer
    uint8_t read_cmd[3] = {0xC1, 0xC1, 0xC1};
    _e32_serial.write(read_cmd, 3);
    if (!waitAUX()) return false;
    uint8_t buff[6];
    uint8_t count = 0;
    count = _e32_serial.available();
    if (count) {
        for(int i=0; i<count; i++) {
            buff[i] = _e32_serial.read();
            delay(10);
        }
    }
    para.HEADER = buff[0];
    para.DEVICE_ADDRESS_HIGH = buff[1];
    para.DEVICE_ADDRESS_LOW = buff[2];
    para.DEVICE_CHANNEL = buff[3];
    para.SPEED = buff[4];
    para.OPTION = buff[5];

    return true;
}

int E32LoRaClass::getDataRate() {
    struct e32_t parameter;
    get(parameter);
    switch (parameter.SPEED) {
        case 0x18:
            return 300;
        case 0x19:
            return 1200;
        case 0x1A:
            return 2400;
        case 0x1B:
            return 4800;
        case 0x1C:
            return 9600;
        case 0x1D:
            return 19200;
        default:
            return 0;
    }
}

bool E32LoRaClass::getFrequency() {

}

bool E32LoRaClass::getAddressHigh() {
    struct e32_t parameter;
    get(parameter);
    return parameter.DEVICE_ADDRESS_HIGH;
}

bool E32LoRaClass::getAddressLow() {
    struct e32_t parameter;
    get(parameter);
    return parameter.DEVICE_ADDRESS_LOW;
}

E32LoRaClass E32LoRa;