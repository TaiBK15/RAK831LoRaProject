#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf sprintf fopen fputs */

#include <string.h>     /* memset */
#include <signal.h>     /* sigaction */
#include <time.h>       /* time clock_gettime strftime gmtime clock_nanosleep*/
#include <unistd.h>     /* getopt access */
#include <stdlib.h>     /* atoi */

#include "loragw_hal.h"
#include "rak831_api.h"
#include "e32_api.h"
#include <pthread.h>    /* thread */

#include <sys/socket.h> /* socket */
#include <netinet/in.h> 
#define PORT 8080
int sock = 0;

#define TOTAL_PACKET    30

uint8_t channel_define();

const char CONTROL_PAYLOAD[] = {0x01, 0x02, 0x03, 0x04, 0x05};
const char ACK_PAYLOAD[] = {0x11, 0x22, 0x33, 0x44, 0x55};

/** configSocketAndConnectServer()
To Config Socket
*/
bool configSocketAndConnectServer() {
    struct sockaddr_in address; 
    struct sockaddr_in serv_addr; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("CONFIG SOCKET:: Socket creation error \n"); 
        return false; 
    } 
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0){ 
        printf("CONFIG SOCKET:: Invalid address/ Address not supported \n"); 
        return false; 
    } 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nCONNECT SOCKET:: Connection Failed \n"); 
        return false; 
    }
    printf("CONNECT SOCKET:: Connection Succeeded. \n");
    return true;
}

/** sendPacketsToServer()
Send packets to Server 
@param payload_to_server: payload to send to server
*/
void sendPacketsToServer(char* payload_to_server) {
    size_t payload_size = 0;
    payload_size = findPayloadSize(payload_to_server);
    send(sock , payload_to_server , payload_size , 0 ); 
}

int main(int argc, char **argv) {


    // rak831_reset();
    if (rak831_config() == EXIT_FAILURE) return 0;
    if (rak831_start() == EXIT_FAILURE) return 0;
    if (!configSocketAndConnectServer()) exit(EXIT_FAILURE);


    
    // implement protocol
    // Send control Pkt
   // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF12, CR_LORA_4_8);
    // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF8, CR_LORA_4_6);

    // Wait for ACK
    int nb_pkt = 0;
    int count = 0;
    int pack_crc_ok = 0;
    int pack_crc_bad = 0;
    struct lgw_pkt_rx_s packet;
    printf("Gateway wait for data\n");
    do {        
        nb_pkt = rak831_listen();
        // delay(100);
        // count ++;
        // sleep(1);
        // if ((count % 10) == 1) rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    } while( nb_pkt == 0);
    char data[1024];
    bool loop = true;
    uint8_t chan_num;
    while(loop) {
        for(int i = 0;i < nb_pkt;i++) {
            printf("\n");
            printf("packet: %d\n", i );
            memset(&packet, 0, sizeof(packet));
            packet = rak831_fetch_packets(i);
            // Check CRC OK 
            if (packet.status == STAT_CRC_OK) {
                chan_num = channel_define(packet.freq_hz);
                printf("-------------------CHANNEL %d-------------------\n", chan_num);
                printf("0x%x 0x%x 0x%x \n", packet.payload[0], packet.payload[1], packet.payload[2] );
                printf("FREQUENCY: %d; RSSI: %f; SNR: %f\n", packet.freq_hz, packet.rssi, packet.snr );
                
                sprintf(data, "{CHANNEL: %d, Packet: %x, %x, %x; FREQUENCY: %d, RSSI: %f, SNR: %f\n}",
                    chan_num, packet.payload[0], packet.payload[1], packet.payload[2], packet.freq_hz, packet.rssi, packet.snr);
                sendPacketsToServer(data);

            }
            // Check CRC BAD
            else if (packet.status == STAT_CRC_BAD) {
                printf("CRC_BAD\n");
            }
            else {
                printf("DEBUG: NO CRC\n");
            }
        }

        // Listen 
        nb_pkt = 0;
        while (nb_pkt == 0 && loop == true) {
            nb_pkt = rak831_listen();
        }
    }
        rak831_stop();
}

uint8_t channel_define(uint32_t freq)
{
    uint8_t chan;
    switch(freq)
    {
        case 433100000:
            chan = 0; break;
        case 433300000:
            chan = 1; break;
        case 433500000:
            chan = 2; break;
        default:
            chan = 3;
    }
    return chan;
}
