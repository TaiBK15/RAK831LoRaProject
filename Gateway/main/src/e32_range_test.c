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

#define NODE_ADDRESSH   0x05
#define NODE_ADDRESSL   0x02
#define NODE_Channel    0x17

#define TOTAL_PACKET    30
const uint8_t ACK_PAYLOAD[] = {NODE_ADDRESSH, NODE_ADDRESSL, 0x11, 0x22};
const uint8_t CONTROL_PAYLOAD[5] = {NODE_ADDRESSH, NODE_ADDRESSL, NODE_Channel, 0x01, 0x02};

int main(int argc, char **argv){ 
    if (e32_config() == EXIT_FAILURE) return 0;
    e32_start();

    // implement protocol
    delay(5000);
    e32_send(CONTROL_PAYLOAD, sizeof(CONTROL_PAYLOAD));
    delay(5000);
    // wait for packets
    int pack_received = 0;
    uint8_t nb_byte = 0;
    do {
        nb_byte = e32_listen();
        delay(1000);
    } while( nb_byte <= 0);

    bool loop = true;
    while(loop) {
        if (nb_byte == 52) {
            uint8_t data_received[nb_byte];
            e32_fetch(nb_byte, data_received);
            bool flag = true;
            
            for (uint8_t j = 0; j < nb_byte; j ++) {
                if ( data_received[j] != ACK_PAYLOAD[j]) {
                    flag = false;
                }
            }
            if (flag == true) { // received ACK
                loop = false;   // exit loop
            }
            else {
                pack_received++;
                printf("pack_received: %d\n", pack_received);
            }
        }
        // Listen 
        nb_byte = 0;
        while (nb_byte == 0 && loop == true) {
            nb_byte = e32_listen();
            delay(100);
        }
    }

    // Calculate PRR
    printf("========Calculate PRR=============\n");
    double PRR = (double)pack_received / TOTAL_PACKET;
    printf("PRR: %lf\n", PRR);

    return 0;
}

