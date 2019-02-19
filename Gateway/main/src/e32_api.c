#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <strings.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "parson.h"
#define LOW 0
#define HIGH 1
#define MAXBUF 256
#define PIN_AUX		22
#define PIN_M0		17  
#define PIN_M1		27
#define TIMEOUT_RET	2

#define HEADER          0xC0
#define GW_AddressH		0x05
#define GW_AddressL 	0x01
#define SPEED           0x18	//0.3
// #define SPEED           0x1D	//19.2
#define GW_Channel		0x17
#define OPTION          0xC6 	//14dBm
#define OPTION          0xC4  	//20dBm

int fd;

int e32_config() {
    printf("e32Config\n");
    wiringPiSetupGpio();
    pinMode(PIN_M0, OUTPUT);	delay(500);
	pinMode(PIN_M1, OUTPUT);	delay(500);
	pinMode(PIN_AUX,INPUT);	delay(500);
	/*serial init*/
	if((fd = serialOpen ("/dev/ttyAMA0", 9600)) < 0 ){
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
    }
    else    printf("Serial Setup is done!\n");
    /*Setup Lora E32 Module - Send command through UART*/
    digitalWrite(PIN_M0,HIGH);	//MODE 3 - SLEEP
	digitalWrite(PIN_M1,HIGH);
    printf("Mode 3 - Sleep\n");
	delay(1000);
    // if(WaitAUX_H() == TIMEOUT_RET) {exit(0);}

    /*Write CMD for LORA configuration*/
	//Header | Address High | Address Low | SPEED | Channel | Option
	uint8_t CMD[6] = {HEADER, GW_AddressH, GW_AddressL, SPEED, GW_Channel, OPTION};
	write(fd, CMD, 6);
    printf("DEBUG: here\n");
	WaitAUX_H();
	delay(1200);
	printf("Setting Configure has been sent!\n");

	/*Read Set configure*/
	while(serialDataAvail(fd)){	/*Clean Uart Buffer*/
		serialGetchar(fd);
	}
	uint8_t READCMD[3] = {0xC1, 0xC1, 0xC1};	/*Send 3 C1 to read*/
	write(fd, READCMD, 3);
	WaitAUX_H();
	delay(50);
	printf("Reading Configure Command has been sent!\n");

	uint8_t readbuf[6];
	uint8_t Readcount, idx, buff;
	Readcount = serialDataAvail(fd);
	if (Readcount == 6){
		printf("Setting Configure is:  ");
		for(idx = 0; idx < 6; idx++){
			readbuf[idx] = serialGetchar(fd);
			printf("%x ",0xFF & readbuf[idx]);
			delay(10);
		}
		if ((readbuf[0] == HEADER) && (readbuf[1] == GW_AddressH) && (readbuf[2] == GW_AddressL)
			&& (readbuf[3] == SPEED) && (readbuf[4] == GW_Channel) && (readbuf[5] == OPTION)) {
				printf("\n");
				printf("---------------SETTING DONE-------------\n");
				return EXIT_SUCCESS;
			}
		else {
			printf("Failed to config e32 lora module\n");
			return EXIT_FAILURE;
		}
	}
	return EXIT_FAILURE;
}

int e32_start() {
	//Setting Mode first
	printf("Start e32 - ttl - 100 uart lora module!\n");
	digitalWrite(PIN_M0,LOW);	//MODE 0 - NORMAL
	digitalWrite(PIN_M1,LOW);
	printf("Mode 0: Normal\n");
	return EXIT_SUCCESS;

}

int e32_listen() {
	delay(100);
	uint8_t data_len;
	data_len = serialDataAvail(fd);
	if (data_len > 0)	{
		printf("Received packet from E32-TTL-100 UART NODE\n");
		return data_len;
	}
	else return 0;
}

void e32_send(uint8_t *payload, uint8_t nb_byte) {
    printf("e32Send");
	delay(100);
	// uint8_t Senda[6] = {0x05, 0x02, 0x17, 0x05,0x04,0x08, 0x04, 0x05, 0x06};
	// write(fd,Senda,6);
	write(fd,payload, nb_byte);
	printf("Packet has been sent to node\n");
}

void e32_fetch( uint8_t data_len, uint8_t* pdata ) {
    printf("e32Fetch");
	for(int i = 0; i < data_len; i++)	{
		pdata[i] = serialGetchar(fd);
	}
	printf("Receive %d bytes from Lora NodeID: %d%d\n",data_len, pdata[0], pdata[1]);
}


/*Function GPIO: Check whether AUX high or not? */
int ReadAUX(){
	int AUX_HL;
	if(!digitalRead(PIN_AUX)){
		AUX_HL = LOW;
	}
	else{
		AUX_HL = HIGH;
	}
	return(AUX_HL);
}
int WaitAUX_H(){
	int ret_var;
	uint8_t cnt = 0;
	while ((ReadAUX() == LOW) && (cnt++ < 100)){
		printf(".");
		delay(100);
	}
	printf("\n");	

	if (cnt >= 100 ) {
		ret_var = TIMEOUT_RET;
		printf("Timeout!\n");
	}
	
	return ret_var;
}