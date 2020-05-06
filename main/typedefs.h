
#ifndef REMOTE_THERMOSTAT_TYPEDEFS
#define REMOTE_THERMOSTAT_TYPEDEFS

#include "freertos/FreeRTOS.h"
#include "esp_system.h"

// **************** General  	****************

typedef enum {
	tempSensor = 0,
	gateBox = 1,
	switchBox = 2,
}device_name;

typedef enum {
	OFF = 0,
	ON = 1,
}switchBox_command;

typedef enum {
	GET = 0,
	SET = 1,
}command_type;

typedef struct {
	uint8_t 	device_name;
	char*		ip;
	char*		request;
	char*		command;
	uint8_t 	request_len;
	float 		data;

}destination_device;

typedef struct {
	uint8_t 	temp_high;
	uint8_t 	temp_low;
	bool 		power;
	uint16_t 	restart_count;
}thermostat_params;


// **************** TempSensor 	****************

typedef struct {
	char *type;
	uint8_t id;
	uint32_t value;
	uint8_t trend;
	uint8_t state;
	uint32_t elapsedTimeS;

} api_tempsensor_state_tempSensor_sensors_0;

#define API_TEMPSENSOR_STATE_DEFAULT() { \
    .type = "temperature", \
    .id = 0, \
    .value = 0, \
	.trend = 0, \
	.state = 2, \
	.elapsedTimeS = 0, \
};

// **************** GateBox  	****************

typedef struct {
	uint8_t currentPos;
	uint8_t desiredPos;
	uint8_t extraButtonType;
	uint8_t extraButtonRelayNumber;
	uint32_t extraButtonPulseTimeMs;
	uint8_t	extraButtonInvert;
	uint8_t gateRelayNumber;
	uint8_t	openLimitSwitchInputNumber;
	uint8_t	closeLimitSwitchInputNumber;
	uint8_t	gateType;
	uint32_t gatePulseTimeMs;
	uint8_t gateInvert;
	uint8_t inputsType;
	uint8_t* fieldsPreferences;

} api_gate_state;


#define API_GATE_STATE_DEFAULT() { \
    .currentPos = 50, \
    .desiredPos = 50, \
    .extraButtonType = 3, \
	.extraButtonRelayNumber = 1, \
	.extraButtonPulseTimeMs = 100, \
	.extraButtonInvert = 0, \
	.gateRelayNumber = 0, \
	.openLimitSwitchInputNumber = 0, \
	.closeLimitSwitchInputNumber = 1, \
	.gateType = 0, \
	.gatePulseTimeMs = 100, \
	.gateInvert = 0, \
	.inputsType = 0, \
	.fieldsPreferences = NULL\
};

typedef struct {
	destination_device*	tempSensor;
	destination_device* gateBox;
	thermostat_params* thermParams;
} thermostat_state;


#endif
