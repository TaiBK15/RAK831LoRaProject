#ifndef _RAK831_API_H
#define _RAK831_API_H
int nb_pkt;

/**
*/
static void sig_handler(int sigio);

/**
Parse configuration from "global_conf.json" & "local_conf.json"
Setup parameters for TX/RX
*/
int parse_SX1301_configuration(const char * conf_file);

/**
Parse configuration from "global_conf.json" & "local_conf.json"
Setup parameters for TX/RX
*/
int parse_gateway_configuration(const char * conf_file);

/**
To open file
*/
void open_log(void);

/**
Config Concentrator
*/
int rak831_config();

/**
*/
int * config_signal();

/**
Start Concentrator
*/
int rak831_start();

/**
Stop Concentrator
*/
int rak831_stop();

/**
Fetch Packets received from Node
@return packet if CRC_OK, else NULL  
*/
struct lgw_pkt_rx_s rak831_fetch_packets(int pkt_order);

/**
Check if Packets from Node
@return EXIT_SUCCESS if have packets, else EXIT_FAILURE
*/
int rak831_listen();

void rak831_send(bool is_json, char *payload_to_node, uint32_t freq_hz, uint8_t bandwidth, uint32_t datarate, uint8_t coderate);

/**
Find Payload size
@param Pointer of payload array
@return Size of payload array
*/
size_t findPayloadSize(char* payload);

void rak831_reset();

#endif