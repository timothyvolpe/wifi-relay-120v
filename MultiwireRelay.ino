/*
 Name:		MultiwireRelay.ino
 Created:	6/16/2020 10:49:00 AM
 Author:	Timothy
*/

#pragma once
#include <WiFiManager.h>
#include <strings_en.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "def.h"
#include "relay.h"
#include "protocol.h"

bool faulted = false;

WiFiManager wifiManager;

protocol_data_t protocol_data;
relay_data highVoltageRelay;

unsigned long last_interrupt_time;
bool signal_set_btn;

void ICACHE_RAM_ATTR set_btn_ISR()
{
	unsigned long current_time = millis();
	if( current_time - last_interrupt_time >= SET_BTN_DEBOUNCE_MS ) {
		signal_set_btn = true;
		last_interrupt_time = current_time;
	}
}

void wifiConfigMode( WiFiManager *pWifiManager )
{
#ifdef DEBUG_SERIAL
	Serial.println( "Failed to restore WiFi, starting user portal" );
#endif
}

// the setup function runs once when you press reset or power the board
void setup()
{
	bool restoredWifi = false;

#ifdef DEBUG_SERIAL
	// Init debug serial output
	Serial.begin( 9600 );
	Serial.println( "" );
	Serial.println( "INIT" );
#endif

	// Setup pins
	pinMode( PIN_LED_POWER, OUTPUT );
	pinMode( PIN_LED_STATUS, OUTPUT );
	pinMode( PIN_RELAY, OUTPUT );
	pinMode( PIN_IN_SETBTN, INPUT_PULLUP );
	pinMode( PIN_IN_OPTO1, INPUT );
	pinMode( PIN_IN_OPTO2, INPUT );
	digitalWrite( PIN_LED_POWER, HIGH );
	digitalWrite( PIN_LED_STATUS, LOW );
	digitalWrite( PIN_RELAY, LOW );

	// Initialize the relay
	init_relay( &highVoltageRelay, PIN_RELAY, PIN_LED_STATUS );

	// Try to connect to the saved WiFi network (will fail if none is saved)
	//WiFi.hostname( WIFI_HOSTNAME );
	//WiFi.mode( WIFI_STA );
	//WiFi.begin();

#ifdef DEBUG_SERIAL
	Serial.printf( "MAC: %s\n", WiFi.macAddress().c_str() );
	Serial.print( "Attempting to restore lost WiFi connection" );
#endif

	// Blink power LED to show responsiveness
	digitalWrite( PIN_LED_POWER, LOW );
	delay( 500 );
	digitalWrite( PIN_LED_POWER, HIGH );
	wifiManager.setAPCallback( wifiConfigMode );
	if( !wifiManager.autoConnect() ) {
#ifdef DEBUG_SERIAL
		Serial.println( "Failed to configure WiFi" );
#endif
		fault();
	}


	// Wait WIFI_TIMEOUT seconds to connect
	for( int i = 0; i < WIFI_TIMEOUT; i++ )
	{
		if( WiFi.status() == WL_CONNECTED )
		{
#ifdef DEBUG_SERIAL
			Serial.println( "\nRestored lost WiFi connection" );
#endif
			restoredWifi = true;
			break;
		}
#ifdef DEBUG_SERIAL
		Serial.print( "." );
#endif
		digitalWrite( PIN_LED_POWER, LOW );
		delay( 500 );
		digitalWrite( PIN_LED_POWER, HIGH );
		delay( 500 );
	}
#ifdef DEBUG_SERIAL
	Serial.print( "\n" );
#endif
	// If we failed to restore wifi
	if( !restoredWifi )
	{
#ifdef DEBUG_SERIAL
		Serial.println( "Failed to restore WiFi connection" );
#endif
	}
	else
	{
#ifdef DEBUG_SERIAL
		Serial.printf( "WIFICONNECT: %s\n", WiFi.SSID().c_str() );
		setup_protocol( &protocol_data );
#endif
	}

	// Setup the interrupt allowing the relay to be set
	signal_set_btn = false;
	last_interrupt_time = 0;
	attachInterrupt( digitalPinToInterrupt( PIN_IN_SETBTN ), set_btn_ISR, FALLING );
}

/** Fatal fault, only manual reset can recover */
void fault()
{
#ifdef DEBUG_SERIAL
	Serial.println( "FAULT" );
#endif
	faulted = true;
	relay_reset( &highVoltageRelay );
	relay_flush( &highVoltageRelay );
}

/** 
	Checks to make sure the WiFi is still connected, and that the
	relay is in the expected state (via the optocouplers). If any checks fail,
	the relay is reset to the OFF state. 
*/
bool safety_check()
{
	bool checkFailed = false;

	// Check for wifi connection
	if( WiFi.status() != WL_CONNECTED )
	{
#ifdef DEBUG_SERIAL
		Serial.print( "WIFILOST" );
#endif
		checkFailed = true;
		ESP.reset();
	}

	// Compare optocouplers to expected value

	// If a check failed, reset the relay
	if( checkFailed ) {
		relay_reset( &highVoltageRelay );
		relay_flush( &highVoltageRelay );
	}

	return checkFailed;
}

// Checks if the set button was pressed via interrupted
void check_set_btn()
{
	if( signal_set_btn )
	{
#ifdef DEBUG_SERIAL
		Serial.println( "SETBTN" );
		relay_set( &highVoltageRelay );
#endif
		signal_set_btn = false;
	}
}

bool command_set_relay()
{
#ifdef DEBUG_SERIAL
	Serial.println( "Received set command from remote" );
#endif
	relay_set( &highVoltageRelay );

	return true;
}
bool command_reset_relay()
{
#ifdef DEBUG_SERIAL
	Serial.println( "Received reset command from remote" );
#endif
	relay_reset( &highVoltageRelay );

	return true;
}
bool command_toggle_relay()
{
#ifdef DEBUG_SERIAL
	Serial.println( "Received toggle command from remote" );
#endif
	relay_toggle( &highVoltageRelay );

	return true;
}

bool command_get_state( WiFiClient tcp_client )
{
	const char* pQuery;

	if( relay_state( &highVoltageRelay ) )
		pQuery = COMMAND_GET_STATE_SET;
	else
		pQuery = COMMAND_GET_STATE_RESET;

	send_tcp_packet( tcp_client, pQuery, strlen( pQuery ) );

	return false;
}

// the loop function runs over and over again until power down or reset
void loop()
{
	bool safetyCheckFailed;
	const char *pCommand;
	const char *pTCPCommand;
	bool actionOkResponse;
	WiFiClient tcp_client;

	if( faulted ) {
		digitalWrite( PIN_LED_POWER, LOW );
		digitalWrite( PIN_LED_STATUS, LOW );
		delay( 500 );
		digitalWrite( PIN_LED_POWER, HIGH );
		digitalWrite( PIN_LED_STATUS, HIGH );
		delay( 500 );
		return;
	}

	// Perform safety checks
	safetyCheckFailed = safety_check();
	// Check if the GPIO15 button was pressed via interrupted
	if( !safetyCheckFailed )
		check_set_btn();

	protocol_loop_reset( &protocol_data );
	get_udp_packet_json( &protocol_data);
	// Check if we got a udp command and process
	pCommand = protocol_data.json_document["command"];
	if( pCommand ) {
		if( strcmp( pCommand, "discovery" ) == 0 )
			command_discovery( &protocol_data );
	}

	// Check for tcp json packet
	protocol_loop_reset( &protocol_data );
	get_tcp_packet_json( &protocol_data, tcp_client );
	pTCPCommand = protocol_data.json_document["command"];
	actionOkResponse = false;
	if( pTCPCommand )
	{
		// Check device uid
		int deviceUID = protocol_data.json_document["device_uid"];
		if( deviceUID != DEVICE_UID ) {
#ifdef DEBUG_SERIAL
			Serial.println( "Command packet had invalid uid, ignoring" );
#endif
			return;
		}
		else
		{
#ifdef DEBUG_SERIAL
			Serial.printf( "Remote Command %s\n", pTCPCommand );
#endif
			if( strcmp( pTCPCommand, "set" ) == 0 ) {
				if( !safetyCheckFailed )
					actionOkResponse = command_set_relay();
			}
			else if( strcmp( pTCPCommand, "reset" ) == 0 )
				actionOkResponse = command_reset_relay();
			else if( strcmp( pTCPCommand, "toggle" ) == 0 ) {
				if( !safetyCheckFailed )
					actionOkResponse = command_toggle_relay();
			}
			else if( strcmp( pTCPCommand, "get_state" ) == 0 )
				actionOkResponse = command_get_state( tcp_client );
			else {
#ifdef DEBUG_SERIAL
				Serial.println( "Remote command was invalid" );
#endif
			}

			// Send action_ok if command was valid and did not respond with a custom packet
			if( actionOkResponse )
				send_action_ok( tcp_client );
		}
	}

	// Check for any unexpected changes
	if( safetyCheckFailed )
		relay_reset( &highVoltageRelay );
	// Flush non-critical changes to relay
	relay_flush( &highVoltageRelay );
}
