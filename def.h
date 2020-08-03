#pragma once

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define DEBUG_SERIAL

#define PIN_LED_POWER	12
#define PIN_LED_STATUS	14

#define PIN_RELAY		13

#define PIN_IN_SETBTN	0

#define	PIN_IN_OPTO1	5
#define PIN_IN_OPTO2	4

/** The name used to identify the device on the wifi network */
#define WIFI_HOSTNAME	"Multiwire-Relay"
/** Number of seconds to wait before timing out wifi connection */
#define WIFI_TIMEOUT	5

/** How long to wait before another set button depressed can be triggered */
#define SET_BTN_DEBOUNCE_MS 500

#define JSON_DOC_SIZE		1024

#define FEATHERSTONE_DEVICE_TYPE	4096		// 0x1000
#define DEVICE_UID					640878103
#define COMMAND_DISCOVERY			"discovery"
#define COMMAND_DISCOVERY_QUERY		"{\"command\": \"discovery_ok\", \"device_id\": " STR(FEATHERSTONE_DEVICE_TYPE) \
									", \"device_uid\": " STR(DEVICE_UID) "}"

#define COMMAND_ACTION_OK			"{\"command\": \"action_ok\", \"device_uid\": " STR(DEVICE_UID) "}"
#define COMMAND_GET_STATE_SET		"{\"command\": \"get_state\", \"device_uid\": " STR(DEVICE_UID) ", \"state\": true}"
#define COMMAND_GET_STATE_RESET		"{\"command\": \"get_state\", \"device_uid\": " STR(DEVICE_UID) ", \"state\": false}"

/** Should be defined in main .ino file */
void fault();