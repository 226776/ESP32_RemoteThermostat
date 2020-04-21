
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "cJSON.h"

// User defined

#include "ap_client.h"
#include "http_server_webpage.h"


#define SHOW_ALL_LOGS false


	// ================================================
	//	AP client definitions
	// ================================================

#ifndef WIFI_SSID
	// Put your SSID
#define	WIFI_SSID "majesiec"

	// Put your PASSWORD
#define	WIFI_PASS "youpassword"

	// Put max retrys
#define	MAXIMUM_RETRY 5
#endif


	// ================================================
	//	HTTP definitions
	// ================================================

	// ------- 	General 	-------
#define WEB_PORT "80"
#define EXPECTED_MAX_RESPONSE_LENGTH 500

#define WEB_SERVER "192.168.1.201"
#define WEB_PATH "/api/tempsensor/state"

	// ------- 	GATEBOX 	-------
#define GATEBOX_HOST "192.168.1.105"
#define GATEBOX_STATE "/api/gate/state"

	// ------- 	TEMPSENSOR 	-------
#define TEMPSENSOR_HOST "192.168.1.201"
#define TEMPSENSOR_STATE "/api/tempsensor/state"

	// ------- 	SWITCHBOX 	-------
#define SWITCHBOX_HOST "192.168.1.245"
#define SWITCHBOX_ON "/s/1"
#define SWITCHBOX_OFF "/s/0"

	// ================================================
	//	Static variables
	// ================================================

static const char *TAG = "MAIN";

static const char *REQUEST = "GET " WEB_PATH " HTTP/1.1\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.1 esp32\r\n"
    "\r\n";

static const char *GATEBOX_REQUEST = "GET " GATEBOX_STATE " HTTP/1.1\r\n"
    "Host: "GATEBOX_HOST":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.1 esp32\r\n"
    "\r\n";

static const char *TEMPSENSOR_REQUEST = "GET " TEMPSENSOR_STATE " HTTP/1.1\r\n"
    "Host: "TEMPSENSOR_HOST":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.1 esp32\r\n"
    "\r\n";

static const char *SWITCHBOX_ON_REQUEST = "GET " SWITCHBOX_ON " HTTP/1.1\r\n"
    "Host: "SWITCHBOX_HOST":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.1 esp32\r\n"
    "\r\n";

static const char *SWITCHBOX_OFF_REQUEST = "GET " SWITCHBOX_OFF " HTTP/1.1\r\n"
    "Host: "SWITCHBOX_HOST":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.1 esp32\r\n"
    "\r\n";


	// ================================================
	//	Type Definitions
	// ================================================

// **************** General	 	****************

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
	uint8_t device_name;
	uint8_t command;
	uint8_t command_type;
	float 	data;

}destination_device;

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


	// ================================================
	//	Test Functions
	// ================================================


typedef struct {
	uint8_t Val1;
	uint8_t Val2;
}test_struct;


void test_func(void *pvParameters){

	test_struct *initVals = (test_struct *) pvParameters;

	printf("Test success? : %d , %d\n", initVals->Val1, initVals->Val2);



	vTaskDelete(NULL);
}

	// ================================================
	//	Request Functions
	// ================================================

char* getJsonFromResponse(char *response, uint16_t response_length){

		uint16_t json_begin = 0;
		uint16_t json_end = 0;
		uint16_t json_len = 0;
		uint8_t count_begin = 0;
		uint8_t count_end = 0;

		char *json;


		for (int i = 0; i < response_length; i++) {

			if (response[i] == '{') {
				count_begin++;
				if (count_begin == 1) {
					json_begin = i;

					if(SHOW_ALL_LOGS == true){
						ESP_LOGI(TAG, "Found Json beginning: %d", i);
					}

				}
			} else if (response[i] == '}') {
				count_end++;
				if (count_end == count_begin) {
					json_end = i;

					if (SHOW_ALL_LOGS == true) {
						ESP_LOGI(TAG, "Found Json end: %d", i);
					}

				}
			}
		}

		json_len = json_end - json_begin;

		if(json_len > 0){
			json_len ++;

			json = (char *) calloc(json_len, sizeof(char));

			for (int k = 0; k < json_len; k++){
				json[k] = response[k + json_begin];
			}

			if (SHOW_ALL_LOGS == true) {
				ESP_LOGI(TAG, "Returning json, json length: %d", json_len);
			}


			return json;

		}
		else{
			ESP_LOGE(TAG, "Response Incorrect, json length: %d", json_len);
			return NULL;
		}
}

api_tempsensor_state_tempSensor_sensors_0 get_api_tempensor_state(char *json){

	static const char *TEMPSESOR_TAG = "get_api_tempensor_state";


	// Only intrest at that moment is value (temperature), rest will be allways default
	api_tempsensor_state_tempSensor_sensors_0 tempsensor_state = API_TEMPSENSOR_STATE_DEFAULT();

	cJSON *root = cJSON_Parse(json);
	if (!cJSON_IsObject(root)) {
		ESP_LOGE(TEMPSESOR_TAG, "JSON root error!");
	}

	cJSON *tempSensor = cJSON_GetObjectItem(root, "tempSensor");
	if (!cJSON_IsObject(tempSensor)) {
		ESP_LOGE(TEMPSESOR_TAG, "JSON root->tempSensor error!");
	}

	cJSON *sensors = cJSON_GetObjectItem(tempSensor, "sensors");
	if (!cJSON_IsArray(sensors)) {
		ESP_LOGE(TEMPSESOR_TAG, "JSON sensors isn't array!");
	}

	cJSON *sensors0 = cJSON_GetArrayItem(sensors, 0);
	if (!cJSON_IsArray(sensors)) {
		ESP_LOGE(TEMPSESOR_TAG, "JSON sensors0 error!");
	}

	cJSON *value = cJSON_GetObjectItem(sensors0, "value");

	tempsensor_state.value = value->valueint;

	//ESP_LOGI(TEMPSESOR_TAG, "cJSON Tempsensor: %d", tempsensor_state.value);

	cJSON_Delete(root);

	return tempsensor_state;
}


api_gate_state get_api_gate_state(char *json){

	static const char *GATEBOX_TAG = "GateBox jSON Parse";

	// Only interest at that moment is currentPos, rest will be always default
	api_gate_state gatebox_state = API_GATE_STATE_DEFAULT();

	cJSON *root = cJSON_Parse(json);
	if(SHOW_ALL_LOGS == true){
		ESP_LOGI(GATEBOX_TAG, "cJSON root created");
	}

	// currentPos override
	cJSON *currentPos = cJSON_GetObjectItem(root, "currentPos");
	gatebox_state.currentPos = currentPos->valueint;

	if(SHOW_ALL_LOGS == true){
		ESP_LOGI(GATEBOX_TAG, "cJSON GATE: CurrentPos Value %d", gatebox_state.currentPos);
	}

	cJSON_Delete(root);

	return gatebox_state;
}


static void http_get_device_state(void *pvParameters)
{

	bool sendPeriodycally = true;
	bool readResponse = true;

	char* device_ip = WEB_SERVER;
	char* request = REQUEST;

	destination_device *device = (test_struct*) pvParameters;
	device_name device_name = device->device_name;

	if (device_name == gateBox) {

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "Destination device: switchBox");
		}

		device_ip = GATEBOX_HOST;
		request = GATEBOX_REQUEST;

	} else if (device_name == tempSensor) {

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "Destination device: tempSensor");
		}

		device_ip = TEMPSENSOR_HOST;
		request = TEMPSENSOR_REQUEST;

	} else if (device_name == switchBox) {

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "Destination device: switchBox");
		}

		device_ip = SWITCHBOX_HOST;

		// Asign proper request
		if (device->command == ON) {
			request = SWITCHBOX_ON_REQUEST;
		} else {
			request = SWITCHBOX_OFF_REQUEST;
		}

		sendPeriodycally = false;

	}
	else {
		ESP_LOGE(TAG, "No such device!");
	}

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    do{
        int err = getaddrinfo(device_ip, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        if (SHOW_ALL_LOGS == true) {
        	ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));
        }

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        if (SHOW_ALL_LOGS == true) {
        	ESP_LOGI(TAG, "... allocated socket");
        }

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        if (SHOW_ALL_LOGS == true) {
        	ESP_LOGI(TAG, "... connected");
        }

        freeaddrinfo(res);

        if (write(s, request, strlen(request)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        if (SHOW_ALL_LOGS == true) {
        	ESP_LOGI(TAG, "... socket send success");
        }

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        if (SHOW_ALL_LOGS == true) {
        	ESP_LOGI(TAG, "... set socket receiving timeout success");
        }

        // =============================================================================================


        char resoponse[EXPECTED_MAX_RESPONSE_LENGTH];
        char* json;

        if(readResponse) {

			/* Read HTTP response */

			int iter = 0;
			do {
				bzero(recv_buf, sizeof(recv_buf));
				r = read(s, recv_buf, sizeof(recv_buf)-1);

				for(int i = 0; i < r; i++) {

					if(iter < EXPECTED_MAX_RESPONSE_LENGTH){
						resoponse[iter] = recv_buf[i];
						iter ++;
					}
					else{
						ESP_LOGE(TAG, "Response lenght bigger than expected max response length!");
					}

				}
			} while(r > 0);

			// =============================================================================================

			json = getJsonFromResponse(resoponse, (uint16_t) EXPECTED_MAX_RESPONSE_LENGTH);


			if (device_name == gateBox) {

				api_gate_state gatebox_state = get_api_gate_state(json);

				device->data = gatebox_state.currentPos;

				if (SHOW_ALL_LOGS == true) {
					ESP_LOGI(TAG, "jSON GateBox Value %d: ",
							gatebox_state.currentPos);
				}



			} else if (device_name == tempSensor) {

				api_tempsensor_state_tempSensor_sensors_0 api_tempsensor_state = get_api_tempensor_state(json);

				float temperature = ((float )api_tempsensor_state.value) / 100;

				device->data = temperature;

				if (SHOW_ALL_LOGS == true) {
					ESP_LOGI(TAG, "jSON Tempsensor Value:  %.1f", temperature);
				}

			}


			// =============================================================================================

			if (SHOW_ALL_LOGS == true) {
				ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
			}

			close(s);
			for(int countdown = 1; countdown >= 0; countdown--) {
				if (SHOW_ALL_LOGS == true) {
					ESP_LOGI(TAG, "%d... ", countdown);
				}

				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}
			if (SHOW_ALL_LOGS == true) {
				ESP_LOGI(TAG, "Starting again!");
			}
        }

    }while(sendPeriodycally);

    vTaskDelete(NULL);

}



	// ================================================
	//	APP MAIN
	// ================================================

void app_main(void) {

	// AP server start and connect to wifi
	ap_client_start();

	// http server start (for html settings)
	http_server_start();

	// declare devices
	// tempSensor
	static destination_device temp;
	static device_name temp_dn = tempSensor;
	temp.device_name = temp_dn;

	// gateBox
	static destination_device gate;
	static device_name gateName = gateBox;
	gate.device_name = gateName;

	// switchBox
	static destination_device sw;
	static device_name sw_dn = switchBox;
	static switchBox_command switchBoxCommand = ON;
	sw.device_name = sw_dn;
	sw.command = switchBoxCommand;


	// Run thread periodically checking state of tempSensor
	xTaskCreate(&http_get_device_state, "http_get_device_state", 4096, &temp, 5, NULL);
	// Run thread periodically checking state of gateBox
	xTaskCreate(&http_get_device_state, "http_get_device_state", 4096, &gate, 5, NULL);


	while(1){

		ESP_LOGI(TAG, "TempSensor: 	%.1f", temp.data);
		ESP_LOGI(TAG, "GateBox: 	%.0f", gate.data);

		/*
		sw.command = OFF;
		xTaskCreate(&http_get_device_state, "http_get_device_state", 4096, &sw, 5, NULL);

		sw.command = ON;
		xTaskCreate(&http_get_device_state, "http_get_device_state", 4096, &sw,5, NULL);
		*/

		vTaskDelay(6000 / portTICK_PERIOD_MS);
	}
}

