#include <SPI.h>
#include <LoRa.h>

// DEFINE 
// #define DEBUG_SERIAL
#define SPI_LORA_MODULE

#define LED_TX_PIN   3
#define LED_RX_PIN   5
#define LED_TX      true
#define LED_RX      true

#define FREQUENCY   433.5E6
//#define SF          12
// #define SF          7
 #define SF          8
#define BW          125E3
//#define CR          8
// #define CR          5
 #define CR          6
#define PL          8   // Preample Length
#define SYNCWORD    0x34

#define PACKET_CONTROL  1
#define PACKET_ACK      2

uint8_t CONTROL_PAYLOAD[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t ACK_PAYLOAD[5] = {0x11, 0x22, 0x33, 0x44, 0x55};

//SETUP
void setup() {
    #ifdef DEBUG_SERIAL
    Serial.begin(9600);
    while(!Serial);
    Serial.println("Setup Serial Success!");
    #endif

    ledConfig();

    #ifdef SPI_LORA_MODULE
    SPILoRaConfig();
    #else  // UART_LORA_MODULE
    UARTLoRaConfig();
    #endif // SPI & UART_LORA_MODULE

    delay(1000);
    ledToggle(5, LED_TX, LED_RX, 500);
}

void loop() {
    #ifdef DEBUG_SERIAL
    Serial.println("Waiting for controlled packets from gateway...");
    #endif // DEBUG_SERIAL

    // SPI LORA
    do {
        SPILoRaListen();
        ledToggle(1, false, LED_RX, 500);
    } while( !SPILoRaCheckPacket(PACKET_CONTROL) );

    uint8_t pack_test[50] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 
                            0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40,
                            0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50}; 
    SPILoRaSendPackets(30, pack_test, sizeof(pack_test));
    SPILoRaSendPackets(30, ACK_PAYLOAD, sizeof(ACK_PAYLOAD));
    do {
        SPILoRaListen();
    } while( !SPILoRaCheckPacket(PACKET_ACK) );
}

void ledConfig() {
    pinMode(LED_TX_PIN, OUTPUT);
    pinMode(LED_RX_PIN, OUTPUT);
}

void ledToggle(uint8_t count, bool ledTx, bool ledRx, int delayms) {
    for(int i=0; i<count; i++){
        if (ledTx) digitalWrite(LED_TX_PIN, HIGH);
        if (ledRx) digitalWrite(LED_RX_PIN, HIGH);
        delay(delayms);
        if (ledTx) digitalWrite(LED_TX_PIN, LOW);
        if (ledRx) digitalWrite(LED_RX_PIN, LOW);    
        delay(delayms);
    }
}

void SPILoRaConfig() {
    if (!LoRa.begin(FREQUENCY)) {   // failed to start LoRa
        #ifdef DEBUG_SERIAL
        Serial.println("Starting LoRa failed!");
        #endif // DEBUG_SERIAL
        while(1);
    }
    LoRa.setSpreadingFactor(SF);
    LoRa.setSignalBandwidth(BW);
    LoRa.setCodingRate4(CR);
    LoRa.setPreambleLength(PL);
    LoRa.setSyncWord(SYNCWORD);
    LoRa.enableCrc();

    #ifdef DEBUG_SERIAL
    Serial.println("Setting done!!");
    #endif // DEBUG_SERIAL
}

void UARTLoRaConfig() {
    // TODO
}

bool UARTLoRaListen() {
    // TODO
}

bool UARTLoRaCheckPacket() {
    // TODO
}

void UARTLoRaSendPacket() {
    // TODO
}

bool SPILoRaListen() {
    int packetSize = 0;
    while(packetSize == 0) {
        packetSize = LoRa.parsePacket();
    }
    #ifdef DEBUG_SERIAL
    Serial.println("Received packet!");
    #endif // DEBUG_SERIAL
    return true;
}

bool SPILoRaCheckPacket(int pktType) {
    // PACKET_CONTROL: {0x00, 0x01} 
    // PACKET_ACK: {0x01, 0x02}
    int i = 0;
    while ( LoRa.available() ) {
        if (pktType == PACKET_CONTROL) {
            #ifdef  DEBUG_SERIAL
            Serial.print("Received Packets...... ");
            #endif
            if ( LoRa.read() != CONTROL_PAYLOAD[i] ) return false;
        }
        else if (pktType == PACKET_ACK) {
            if (LoRa.read() != ACK_PAYLOAD[i] ) return false;
        }
        else return false;
        i++;        
    }
    ledToggle(3, LED_TX, LED_RX, 500);
    return true;
}

void SPILoRaSendPackets(int n, uint8_t *payload, uint8_t nb_byte) {
    #ifdef  DEBUG_SERIAL
    Serial.print("Sending packet...... ");
    #endif

    while (n != 0) {
        n--;
        LoRa.beginPacket();
        LoRa.write(payload, nb_byte);
        LoRa.endPacket();
    }
    #ifdef  DEBUG_SERIAL
    Serial.println("Done!");
    #endif
}
