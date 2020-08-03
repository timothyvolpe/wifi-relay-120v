#include <Arduino.h>
#include "def.h"
#include "protocol.h"

char* decrypt( char *pBuffer, int len, bool no_length )
{
	int specifiedLen = 0;
	int current_key = CIPHER_KEY_START;
	char *pPlainBuffer;
	int offset = 0;

	// The first 4 bytes should be the length of the payload
	// Or if no_length, then the length provided to the method
	if( !no_length )
	{
		if( len <= 4 ) {
#ifdef DEBUG_SERIAL
			Serial.println( " > No length of packet specified" );
#endif
			return 0;
		}
		for( int i = 0; i < 4; i++ )
			specifiedLen |= (pBuffer[i] << i * 8);
		// Make sure the specified length is available
		if( len < specifiedLen + 4 ) {
#ifdef DEBUG_SERIAL
			Serial.println( " > Malformed packet" );
#endif
			return 0;
		}
	}
	else
		specifiedLen = len;

#ifdef DEBUG_SERIAL
	Serial.printf( " > Specified length: %d\n", specifiedLen );
#endif

	// Allocate
	pPlainBuffer = (char*)malloc( specifiedLen+1 );
	if( !pPlainBuffer )
		return 0;

	// Decrypt the contents
	if( !no_length )
		offset = 4;
	for( int i = 0; i < specifiedLen; i++ ) {
		pPlainBuffer[i] = current_key ^ pBuffer[i + offset];
		current_key = pBuffer[i + offset];
	}
	pPlainBuffer[specifiedLen] = '\0';

#ifdef DEBUG_SERIAL
	Serial.printf( " > %s\n", pPlainBuffer );
#endif

	return pPlainBuffer;
}

char* encrypt( char *pBuffer, int len )
{
	char *pEncryptedBuffer;
	int current_key = CIPHER_KEY_START;

	// Allocate
	pEncryptedBuffer = (char*)malloc( len + 4 );
	if( !pEncryptedBuffer )
		return 0;
	// Append length to the front
	for( int i = 0; i < 4; i++ )
		pEncryptedBuffer[i] = (len >> (8 * i)) & 0xFF;
	// Add encrypted bytes
	for( int i = 0; i < len; i++ ) {
		pEncryptedBuffer[i + 4] = current_key ^ pBuffer[i];
		current_key = pEncryptedBuffer[i + 4];
	}
	return pEncryptedBuffer;
}

void setup_protocol( protocol_data_t *pProtocol_data )
{
	pProtocol_data->broadcast_client.begin(FESTONE_PORT);
	pProtocol_data->tcp_listener.begin( FESTONE_PORT );
	pProtocol_data->last_remote_ip = "";
	pProtocol_data->last_remote_port = 0;

	pProtocol_data->json_document.clear();
	pProtocol_data->pJson_raw = 0;
}

/** Gets a json packet from the udp client if available and stores in json doc */
void get_udp_packet_json( protocol_data_t *pProtocol_data )
{
	int udpPacketSize;
	char* pBuffer;
	DeserializationError err;

	pProtocol_data->json_document.clear();

	// Check for UDP packets on port FESTONE_PORT
	udpPacketSize = pProtocol_data->broadcast_client.parsePacket();
	if( udpPacketSize > 0 )
	{
#ifdef DEBUG_SERIAL
		Serial.printf( "Received %d bytes from %s: at port %d (discovery)\n",
			udpPacketSize, pProtocol_data->broadcast_client.remoteIP().toString().c_str(),
			pProtocol_data->broadcast_client.remotePort(),
			pProtocol_data->broadcast_client.localPort()
		);
#endif
		pProtocol_data->last_remote_ip = pProtocol_data->broadcast_client.remoteIP().toString().c_str();
		pProtocol_data->last_remote_port = pProtocol_data->broadcast_client.remotePort();
		// Decode the discovery
		pBuffer = (char*)malloc( udpPacketSize );
		if( !pBuffer ) {
#ifdef DEBUG_SERIAL
			Serial.println( "malloc failure!" );
#endif
			fault();
			return;
		}
		pProtocol_data->broadcast_client.readBytes( pBuffer, udpPacketSize );
		pProtocol_data->pJson_raw = decrypt( pBuffer, udpPacketSize, false );
		free( pBuffer );
		pBuffer = 0;
		if( !pProtocol_data->pJson_raw )
			return;	// just ignore discovery packet if failed to decrypt
					// Deserialize JSON
		err = deserializeJson( pProtocol_data->json_document, pProtocol_data->pJson_raw );
		if( err != DeserializationError::Ok ) {
#ifdef DEBUG_SERIAL
			Serial.println( "Invalid JSON in packet" );
			pProtocol_data->json_document.clear();
#endif
		}
	}
}

void get_tcp_packet_json( protocol_data_t* pProtocolData, WiFiClient& tcp_client )
{
	char packetLengthBuffer[4];
	int bytesReceived;
	int packetLength;
	char* pBuffer;
	DeserializationError err;

	pProtocolData->json_document.clear();

	tcp_client = pProtocolData->tcp_listener.available();
	if( tcp_client )
	{
#ifdef DEBUG_SERIAL
		Serial.printf( "Opened TCP socket with %s\n", tcp_client.remoteIP().toString().c_str() );
		Serial.flush();
#endif
		tcp_client.setTimeout( WIFI_TIMEOUT );

		// Read the packet length
		bytesReceived = tcp_client.readBytes( packetLengthBuffer, 4 );
		if( bytesReceived < 4 ) {
#ifdef DEBUG_SERIAL
			Serial.println( "Timed out reading packet length" );
#endif
			return;
		}
		packetLength = 0;
		// Convert to integer
		for( int i = 0; i < 4; i++ )
			packetLength |= (packetLengthBuffer[i] << i * 8);
		if( packetLength <= 0 ) {
#ifdef DEBUG_SERIAL
			Serial.printf( "Invalid packet length (%d)\n", packetLengthBuffer );
#endif
			return;
		}
#ifdef DEBUG_SERIAL
		Serial.printf( "Reading %d bytes from client\n", packetLength );
#endif
		pBuffer = (char*)malloc( packetLength );
		if( !pBuffer ) {
#ifdef DEBUG_SERIAL
			Serial.println( "malloc failure!" );
#endif
			fault();
			return;
		}
		bytesReceived = tcp_client.readBytes( pBuffer, packetLength );
		if( bytesReceived < packetLength ) {
#ifdef DEBUG_SERIAL
			Serial.println( "Timeout reading packet payload" );
#endif
			free( pBuffer );
			return;
		}
		// Decrypt buffer
		pProtocolData->pJson_raw = decrypt( pBuffer, packetLength, true );
		free( pBuffer );
		pBuffer = 0;
		if( !pProtocolData->pJson_raw ) {
			return;
		}
		// Deserialize JSON
		err = deserializeJson( pProtocolData->json_document, pProtocolData->pJson_raw );
		if( err != DeserializationError::Ok ) {
#ifdef DEBUG_SERIAL
			Serial.println( "Invalid JSON in packet" );
			pProtocolData->json_document.clear();
			return;
#endif
		}
		// Save host info for response
		pProtocolData->last_remote_ip = tcp_client.remoteIP().toString().c_str();
		pProtocolData->last_remote_port = tcp_client.remotePort();
	}
}

void send_tcp_packet( WiFiClient& tcp_client, const char *pPacketBuffer, int len )
{
	char *pEncryptedBuffer;

	if( tcp_client )
	{
		pEncryptedBuffer = encrypt( (char*)pPacketBuffer, len );
		if( !pEncryptedBuffer ) {
#ifdef DEBUG_SERIAL
			Serial.println( "encrypt failure!" );
#endif
			fault();
			return;
		}

		// Send the bytes
		tcp_client.write( pEncryptedBuffer, len + 4 );

		free( pEncryptedBuffer );
		pEncryptedBuffer = 0;
	}
	else {
#ifdef DEBUG_SERIAL
		Serial.println( "Remote client refused response connection" );
#endif
	}
}

void command_discovery( protocol_data_t* pProtocolData )
{
#ifdef DEBUG_SERIAL
	Serial.println( "Received discovery packet, responding..." );
#endif
	const char *discoveryResponse = COMMAND_DISCOVERY_QUERY;
	char *pEncryptedBuffer = encrypt( (char*)discoveryResponse, strlen( discoveryResponse ) );
	if( !pEncryptedBuffer ) {
#ifdef DEBUG_SERIAL
		Serial.println( "encrypt failure!" );
#endif
		fault();
		return;
	}
	pProtocolData->broadcast_client.beginPacket( pProtocolData->last_remote_ip, pProtocolData->last_remote_port );
	pProtocolData->broadcast_client.write( pEncryptedBuffer, strlen( discoveryResponse ) + 4 );
	pProtocolData->broadcast_client.endPacket();

	free( pEncryptedBuffer );
	pEncryptedBuffer = 0;
}

void send_action_ok( WiFiClient tcp_client )
{
#ifdef DEBUG_SERIAL
	Serial.println( "Sending action ok..." );
#endif
	send_tcp_packet( tcp_client, COMMAND_ACTION_OK, strlen( COMMAND_ACTION_OK ) );
}

void protocol_loop_reset( protocol_data_t* pProtocolData )
{
	pProtocolData->last_remote_ip = "";
	pProtocolData->last_remote_port = 0;
	if( pProtocolData->pJson_raw ) {
		free( pProtocolData->pJson_raw );
		pProtocolData->pJson_raw = 0;
	}
}