#pragma once

typedef struct
{
	int		relay_pin;		// The pin that controls the relay, high is closed, low is open
	int		relay_led;		// The pin that controls the relay LED, -1 does will ignore
	bool	relay_state;	// True is closed, false is open
	bool	relay_fault;	// True if faulted, relay cannot be set. False if OK
} relay_data;

/** 
	Initializes a relay on pin relay_pin, with status LED relay_led (-1 for no LED ) 
	Also calls relay_reset on the relay and flushes
*/
void init_relay( relay_data* pRelay, int relay_pin, int relay_led );

/** Set the relay to closed and turn on the status LED */
void relay_set( relay_data* pRelay );
/** Set the relay to open and turn off the status LED */
void relay_reset( relay_data* pRelay );
/** Toggle the relay, calls relay_set or relay_reset */
void relay_toggle( relay_data* pRelay );

/** Actually changes the pin states to correspond with the relay_data */
void relay_flush( relay_data* pRelay );

/** Get the relay state. True is set, false is reset */
bool relay_state( relay_data* pRelay );