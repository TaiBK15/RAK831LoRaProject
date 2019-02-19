#if __STDC_VERSION__ >= 199901L
    #define _XOPEN_SOURCE 600
#else
    #define _XOPEN_SOURCE 500
#endif

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf sprintf fopen fputs */

#include <string.h>     /* memset */
#include <signal.h>     /* sigaction */
#include <time.h>       /* time clock_gettime strftime gmtime clock_nanosleep*/
#include <unistd.h>     /* getopt access */
#include <stdlib.h>     /* atoi */

#include "parson.h"
#include "loragw_hal.h"
#include "rak831_api.h"
#include <wiringPi.h>
#define GPIO_RESET_PIN 6
/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MSG(args...)    fprintf(stderr,"Configure Concentrator:: " args) /* message that is destined to the user */
#define TX_RF_CHAIN                 0 /* TX only supported on radio A */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES (GLOBAL) ------------------------------------------- */

/* signal handling variables */
struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */
static int exit_sig = 0; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */
static int quit_sig = 0; /* 1 -> application terminates without shutting down the hardware */

/* configuration variables needed by the application  */
uint64_t lgwm = 0; /* LoRa gateway MAC address */
char lgwm_str[17];

/* clock and log file management */
time_t now_time;
time_t log_start_time;
FILE * log_file = NULL;
char log_file_name[64];
/* Gateway specificities */
static int8_t antenna_gain = 0;

/* TX capabilities */
static struct lgw_tx_gain_lut_s txlut; /* TX gain table */
static uint32_t tx_freq_min[LGW_RF_CHAIN_NB]; /* lowest frequency supported by TX chain */
static uint32_t tx_freq_max[LGW_RF_CHAIN_NB]; /* highest frequency supported by TX chain */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

static void sig_handler(int sigio);
int * config_signal();

int parse_SX1301_configuration(const char * conf_file);
int parse_gateway_configuration(const char * conf_file);
void open_log(void);

void rak831_reset();
int rak831_config();
int rak831_start();
int rak831_stop();
struct lgw_pkt_rx_s rak831_fetch_packets(int pkt_order);

int rak831_listen();
void rak831_send(bool is_json, char *payload_to_node, uint32_t freq_hz, uint8_t bandwidth, uint32_t datarate, uint8_t coderate);

size_t findPayloadSize(char* payload);

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static void sig_handler(int sigio) {
	if (sigio == SIGQUIT) {
		quit_sig = 1;;
	} else if ((sigio == SIGINT) || (sigio == SIGTERM)) {
		exit_sig = 1;
	}
}

int parse_SX1301_configuration(const char * conf_file) {
	int i;
	const char conf_obj[] = "SX1301_conf";
    char param_name[32]; /* used to generate variable parameter names */
    const char *str; /* used to store string value from JSON object */
	struct lgw_conf_board_s boardconf;
	struct lgw_conf_rxrf_s rfconf;
	struct lgw_conf_rxif_s ifconf;
	JSON_Value *root_val;
	JSON_Object *root = NULL;
	JSON_Object *conf = NULL;
	JSON_Value *val;
	uint32_t sf, bw;

    /* try to parse JSON */
	root_val = json_parse_file_with_comments(conf_file);
	root = json_value_get_object(root_val);
	if (root == NULL) {
		MSG("ERROR: %s id not a valid JSON file\n", conf_file);
		exit(EXIT_FAILURE);
	}
	conf = json_object_get_object(root, conf_obj);
	if (conf == NULL) {
		MSG("INFO: %s does not contain a JSON object named %s\n", conf_file, conf_obj);
		return -1;
	} else {
		MSG("INFO: %s does contain a JSON object named %s, parsing SX1301 parameters\n", conf_file, conf_obj);
	}

    /* set board configuration */
    memset(&boardconf, 0, sizeof boardconf); /* initialize configuration structure */
    val = json_object_get_value(conf, "lorawan_public"); /* fetch value (if possible) */
	if (json_value_get_type(val) == JSONBoolean) {
		boardconf.lorawan_public = (bool)json_value_get_boolean(val);
	} else {
		MSG("WARNING: Data type for lorawan_public seems wrong, please check\n");
		boardconf.lorawan_public = false;
	}
    val = json_object_get_value(conf, "clksrc"); /* fetch value (if possible) */
	if (json_value_get_type(val) == JSONNumber) {
		boardconf.clksrc = (uint8_t)json_value_get_number(val);
	} else {
		MSG("WARNING: Data type for clksrc seems wrong, please check\n");
		boardconf.clksrc = 0;
	}
	MSG("INFO: lorawan_public %d, clksrc %d\n", boardconf.lorawan_public, boardconf.clksrc);
    /* all parameters parsed, submitting configuration to the HAL */
	if (lgw_board_setconf(boardconf) != LGW_HAL_SUCCESS) {
		MSG("ERROR: Failed to configure board\n");
		return -1;
	}
     /* set antenna gain configuration */
    val = json_object_get_value(conf, "antenna_gain"); /* fetch value (if possible) */
	if (val != NULL) {
		if (json_value_get_type(val) == JSONNumber) {
			antenna_gain = (int8_t)json_value_get_number(val);
		} else {
			MSG("WARNING: Data type for antenna_gain seems wrong, please check\n");
			antenna_gain = 0;
		}
	}
	MSG("INFO: antenna_gain %d dBi\n", antenna_gain);
    /* set configuration for tx gains */
    memset(&txlut, 0, sizeof txlut); /* initialize configuration structure */
	for (i = 0; i < TX_GAIN_LUT_SIZE_MAX; i++) {
        snprintf(param_name, sizeof param_name, "tx_lut_%i", i); /* compose parameter path inside JSON structure */
        val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
		if (json_value_get_type(val) != JSONObject) {
			MSG("INFO: no configuration for tx gain lut %i\n", i);
			continue;
		}
        txlut.size++; /* update TX LUT size based on JSON object found in configuration file */
        /* there is an object to configure that TX gain index, let's parse it */
		snprintf(param_name, sizeof param_name, "tx_lut_%i.pa_gain", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONNumber) {
			txlut.lut[i].pa_gain = (uint8_t)json_value_get_number(val);
		} else {
			MSG("WARNING: Data type for %s[%d] seems wrong, please check\n", param_name, i);
			txlut.lut[i].pa_gain = 0;
		}
		snprintf(param_name, sizeof param_name, "tx_lut_%i.dac_gain", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONNumber) {
			txlut.lut[i].dac_gain = (uint8_t)json_value_get_number(val);
		} else {
            txlut.lut[i].dac_gain = 3; /* This is the only dac_gain supported for now */
		}
		snprintf(param_name, sizeof param_name, "tx_lut_%i.dig_gain", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONNumber) {
			txlut.lut[i].dig_gain = (uint8_t)json_value_get_number(val);
		} else {
			MSG("WARNING: Data type for %s[%d] seems wrong, please check\n", param_name, i);
			txlut.lut[i].dig_gain = 0;
		}
		snprintf(param_name, sizeof param_name, "tx_lut_%i.mix_gain", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONNumber) {
			txlut.lut[i].mix_gain = (uint8_t)json_value_get_number(val);
		} else {
			MSG("WARNING: Data type for %s[%d] seems wrong, please check\n", param_name, i);
			txlut.lut[i].mix_gain = 0;
		}
		snprintf(param_name, sizeof param_name, "tx_lut_%i.rf_power", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONNumber) {
			txlut.lut[i].rf_power = (int8_t)json_value_get_number(val);
		} else {
			MSG("WARNING: Data type for %s[%d] seems wrong, please check\n", param_name, i);
			txlut.lut[i].rf_power = 0;
		}
	}
    /* all parameters parsed, submitting configuration to the HAL */
	if (txlut.size > 0) {
		MSG("INFO: Configuring TX LUT with %u indexes\n", txlut.size);
		if (lgw_txgain_setconf(&txlut) != LGW_HAL_SUCCESS) {
			MSG("ERROR: Failed to configure concentrator TX Gain LUT\n");
			return -1;
		}
	} else {
		MSG("WARNING: No TX gain LUT defined\n");
	}

    /* set configuration for RF chains */
	for (i = 0; i < LGW_RF_CHAIN_NB; ++i) {
        memset(&rfconf, 0, sizeof(rfconf)); /* initialize configuration structure */
        sprintf(param_name, "radio_%i", i); /* compose parameter path inside JSON structure */
        val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
		if (json_value_get_type(val) != JSONObject) {
			MSG("INFO: no configuration for radio %i\n", i);
			continue;
		}
        /* there is an object to configure that radio, let's parse it */
		sprintf(param_name, "radio_%i.enable", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONBoolean) {
			rfconf.enable = (bool)json_value_get_boolean(val);
		} else {
			rfconf.enable = false;
		}
        if (rfconf.enable == false) { /* radio disabled, nothing else to parse */
		MSG("INFO: radio %i disabled\n", i);
        } else  { /* radio enabled, will parse the other parameters */
		snprintf(param_name, sizeof param_name, "radio_%i.freq", i);
		rfconf.freq_hz = (uint32_t)json_object_dotget_number(conf, param_name);
		snprintf(param_name, sizeof param_name, "radio_%i.rssi_offset", i);
		rfconf.rssi_offset = (float)json_object_dotget_number(conf, param_name);
		snprintf(param_name, sizeof param_name, "radio_%i.type", i);
		str = json_object_dotget_string(conf, param_name);
		if (!strncmp(str, "SX1255", 6)) {
			rfconf.type = LGW_RADIO_TYPE_SX1255;
		} else if (!strncmp(str, "SX1257", 6)) {
			rfconf.type = LGW_RADIO_TYPE_SX1257;
		} else {
			MSG("WARNING: invalid radio type: %s (should be SX1255 or SX1257)\n", str);
		}
		snprintf(param_name, sizeof param_name, "radio_%i.tx_enable", i);
		val = json_object_dotget_value(conf, param_name);
		if (json_value_get_type(val) == JSONBoolean) {
			rfconf.tx_enable = (bool)json_value_get_boolean(val);
			if (rfconf.tx_enable == true) {
                    /* tx notch filter frequency to be set */
				snprintf(param_name, sizeof param_name, "radio_%i.tx_notch_freq", i);
				rfconf.tx_notch_freq = (uint32_t)json_object_dotget_number(conf, param_name);
			}
		} else {
			rfconf.tx_enable = false;
		}
		MSG("INFO: radio %i enabled (type %s), center frequency %u, RSSI offset %f, tx enabled %d, tx_notch_freq %u\n", i, str, rfconf.freq_hz, rfconf.rssi_offset, rfconf.tx_enable, rfconf.tx_notch_freq);
	}
        /* all parameters parsed, submitting configuration to the HAL */
	if (lgw_rxrf_setconf(i, rfconf) != LGW_HAL_SUCCESS) {
		MSG("ERROR: invalid configuration for radio %i\n", i);
		return -1;
	}
}

    /* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
for (i = 0; i < LGW_MULTI_NB; ++i) {
        memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
        sprintf(param_name, "chan_multiSF_%i", i); /* compose parameter path inside JSON structure */
        val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
	if (json_value_get_type(val) != JSONObject) {
		MSG("INFO: no configuration for LoRa multi-SF channel %i\n", i);
		continue;
	}
        /* there is an object to configure that LoRa multi-SF channel, let's parse it */
	sprintf(param_name, "chan_multiSF_%i.enable", i);
	val = json_object_dotget_value(conf, param_name);
	if (json_value_get_type(val) == JSONBoolean) {
		ifconf.enable = (bool)json_value_get_boolean(val);
	} else {
		ifconf.enable = false;
	}
        if (ifconf.enable == false) { /* LoRa multi-SF channel disabled, nothing else to parse */
	MSG("INFO: LoRa multi-SF channel %i disabled\n", i);
        } else  { /* LoRa multi-SF channel enabled, will parse the other parameters */
	sprintf(param_name, "chan_multiSF_%i.radio", i);
	ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, param_name);
	sprintf(param_name, "chan_multiSF_%i.if", i);
	ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, param_name);
            // TODO: handle individual SF enabling and disabling (spread_factor)
	MSG("INFO: LoRa multi-SF channel %i enabled, radio %i selected, IF %i Hz, 125 kHz bandwidth, SF 7 to 12\n", i, ifconf.rf_chain, ifconf.freq_hz);
}
        /* all parameters parsed, submitting configuration to the HAL */
if (lgw_rxif_setconf(i, ifconf) != LGW_HAL_SUCCESS) {
	MSG("ERROR: invalid configuration for Lora multi-SF channel %i\n", i);
	return -1;
}
}

    /* set configuration for LoRa standard channel */
    memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
    val = json_object_get_value(conf, "chan_Lora_std"); /* fetch value (if possible) */
if (json_value_get_type(val) != JSONObject) {
	MSG("INFO: no configuration for LoRa standard channel\n");
} else {
	val = json_object_dotget_value(conf, "chan_Lora_std.enable");
	if (json_value_get_type(val) == JSONBoolean) {
		ifconf.enable = (bool)json_value_get_boolean(val);
	} else {
		ifconf.enable = false;
	}
	if (ifconf.enable == false) {
		MSG("INFO: LoRa standard channel %i disabled\n", i);
	} else  {
		ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.radio");
		ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_Lora_std.if");
		bw = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.bandwidth");
		switch(bw) {
			case 500000: ifconf.bandwidth = BW_500KHZ; break;
			case 250000: ifconf.bandwidth = BW_250KHZ; break;
			case 125000: ifconf.bandwidth = BW_125KHZ; break;
			default: ifconf.bandwidth = BW_UNDEFINED;
		}
		sf = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.spread_factor");
		switch(sf) {
			case  7: ifconf.datarate = DR_LORA_SF7;  break;
			case  8: ifconf.datarate = DR_LORA_SF8;  break;
			case  9: ifconf.datarate = DR_LORA_SF9;  break;
			case 10: ifconf.datarate = DR_LORA_SF10; break;
			case 11: ifconf.datarate = DR_LORA_SF11; break;
			case 12: ifconf.datarate = DR_LORA_SF12; break;
			default: ifconf.datarate = DR_UNDEFINED;
		}
		MSG("INFO: LoRa standard channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, SF %u\n", ifconf.rf_chain, ifconf.freq_hz, bw, sf);
	}
	if (lgw_rxif_setconf(8, ifconf) != LGW_HAL_SUCCESS) {
		MSG("ERROR: invalid configuration for Lora standard channel\n");
		return -1;
	}
}

    /* set configuration for FSK channel */
    memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
    val = json_object_get_value(conf, "chan_FSK"); /* fetch value (if possible) */
if (json_value_get_type(val) != JSONObject) {
	MSG("INFO: no configuration for FSK channel\n");
} else {
	val = json_object_dotget_value(conf, "chan_FSK.enable");
	if (json_value_get_type(val) == JSONBoolean) {
		ifconf.enable = (bool)json_value_get_boolean(val);
	} else {
		ifconf.enable = false;
	}
	if (ifconf.enable == false) {
		MSG("INFO: FSK channel %i disabled\n", i);
	} else  {
		ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_FSK.radio");
		ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_FSK.if");
		bw = (uint32_t)json_object_dotget_number(conf, "chan_FSK.bandwidth");
		if      (bw <= 7800)   ifconf.bandwidth = BW_7K8HZ;
		else if (bw <= 15600)  ifconf.bandwidth = BW_15K6HZ;
		else if (bw <= 31200)  ifconf.bandwidth = BW_31K2HZ;
		else if (bw <= 62500)  ifconf.bandwidth = BW_62K5HZ;
		else if (bw <= 125000) ifconf.bandwidth = BW_125KHZ;
		else if (bw <= 250000) ifconf.bandwidth = BW_250KHZ;
		else if (bw <= 500000) ifconf.bandwidth = BW_500KHZ;
		else ifconf.bandwidth = BW_UNDEFINED;
		ifconf.datarate = (uint32_t)json_object_dotget_number(conf, "chan_FSK.datarate");
		MSG("INFO: FSK channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, %u bps datarate\n", ifconf.rf_chain, ifconf.freq_hz, bw, ifconf.datarate);
	}
	if (lgw_rxif_setconf(9, ifconf) != LGW_HAL_SUCCESS) {
		MSG("ERROR: invalid configuration for FSK channel\n");
		return -1;
	}
}
json_value_free(root_val);
return 0;
}

int parse_gateway_configuration(const char * conf_file) {
	const char conf_obj[] = "gateway_conf";
	JSON_Value *root_val;
	JSON_Object *root = NULL;
	JSON_Object *conf = NULL;
    const char *str; /* pointer to sub-strings in the JSON data */
	unsigned long long ull = 0;

    /* try to parse JSON */
	root_val = json_parse_file_with_comments(conf_file);
	root = json_value_get_object(root_val);
	if (root == NULL) {
		MSG("ERROR: %s id not a valid JSON file\n", conf_file);
		exit(EXIT_FAILURE);
	}
	conf = json_object_get_object(root, conf_obj);
	if (conf == NULL) {
		MSG("INFO: %s does not contain a JSON object named %s\n", conf_file, conf_obj);
		return -1;
	} else {
		MSG("INFO: %s does contain a JSON object named %s, parsing gateway parameters\n", conf_file, conf_obj);
	}

    /* getting network parameters (only those necessary for the packet logger) */
	str = json_object_get_string(conf, "gateway_ID");
	if (str != NULL) {
		sscanf(str, "%llx", &ull);
		lgwm = ull;
		MSG("INFO: gateway MAC address is configured to %016llX\n", ull);
	}

	json_value_free(root_val);
	return 0;
}

void open_log(void) {
	int i;
	char iso_date[20];

	sprintf(log_file_name, "packets_from_node.json");
    log_file = fopen(log_file_name, "a"); /* create log file, append if file already exist */
	if (log_file == NULL) {
		MSG("ERROR: impossible to create log file %s\n", log_file_name);
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */
    int i, j; /* loop and temporary variables */
    struct timespec sleep_time = {0, 3000000}; /* 3 ms */

    /* clock and log rotation management */
    int log_rotate_interval = 3600; /* by default, rotation every hour */
    int time_check = 0; /* variable used to limit the number of calls to time() function */
    unsigned long pkt_in_log = 0; /* count the number of packet written in each log file */

    /* configuration file related */
    const char global_conf_fname[] = "global_conf.json"; /* contain global (typ. network-wide) configuration */
    const char local_conf_fname[] = "local_conf.json"; /* contain node specific configuration, overwrite global parameters for parameters that are defined in both */
    const char debug_conf_fname[] = "debug_conf.json"; /* if present, all other configuration files are ignored */

    /* allocate memory for packet fetching and processing */
    struct lgw_pkt_rx_s rxpkt[16]; /* array containing up to 16 inbound packets metadata */
    struct lgw_pkt_rx_s *p; /* pointer on a RX packet */

    /* local timestamp variables until we get accurate GPS time */
struct timespec fetch_time;
char fetch_timestamp[30];
struct tm * x;

/* -----------------CONFIG_----------------------- */    
int rak831_config()
{
    /* configuration files management */
	if (access(debug_conf_fname, R_OK) == 0) {
    /* if there is a debug conf, parse only the debug conf */
		MSG("INFO: found debug configuration file %s, other configuration files will be ignored\n", debug_conf_fname);
		parse_SX1301_configuration(debug_conf_fname);
		parse_gateway_configuration(debug_conf_fname);
	} else if (access(global_conf_fname, R_OK) == 0) {
    /* if there is a global conf, parse it and then try to parse local conf  */
		MSG("INFO: found global configuration file %s, trying to parse it\n", global_conf_fname);
		parse_SX1301_configuration(global_conf_fname);
		parse_gateway_configuration(global_conf_fname);
		if (access(local_conf_fname, R_OK) == 0) {
			MSG("INFO: found local configuration file %s, trying to parse it\n", local_conf_fname);
			parse_SX1301_configuration(local_conf_fname);
			parse_gateway_configuration(local_conf_fname);
		}
	} else if (access(local_conf_fname, R_OK) == 0) {
    /* if there is only a local conf, parse it and that's all */
		MSG("INFO: found local configuration file %s, trying to parse it\n", local_conf_fname);
		parse_SX1301_configuration(local_conf_fname);
		parse_gateway_configuration(local_conf_fname);
	} else {
		MSG("ERROR: failed to find any configuration file named %s, %s or %s\n", global_conf_fname, local_conf_fname, debug_conf_fname);
		return EXIT_FAILURE;
	}
    /* transform the MAC address into a string */
	sprintf(lgwm_str, "%08X%08X", (uint32_t)(lgwm >> 32), (uint32_t)(lgwm & 0xFFFFFFFF));
	return EXIT_SUCCESS;
}

/* -----------------CONFIG_SIGNAL------------------ */
int * config_signal(){
    /* configure signal handling */
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = sig_handler;
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	int *signal_ = (int *) malloc(4 * sizeof(int));
	signal_[0] = quit_sig;
	signal_[1] = exit_sig;
	return signal_;
}

/* -----------------rak831_start------------------ */
int rak831_start(){
	int i;
	sleep(2);
	i = lgw_start();
	if (i == LGW_HAL_SUCCESS) {
		MSG("INFO: concentrator started, packet can now be received\n");
		return EXIT_SUCCESS;
	} else {
		MSG("ERROR: failed to start the concentrator\n");
		return EXIT_FAILURE;
	}
}

/* -----------------rak831_stop------------------ */
int rak831_stop(){
	int i;
    /* clean up before leaving */
	i = lgw_stop();
	if (i == LGW_HAL_SUCCESS) {
		MSG("INFO: concentrator stopped successfully\n");
	} else {
		MSG("WARNING: failed to stop concentrator successfully\n");
	}
	return EXIT_SUCCESS;
}

/* -----------------IS_PACKETS_RECEIVED------------------ */
int rak831_listen(){
	int nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);
	if (nb_pkt == LGW_HAL_ERROR) {
		MSG("ERROR: failed packet fetch, exiting\n");
		return 0;
	} 
	else if (nb_pkt == 0) {
        clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_time, NULL); /* wait a short time if no packets */
		return 0;
	} 
	else {
		printf("RECEIVED %d PACKETS, FETCHING...!\n", nb_pkt);
		return nb_pkt;        
	}
}
/* -----------------rak831_fetch_packets------------------ */
struct lgw_pkt_rx_s rak831_fetch_packets(int pkt_order){
	struct lgw_pkt_rx_s pktdata;
    memset(&pktdata, 0, sizeof(pktdata));
	pktdata = rxpkt[pkt_order];
	return pktdata;
}

/* -----------------rak831_send------------------ */
void rak831_send(bool is_json, char *payload_to_node, uint32_t freq_hz, uint8_t bandwidth, uint32_t datarate, uint8_t coderate){
	printf("Send packets to node\n");
	 /* config payload parameter*/
	JSON_Array* payload;
	uint8_t status_var;
	uint32_t sx1301_count_us;
    struct lgw_pkt_tx_s txpkt; /* array containing 1 outbound packet + metadata */
	memset(&txpkt, 0, sizeof(txpkt));

	if (is_json) {
	JSON_Value* parsed_payload_value = json_parse_string_with_comments(payload_to_node);
	if (parsed_payload_value == NULL){
		printf("nothing to parse/ to send to node");
		return ;
	}
	JSON_Object* parsed_payload_object = json_value_get_object(parsed_payload_value);
	payload = json_object_get_string(parsed_payload_object, "payload");
	size_t payload_size = findPayloadSize(payload);

	txpkt.freq_hz = (int)json_object_get_number(parsed_payload_object, "frequency");	// get frequency from payload
	txpkt.tx_mode = IMMEDIATE;
	txpkt.count_us = sx1301_count_us;
	txpkt.rf_chain = TX_RF_CHAIN;
	txpkt.rf_power = 20;
	txpkt.modulation = MOD_LORA;
	txpkt.bandwidth = BW_125KHZ;
	txpkt.datarate = DR_LORA_SF7;
	txpkt.coderate = CR_LORA_4_5;
	txpkt.invert_pol = false;
	txpkt.preamble = 8;
	txpkt.no_crc = false;
	txpkt.no_header = false;
	txpkt.size = (uint16_t)payload_size;
	// get payload size
	}
	else {
	payload = payload_to_node;
	size_t payload_size = findPayloadSize(payload);

	txpkt.freq_hz = freq_hz;
	txpkt.tx_mode = IMMEDIATE;
	txpkt.count_us = sx1301_count_us;
	txpkt.rf_chain = TX_RF_CHAIN;
	txpkt.rf_power = 20;
	txpkt.modulation = MOD_LORA;
	txpkt.bandwidth = bandwidth;	//bandwidth
	txpkt.datarate = datarate;		//datarate
	txpkt.coderate = coderate;		//coderate
	txpkt.invert_pol = false;
	txpkt.preamble = 8;
	txpkt.no_crc = false;
	txpkt.no_header = false;
	txpkt.size = (uint16_t)payload_size;
	}

    strcpy((char *)txpkt.payload, payload ); /* abc.. is for padding */
    i = lgw_send(txpkt); /* non-blocking scheduling of TX packet */
	if (i == LGW_HAL_ERROR) {
		printf("ERROR\n");
		return EXIT_FAILURE;
	}
	else {
            /* wait for packet to finish sending */
		do {
			wait_ms(5);
                lgw_status(TX_STATUS, &status_var); /* get TX status */
		} while (status_var != TX_FREE);
		printf("OK\n");
	}
}

size_t findPayloadSize(char* payload){
	size_t payload_size = 0;
	for (int i = 0; i < 1024; ++i) {
		if (* (payload + i) == '\0') return payload_size;
		else payload_size ++;
	}
	return 1024;
}

void rak831_reset() {
	printf("reseting RAK831...\n");
	wiringPiSetup();
	pinMode(GPIO_RESET_PIN, OUTPUT);
	digitalWrite(GPIO_RESET_PIN, HIGH);
	sleep(5);
	digitalWrite(GPIO_RESET_PIN, LOW);
	printf("reset done!\n");
}
/* --- EOF ------------------------------------------------------------------ */
