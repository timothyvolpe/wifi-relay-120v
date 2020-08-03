#pragma once
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "def.h"

#define BROADCAST_ADDRESS	"255.255.255.255"
#define FESTONE_PORT		6323

#define COMM_PORT			6323

#define CIPHER_KEY_START	111

typedef struct tag_protocol_data_t
{
	tag_protocol_data_t() : tcp_listener( FESTONE_PORT ) {}

	WiFiUDP								broadcast_client;
	WiFiServer							tcp_listener;
	const char*							last_remote_ip;
	uint16_t							last_remote_port;

	StaticJsonDocument<JSON_DOC_SIZE>	json_document;
	char*								pJson_raw;
} protocol_data_t;

/** Decrypt the received packet. Caller is responsible for deleting returned memory. */
char* decrypt( char *pBuffer, int len, bool no_length );
/** Encrypt the packet to send. Caller is responsible for deleting the returned memory.*/
char* encrypt( char *pBuffer, int len );

/** Sets up protocol globals, call from setup() */
void setup_protocol( protocol_data_t *pProtocol_data );

/** gets a UDP packet and stores it in json */
void get_udp_packet_json( protocol_data_t *pProtocol_data );
/** Gets a TCP packet and stores it in json */
void get_tcp_packet_json( protocol_data_t* pProtocolData, WiFiClient& tcp_client );
/** Sends a packet over a TCP connection */
void send_tcp_packet( WiFiClient& tcp_client, const char *pPacketBuffer, int len );

/** Discovery command */
void command_discovery( protocol_data_t* pProtocolData );
/** Send action ok */
void send_action_ok( WiFiClient tcp_client );

/** Resets the per-loop data */
void protocol_loop_reset( protocol_data_t* pProtocolData );