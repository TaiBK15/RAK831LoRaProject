#include <SoftwareSerial.h>
#include <SPI.h>
#include <LoRa.h>
//////////////UART NODE 1
#define UART_NODE_1_AddressH   0xBB
#define UART_NODE_1_AddressL   0x11
/////////////UART NODE 2
#define UART_NODE_2_AddressH   0xBB
#define UART_NODE_2_AddressL   0x22
char UART_ADDR_RECV_PKT_1[255] ={'b','b','1','1'};
char UART_ADDR_RECV_PKT_2[255] ={'b','b','2','2'};
/////////////////////////////////////////////////

 #define DEBUG_SERIAL
#define SPI_LORA_MODULE

#define HEADER              0xC0
#define RELAY_NODE_ADDR_UART_H   0xBB
#define RELAY_NODE_ADDR_UART_L   0x33
#define RELAY_NODE_CHANNEL  0x17

#define SPEED           0x1D  // 19.2 KBPS
#define OPTION          0xC4  //20dBm

#define M0pin   A0
#define M1pin   A1
#define AUXpin  A2
#define SOFT_RX 7
#define SOFT_TX 6
#define TIMEOUT_RET   2

#define LEDTX_U   3
#define LEDRX_U   A3
#define LEDTX_S 5
#define LEDRX_S 4
#define LED_TX      true
#define LED_RX      true

//////////////SPI
#define RELAY_NODE_ADDR_SPI_H 0xAA
#define RELAY_NODE_ADDR_SPI_L 0x03

#define FREQUENCY   434.5E6
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
char SPI_ADDR_RECV_PKT[255] ={'a','a','0','3'};
///////////////

SoftwareSerial LoraSerial(SOFT_RX, SOFT_TX);
/*-----------------------------------------------
    setup
------------------------------------------------*/
void setup() {
   

    // Led rx,tx 
    ledConfig();

    //Serial Init
    #ifdef DEBUG_SERIAL
    Serial.begin(9600);
    Serial.println("Serial are ready");
    #endif // DEBUG_SERIAL

    // uart lora config
    if (!e32Config()) while(1);
    ledToggle(2, LED_TX, LED_RX, 500, true);

    SPILoRaConfig();
    ledToggle(2, LED_TX, LED_RX, 500, false);

    /* Register the receive callback */
    LoRa.onReceive(onReceive);

    /* Put the radio into receive mode */
    LoRa.receive();
}
///////////////////////////////////////////////////////
/*-----------------------------------------------
    FUNCTIONS
-------------------------------------------------*/
bool AUX_HL;
bool ReadAUX() {
    int val = analogRead(AUXpin);
    if(val<50){
        AUX_HL = LOW;
    }else {
        AUX_HL = HIGH;
    }
    return AUX_HL;
}

int WaitAUX_H() {
    int ret_var = 0;
    uint8_t cnt = 0;
    while ((ReadAUX() == LOW) && (cnt++ < 100)){
        #ifdef DEBUG_SERIAL
        Serial.print(".");
        #endif // DEBUG_SERIAL
        delay(100);
    }
    if (cnt >= 100 ) {
        ret_var = TIMEOUT_RET;
        #ifdef DEBUG_SERIAL
        Serial.println("Timeout!");
        #endif // DEBUG_SERIAL
    }
    return ret_var;
}

void ledConfig() {
    pinMode(LEDTX_U, OUTPUT);
    pinMode(LEDRX_U, OUTPUT);
    pinMode(LEDTX_S, OUTPUT);
    pinMode(LEDRX_S, OUTPUT);
}

void ledToggle(uint8_t count, bool ledTx, bool ledRx, int delayms, bool is_uart) {
    for(int i=0; i<count; i++){
        if (!is_uart) {
            if (ledTx) digitalWrite(LEDTX_U, HIGH);
            if (ledRx) digitalWrite(LEDRX_U, HIGH);
            delay(delayms);
            if (ledTx) digitalWrite(LEDTX_U, LOW);
            if (ledRx) digitalWrite(LEDRX_U, LOW);    
            delay(delayms);
        }
        else {
            if (ledTx) digitalWrite(LEDTX_S, HIGH);
            if (ledRx) digitalWrite(LEDRX_S, HIGH);
            delay(delayms);
            if (ledTx) digitalWrite(LEDTX_S, LOW);
            if (ledRx) digitalWrite(LEDRX_S, LOW);    
            delay(delayms);
        }
       
    }
}
bool e32Config() {
    // pin Mode
    pinMode(M0pin, OUTPUT);
    pinMode(M1pin, OUTPUT);
    pinMode(AUXpin, INPUT);
    LoraSerial.begin(9600);

    // configure
    digitalWrite(M0pin, HIGH);
    digitalWrite(M1pin, HIGH);
    delay(10);
    if(WaitAUX_H() == TIMEOUT_RET) {exit(0);}
    uint8_t CMD[6] = {HEADER, RELAY_NODE_ADDR_UART_H, RELAY_NODE_ADDR_UART_L, SPEED, RELAY_NODE_CHANNEL, OPTION};
    LoraSerial.write(CMD, 6);
    WaitAUX_H();
    delay(1200);
    #ifdef DEBUG_SERIAL
    Serial.println("Setting Configure has been sent!");
    #endif // DEBUG_SERIAL

    //Read Set configure
    while(LoraSerial.available()){LoraSerial.read();} //Clean Uart Buffer
    uint8_t READCMD[3] = {0xC1, 0xC1, 0xC1};
    LoraSerial.write(READCMD, 3);
    WaitAUX_H();
    delay(50);
    #ifdef DEBUG_SERIAL
    Serial.println("Reading Configure has been sent!");
    #endif // DEBUG_SERIAL
    uint8_t readbuf[6];
    uint8_t Readcount, idx, buff;
    Readcount = LoraSerial.available();
    if (Readcount){
        #ifdef DEBUG_SERIAL
        Serial.println("Setting Configure is:  ");
        #endif // DEBUG_SERIAL
        for(idx = 0; idx < Readcount; idx++) {
            readbuf[idx] = LoraSerial.read();
            delay(10);
        }
        #ifdef DEBUG_SERIAL
        Serial.print("Shutdown Mode: ");Serial.println(0xFF & readbuf[0],HEX);
        Serial.print("Address High: ");Serial.println(0xFF & readbuf[1],HEX);
        Serial.print("Address Low: ");Serial.println(0xFF & readbuf[2],HEX);
        Serial.print("Speed: ");Serial.println(0xFF & readbuf[3],HEX);
        Serial.print("Channel: ");Serial.println(0xFF & readbuf[4],HEX);
        Serial.print("Option: ");Serial.println(0xFF & readbuf[5],HEX);
        #endif // DEBUG_SERIAL

        if ((readbuf[0] == HEADER) && (readbuf[1] == RELAY_NODE_ADDR_UART_H ) && (readbuf[2] == RELAY_NODE_ADDR_UART_L )  
                      && (readbuf[3] == SPEED) && (readbuf[4] == RELAY_NODE_CHANNEL ) && (readbuf[5] == OPTION) ) {
            ledToggle(3, true, true, 500, true);
            digitalWrite(M0pin, LOW);
            digitalWrite(M1pin, LOW);
            #ifdef DEBUG_SERIAL
            Serial.println("---------------SETTING DONE-------------");
            #endif // DEBUG_SERIAL
            return true;
        }
    }
    return false;
}

int e32Listen() {
    delay(100);
    int data_len;
    data_len = LoraSerial.available();
    if (data_len > 0) {
        #ifdef DEBUG_SERIAL
        Serial.print("ReceiveMsg: ");  Serial.print(data_len);  Serial.println(" bytes.");
        #endif // DEBUG_SERIAL
        ledToggle(1, false, true, 500, true);
    }
    return data_len;
}

void e32Fetch(int data_len, uint8_t *pdata) {
    if (data_len > 0) {
        for(int i=0;i<data_len;i++) {
            pdata[i] = LoraSerial.read();
            #ifdef DEBUG_SERIAL
            Serial.print(pdata[i]);
            Serial.print(" ");
            #endif // DEBUG_SERIAL
        }
        #ifdef DEBUG_SERIAL
        Serial.println("");
        #endif // DEBUG_SERIAL
    }
}

void e32Send(uint8_t *payload, uint8_t nb_byte) {
    // uint8_t FrameSend[8] = {GW_AddressH, GW_AddressL, GW_Channel, RELAY_NODE_ADDR_UART_H, RELAY_NODE_ADDR_UART_L, 0x01, 0x02, 0x03};
    LoraSerial.write(payload, nb_byte);
    #ifdef DEBUG_SERIAL
    Serial.println("Data hasbeen sent!");
    #endif // DEBUG_SERIAL
    ledToggle(1, true, false, 500, true);
    delay(10);

}


////////////////////////////////////////////////
/*-----------------------------------------------
    spi
------------------------------------------------*/
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
    char buff[255];
    for (int i = 0; i < 12; i ++) {
        buff[i] =(char)LoRa.read();
    }
    // check spi Address
    for (int i = 0; i < 4; i ++) {
      if (buff[i] != SPI_ADDR_RECV_PKT[i]) return false;
    }

    bool flag_check_uart_addr_1 = true;
    bool flag_check_uart_addr_2 = true;
    for (int i = 4; i < 8; i ++) {
      if (buff[i] != UART_ADDR_RECV_PKT_1[i-4]) flag_check_uart_addr_1 = false;
    }
    for (int i = 4; i < 8; i ++) {
      if (buff[i] != UART_ADDR_RECV_PKT_2[i-4]) flag_check_uart_addr_2 = false;
    }
    if ((flag_check_uart_addr_1 == true) && (flag_check_uart_addr_2 == false) ) {
        // Send to node uart 1
        Serial.println("DEBUG: HERE 1");

        uint8_t FAKE_PAYLOAD[] = {UART_NODE_1_AddressH, UART_NODE_1_AddressL, RELAY_NODE_CHANNEL,RELAY_NODE_ADDR_SPI_H, RELAY_NODE_ADDR_SPI_L,  0x01, 0x02};
        e32Send(FAKE_PAYLOAD, sizeof(FAKE_PAYLOAD));
    }
    else if ((flag_check_uart_addr_1 == false) && (flag_check_uart_addr_2 == true) ) {
        // Send to node uart 2
    Serial.println("DEBUG: HERE 2");

        uint8_t FAKE_PAYLOAD[] = {UART_NODE_2_AddressH, UART_NODE_2_AddressL, RELAY_NODE_CHANNEL,RELAY_NODE_ADDR_SPI_H, RELAY_NODE_ADDR_SPI_L,  0x01, 0x02};
        e32Send(FAKE_PAYLOAD, sizeof(FAKE_PAYLOAD));
    }
    else {
        // do nothing
    Serial.println("DEBUG: HERE 3");

        return false;
    }
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

/////////////////////////////////////////////////
/*-----------------------------------------------
    LOOP
------------------------------------------------*/
void loop() {
    // uint8_t FAKE_PAYLOAD[] = {UART_NODE_AddressH, UART_NODE_AddressL, RELAY_NODE_CHANNEL,RELAY_NODE_ADDR_SPI_H, RELAY_NODE_ADDR_SPI_L,  0x01, 0x02};
    // e32Send(FAKE_PAYLOAD, sizeof(FAKE_PAYLOAD));
    // delay(8000);
    uint8_t address_uart_1[] = {UART_NODE_1_AddressH, UART_NODE_1_AddressL};
    uint8_t address_uart_2[] = {UART_NODE_2_AddressH, UART_NODE_2_AddressL};
    uint8_t data_len = 0;
    while (data_len == 0)
        data_len = e32Listen();
    if (data_len > 0) {
        uint8_t data[data_len];
        e32Fetch(data_len, data);
        bool flag_address_uart_1 = true, flag_address_uart_2 = true;
        for(uint8_t i=0; i<2; i++) {
            if (data[i] != address_uart_1[i]) flag_address_uart_1 = false;
            else if (data[i] != address_uart_2[i]) flag_address_uart_2 = false;
            else;
        }
        if ((flag_address_uart_1 == true) && (flag_address_uart_2 == false) ) {
            // send to gateway with information of node 1 uart
            LoRa.idle();
            uint8_t packet_uart_1[] = {RELAY_NODE_ADDR_SPI_H, RELAY_NODE_ADDR_SPI_L, UART_NODE_1_AddressH, UART_NODE_1_AddressL, 0x11, 0x22};
            SPILoRaSendPackets(1, packet_uart_1, sizeof(packet_uart_1));
            LoRa.receive();
        }
        else if ((flag_address_uart_1 == false) && (flag_address_uart_2 == true) ) {
            // send to gateway with information of node 2 uart
            LoRa.idle();
            uint8_t packet_uart_2[] = {RELAY_NODE_ADDR_SPI_H, RELAY_NODE_ADDR_SPI_L, UART_NODE_2_AddressH, UART_NODE_2_AddressL, 0x33, 0x44};
            SPILoRaSendPackets(1, packet_uart_2, sizeof(packet_uart_2));
            LoRa.receive();
        }
    }
}
//////////////////////////////////////////////////////////////
/* Handle received packets */
void onReceive(int packetSize) {
    #ifdef DEBUG_SERIAL
    Serial.print("Received packet '");
    #endif
    ledToggle(1, false, LED_RX, 500, false);
    // read packet
    if (SPILoRaCheckPacket(ADDRESSED_PACKET) == true ) {
    }
  // print RSSI of packet
    #ifdef DEBUG_SERIAL
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    #endif

}
//////////////////////////////////////////////////////////////
