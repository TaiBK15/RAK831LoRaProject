#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf sprintf fopen fputs */

#include <string.h>     /* memset */
#include <signal.h>     /* sigaction */
#include <time.h>       /* time clock_gettime strftime gmtime clock_nanosleep*/
#include <unistd.h>     /* getopt access */
#include <stdlib.h>     /* atoi */

#include "rak831_api.h"
#include "e32_api.h"
#include "loragw_hal.h"
#include <pthread.h>	/* thread */
#include <sys/socket.h> /* socket */
#include <netinet/in.h> 


/*---------------------GLOBAL VARIABLES---------------------------------*/
// thread
volatile bool is_packets_from_server = false;
volatile bool is_packets_from_node = false;
#define BUFFER 1024
// socket
bool configSocketAndConnectServer();
#define PORT 8080
int sock = 0, valread; 
// payload
volatile char* payload_to_server;
volatile char* payload_to_node;


/*---------------------THREAD-------------------------------------------*/

/* thread for Checking if packet(s) is(are) received from Server */
void *threadCheckPacketsFromServer(void *vargp) { 
	sleep(1); 
	printf("PACKETS FROM SERVER:: this is thread for waiting packets from server \n");
	char packets_from_server[BUFFER];
	while(true){
		valread = read(sock , packets_from_server, 1024);
		payload_to_node = packets_from_server;
		is_packets_from_server = true;
	}
	return NULL; 
} 

/* thread for Checking if packet(s) is(are) received from Node */
void *threadCheckPacketsFromNode(void *vargp) { 
	sleep(1); 
	printf("PACKETS FROM NODE:: this is thread for waiting packets from node \n"); 
	while(true){
		if(rak831_listen() == EXIT_SUCCESS){
			payload_to_server = rak831_fetch_packets();
			if (payload_to_server != NULL)
				is_packets_from_node = true;
		} 
	}
	return NULL; 
}

/* thread for Sending packet(s) received from Node to Server */
void *threadSendPacketsToServer(void *vargp) { 
	sleep(1); 
	printf("PACKETS TO SERVER:: this is thread for sending packets to server \n"); 
	while(true){
		if (is_packets_from_node){
			is_packets_from_node = false;
			sendPacketsToServer(payload_to_server);
		}
	}
	return NULL; 
}

/* thread for Sending packet(s) received from Server to Node */ 
void *threadSendPacketsToNode(void *vargp) { 
	sleep(1); 
	printf("PACKETS TO NODE:: this is thread for sending packets to node \n"); 
	while(true){
		if (is_packets_from_server){
			is_packets_from_server = false;
			printf("PACKETS FROM SERVER:: %s", payload_to_node);
			rak831_send(payload_to_node);
	    	// TODO: should have mutex to disable receive to send
		}	
	}
	return NULL; 
} 


/* ------------------------------MAIN--------------------------------------- */
int main(){
	/* Config concentrator */
	configAndStartConcentrator();
	/* Config client socket */
	if (!configSocketAndConnectServer()) exit(EXIT_FAILURE);
	/* config thread */
	pthread_t thread_check_packets_from_server; 
	pthread_t thread_check_packets_from_node; 
	pthread_t thread_send_packets_to_server; 
	pthread_t thread_send_packets_to_node; 
	configThread(thread_check_packets_from_server, threadCheckPacketsFromServer);
	configThread(thread_check_packets_from_node, threadCheckPacketsFromNode);
	configThread(thread_send_packets_to_server, threadSendPacketsToServer);
	configThread(thread_send_packets_to_node, threadSendPacketsToNode);

	/* loop */
	while(true);

    rak831_stop();

	return 0;
}

/* ------------------------FUNCTIONS------------------------ */

/** configAndStartConcentrator()
Config Concentrator using API: rak831_config() in "rak831_api.h"
Start Concentrator using API: rak831_start() in "rak831_api.h"
*/
void configAndStartConcentrator() {
	// configure the radios and IF+modems
	rak831_config();
    // start the LoRa concentrator
	while(rak831_start() != EXIT_SUCCESS) 
		sleep(1);
}

/** configThread()
To setup Thread.
*/
void configThread(pthread_t thread_id, void* myThreadFun){
	pthread_create(&thread_id, NULL, myThreadFun, NULL); 
    // pthread_join(thread_id, NULL); 
}

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

