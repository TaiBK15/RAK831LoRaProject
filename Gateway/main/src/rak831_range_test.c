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
#include <pthread.h>	/* thread */

#define TOTAL_PACKET    30
const uint32_t FREQUENCY = 433500000; //FREQUENCY for test range
const char CONTROL_PAYLOAD[] = {0x01, 0x02, 0x03, 0x04, 0x05};
const char ACK_PAYLOAD[] = {0x11, 0x22, 0x33, 0x44, 0x55};

int main(int argc, char **argv) {

    // rak831_reset();
    if (rak831_config() == EXIT_FAILURE) return 0;
    if (rak831_start() == EXIT_FAILURE) return 0;
    
    // implement protocol
    // Send control Pkt
   // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF12, CR_LORA_4_8);
    // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF8, CR_LORA_4_6);

    sleep(5);
    // Wait for ACK
    int nb_pkt = 0;
    int count = 0;
    int pack_crc_ok = 0;
    int pack_crc_bad = 0;
    struct lgw_pkt_rx_s packet;
    printf("Gateway wait for data");
    do {        
        nb_pkt = rak831_listen();
        delay(100);
        // count ++;
        // sleep(1);
        // if ((count % 10) == 1) rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    } while( nb_pkt == 0);

    bool loop = true;
    while(loop) {
        for(int i = 0;i < nb_pkt;i++) {
            memset(&packet, 0, sizeof(packet));
            packet = rak831_fetch_packets(i);
            if (packet.freq_hz == FREQUENCY) {
                // Check CRC OK 
                if (packet.status == STAT_CRC_OK) {
                    if( packet.size == sizeof(ACK_PAYLOAD) ) {
                        bool flag = true;
                        for(int j = 0; j < packet.size; j++) {
                            if (packet.payload[j] != ACK_PAYLOAD[j]) flag = false;
                        }
                        if (flag == false) {
                            printf("CRC_OK\n");
                            pack_crc_ok++;
                            printf("pack_crc_ok: %d \n", pack_crc_ok);
                        }
                        else {
                            printf("RECEIVED ACK PACKET!\n");
                            loop = false;   // exit loop
                        }
                    } else {
                        printf("CRC_OK\n");
                        pack_crc_ok++;
                        printf("pack_crc_ok1: %d \n", pack_crc_ok);
                    }
                }
                // Check CRC BAD
                else if (packet.status == STAT_CRC_BAD) {
                    printf("CRC_BAD\n");
                    pack_crc_bad++;
                    printf("pack_crc_bad: %d\n", pack_crc_bad);
                }
                else {
                    printf("DEBUG: NO CRC\n");
                }
            }
        }

        // Listen 
        nb_pkt = 0;
        while (nb_pkt == 0 && loop == true) {
            nb_pkt = rak831_listen();
            // sleep(1);
        }
    }

        // Calculate PRR
        printf("========================Calculate============================\n");
        double PRR = (double)pack_crc_ok / TOTAL_PACKET;
        printf("PRR: %lf\n", PRR);
        double PDR = (double) (pack_crc_ok + pack_crc_bad) / TOTAL_PACKET;
        printf("PDR: %lf\n", PDR);

        rak831_stop();
}
