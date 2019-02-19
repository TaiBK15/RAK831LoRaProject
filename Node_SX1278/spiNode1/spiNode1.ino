// gui 3 byte: 2 byte address, oxaa ;   ox11

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <BH1750FVI.h>
BH1750FVI LightSensor;
// DEFINE 
 #define DEBUG_SERIAL
#define SPI_LORA_MODULE

#define LED_TX_PIN   3
#define LED_RX_PIN   5
#define LED_TX      true
#define LED_RX      true

#define M0PIN   A0
#define M1PIN   A1

#define ADDR_SPI_H 0xAA
#define ADDR_SPI_L 0x01
#define NODE_DATA 0x11
char buff[10];
//char DATA_RECV_PKT[] = "01";


#define FREQUENCY   433.1E6
 #define SF          7
#define BW          125E3
#define CR          5
#define PL          8   // Preample Length
#define SYNCWORD    0x12

#define PACKET_CONTROL  1
#define PACKET_ACK      2
#define ADDRESSED_PACKET 3

uint8_t CONTROL_PAYLOAD[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t ACK_PAYLOAD[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
char ADDR_RECV_PKT[255] ={'a','a','0','1'};
char ON_RECV_PKT[255] ={'0','2'};
char HALF_RECV_PKT[255] ={'0','1'};
char OFF_RECV_PKT[255] ={'0','0'};

volatile bool mode_auto = true;
uint8_t status_led = 0x00;  // off

//SETUP
void setup() {
    #ifdef DEBUG_SERIAL
    Serial.begin(9600);
    while(!Serial);
    Serial.println("Setup Serial Success!");
    #endif

    ledConfig();
    pinMode(M0PIN, OUTPUT);
    pinMode(M1PIN, OUTPUT);
    #ifdef SPI_LORA_MODULE
    SPILoRaConfig();
    #else  // UART_LORA_MODULE
    UARTLoRaConfig();
    #endif // SPI & UART_LORA_MODULE

    /* Register the receive callback */
    LoRa.onReceive(onReceive);

    /* Put the radio into receive mode */
    LoRa.receive();

    delay(1000);
    ledToggle(5, LED_TX, LED_RX, 500);
    LightSensor.begin();
    LightSensor.SetAddress(Device_Address_L);
    LightSensor.SetMode(Continuous_H_resolution_Mode);
}
////////////////////////////////
//loop/
////////////////////////////////

void loop() {
    delay(5000);
    LoRa.idle();
    #ifdef DEBUG_SERIAL
    Serial.println("Sending packet to gateway... ");
    #endif
    // READ SENSOR
    uint16_t lux = LightSensor.GetLightIntensity();// Get Lux value
    Serial.print("Light: ");Serial.print(lux); Serial.println(" lux");
    if (mode_auto) {
        if (lux > 50) {
            // turn off light
            digitalWrite(M0PIN, LOW);
            digitalWrite(M1PIN, LOW);
            Serial.println("Auto Mode: Turn off light");
            status_led = 0x00;
        }

        else {  //open light 
            digitalWrite(M0PIN, HIGH);
          digitalWrite(M1PIN, HIGH);
          Serial.println("Auto Mode: Turn on light");
            status_led = 0x01;
            }
        }
    uint8_t pack_test[3] = {ADDR_SPI_H, ADDR_SPI_L, status_led};
    SPILoRaSendPackets(1, pack_test, sizeof(pack_test));
    LoRa.receive();

}

//////////////////////////////////////////////////////////////
/* Handle received packets */
void onReceive(int packetSize) {
    #ifdef DEBUG_SERIAL
    Serial.print("Received packet '");
    #endif
    // read packet
    if (SPILoRaCheckPacket(ADDRESSED_PACKET) == true ) {
        ledToggle(1, false, LED_RX, 500);
        Serial.println("adress oke");
  }
  // print RSSI of packet
    #ifdef DEBUG_SERIAL
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    #endif

}
//////////////////////////////////////////////////////////////


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

int SPILoRaListen() {
    int packetSize = 0;
    while(packetSize == 0) {
        packetSize = LoRa.parsePacket();
    }
    #ifdef DEBUG_SERIAL
    Serial.println("Received packet!");
    #endif // DEBUG_SERIAL
    ledToggle(1, false, LED_RX, 500);
    return packetSize;
}

bool SPILoRaCheckPacket(int pktType) {
    // PACKET_CONTROL: {0x00, 0x01} 
    // PACKET_ACK: {0x01, 0x02}
    
    for (int i = 0; i < 8; i ++) {
      buff[i] =(char)LoRa.read();
    }

    bool flag_check_addr = true;
    for (int i = 0; i < 4; i ++) {
      if (buff[i] != ADDR_RECV_PKT[i]) flag_check_addr = false;
    }
    if (flag_check_addr == true) {
     
      bool flag_on = true, flag_off = true, flag_half = true;
        for (int i = 4; i < 6; i++) {
            if( buff[i] != ON_RECV_PKT[i-4]) {
              flag_on = false;
            }
        }

        for (int i = 4; i < 6; i++) {
            if( buff[i] != OFF_RECV_PKT[i-4]) {
              flag_off = false;
            }
        }

        for (int i = 4; i < 6; i++) {
            if( buff[i] != HALF_RECV_PKT[i-4]) {
              flag_half = false;
            }
        }

        if ((flag_on == true) && (flag_off == false) && (flag_half == false)) {
          digitalWrite(M0PIN, HIGH);
          digitalWrite(M1PIN, HIGH);
          Serial.println("100% light ");
          status_led = 0x01;
        }
        else if ((flag_on == false) && (flag_off == true) && (flag_half == false)) {
          digitalWrite(M0PIN, LOW);
          digitalWrite(M1PIN, LOW);
          Serial.println("0% light '");
          status_led = 0x00;
        }
        else if ((flag_on == false) && (flag_off == false) && (flag_half == true)) {
          digitalWrite(M0PIN, LOW);
          digitalWrite(M1PIN, HIGH);
          Serial.println("50% light '");
          status_led = 0x02;
        }
        else {
        }

        // check mode auto or manual?
        char mode_auto_char[] = {'1', '1'};
        char mode_manual_char[] = {'2', '2'};
        bool flag_mode_auto = true, flag_mode_manual = true;
        for (int i = 6; i < 8; i++) {
            if (buff[i] != mode_auto_char[i-6]) flag_mode_auto = false;
            else if (buff[i] != mode_manual_char[i-6]) flag_mode_manual = false;
        }
        if ((flag_mode_auto == true) && (flag_mode_manual == false)) {
            mode_auto = true;
        }
        else if ((flag_mode_auto == false) && (flag_mode_manual == true)) {
            mode_auto = false;
        }
    }
    else return false;
    
    return true;
}

void SPILoRaSendPackets(int n, uint8_t *payload, uint8_t nb_byte) {
    #ifdef  DEBUG_SERIAL
    Serial.print("Sending packet...... ");
    #endif

    while (n != 0) {
        n--;
        digitalWrite(LED_TX_PIN, HIGH);
        LoRa.beginPacket();
        LoRa.write(payload, nb_byte);
        LoRa.endPacket();
        delay(1000);
        digitalWrite(LED_TX_PIN, LOW);
    }

    #ifdef  DEBUG_SERIAL
    Serial.println("Done!");
    #endif
}
