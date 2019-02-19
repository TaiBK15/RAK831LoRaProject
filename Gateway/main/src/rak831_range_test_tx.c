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


#define test_SF7         DR_LORA_SF7

#define TOTAL_PACKET    30
const uint32_t FREQUENCY = 433500000; //FREQUENCY for test range
const char DATA_PAYLOAD[] = {0x01, 0x02, 0x03, 0x04, 0x05};
const char ACK_PAYLOAD[] = {0x11, 0x22, 0x33, 0x44, 0x55};

uint8_t cnt_pkg = 100;

int main(int argc, char **argv) {

    int pack_crc_ok = 0;
    int pack_crc_bad = 0;
    struct lgw_pkt_rx_s packet;

    // rak831_reset();
    if (rak831_config() == EXIT_FAILURE) return 0;
    if (rak831_start() == EXIT_FAILURE) return 0;


    for (uint8_t i = 1;i <= cnt_pkg; i++)
    {
	printf("***PACKET %d", i); 
        rak831_send(false ,DATA_PAYLOAD, FREQUENCY, BW_125KHZ, test_SF7, CR_LORA_4_5);
        sleep(5);
    }
   

    // #ifdef test_SF8
    // for (uint8_t i = 1; cnt_pkg; i++)
    // {
    //     rak831_send(false ,DATA_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF8, CR_LORA_4_5);
    //     sleep(4);
    // }
    // #endif

    // #ifdef test_SF9
    // for (uint8_t i = 1; cnt_pkg; i++)
    // {
    //     rak831_send(false ,DATA_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF9, CR_LORA_4_5);
    //     sleep(4);
    // }
    // #endif

    // #ifdef test_SF10
    // for (uint8_t i = 1; cnt_pkg; i++)
    // {
    //     rak831_send(false ,DATA_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    //     sleep(4);
    // }
    // #endif

    // #ifdef test_SF11
    // for (uint8_t i = 1; cnt_pkg; i++)
    // {
    //     rak831_send(false ,DATA_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    //     sleep(4);
    // }
    // #endif

    // #ifdef test_SF12
    // for (uint8_t i = 1; cnt_pkg; i++)
    // {
    //     rak831_send(false ,DATA_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    //     sleep(4);
    // }
    // #endif

   while(1);
}
    
    // implement protocol
    // Send control Pkt
   // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF12, CR_LORA_4_8);
    // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    // rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF8, CR_LORA_4_6);
    



void Gateway_Wait_ACK()
{
    struct lgw_pkt_rx_s packet;
    printf("Gateway wait for ACK");
        // Wait for ACK
    int nb_pkt = 0;
    int count = 0;
    int pack_crc_ok = 0;
    int pack_crc_bad = 0;


    do {        
        nb_pkt = rak831_listen();
        delay(100);
        // count ++;
        // sleep(1);
        // if ((count % 10) == 1) rak831_send(false ,CONTROL_PAYLOAD, FREQUENCY, BW_125KHZ, DR_LORA_SF7, CR_LORA_4_5);
    } while( nb_pkt == 0);

    for(int i = 0;i < nb_pkt;i++) 
    {
        memset(&packet, 0, sizeof(packet));
        packet = rak831_fetch_packets(i);
        if (packet.freq_hz == FREQUENCY) 
        {
            // Check CRC OK 
            if (packet.status == STAT_CRC_OK) {
                if( packet.size == sizeof(ACK_PAYLOAD) ) {
                    printf("CRC_OK\n");
                    pack_crc_ok++;
                }
            }
            else if (packet.status == STAT_CRC_BAD)
                  printf("CRC_BAD\n");
                    pack_crc_bad++;
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
