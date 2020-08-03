#include <Arduino.h>
#include "def.h"
#include "relay.h"

void init_relay( relay_data* pRelay, int relay_pin, int relay_led )
{
	pRelay->relay_pin = relay_pin;
	pRelay->relay_led = relay_led;
	pRelay->relay_fault = false;

	relay_reset( pRelay );
	relay_flush( pRelay );
}

void relay_set( relay_data* pRelay )
{
	if( !pRelay->relay_fault )
	{
		pRelay->relay_state = true;
#ifdef DEBUG_SERIAL
		Serial.println( "RELAYSET" );
#endif
	}
	else
		relay_reset( pRelay );
}
void relay_reset( relay_data* pRelay )
{
	pRelay->relay_state = false;
#ifdef DEBUG_SERIAL
	Serial.println( "RELAYRESET" );
#endif
}
void relay_toggle( relay_data* pRelay )
{
	if( !pRelay->relay_state )
		relay_set( pRelay );
	else
		relay_reset( pRelay );
}

void relay_flush( relay_data* pRelay )
{
	if( pRelay->relay_state ) {
		digitalWrite( pRelay->relay_pin, HIGH );
		if( pRelay->relay_led >= 0 )
			digitalWrite( pRelay->relay_led, HIGH );
	}
	else {
		digitalWrite( pRelay->relay_pin, LOW );
		if( pRelay->relay_led >= 0 )
			digitalWrite( pRelay->relay_led, LOW );
	}
}

bool relay_state( relay_data* pRelay )
{
	return pRelay->relay_state;
}