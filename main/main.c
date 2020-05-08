
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
#include "typedefs.h"
#include "thermostat_params_nvs_operations.h"


#define SHOW_ALL_LOGS false
#define SHOW_MAIN_LOGS true




	// ================================================
	//	HTTP definitions (default)
	// ================================================

	// ------- 	General 	-------
#define WEB_PORT "80"
#define EXPECTED_MAX_RESPONSE_LENGTH 500

	// ------- 	Commands 	-------
#define GATEBOX_STATE "/api/gate/state"
#define TEMPSENSOR_STATE "/api/tempsensor/state"
#define SWITCHBOX_STATE "/api/relay/state"
#define SWITCHBOX_ON "/s/1"
#define SWITCHBOX_OFF "/s/0"


	// ------- 	GATEBOX 	-------
//#define GATEBOX_HOST "192.168.1.105"

	// ------- 	TEMPSENSOR 	-------
#define TEMPSENSOR_MAIN_IP "192.168.1.201"
#define TEMPSENSOR_FUSE_IP "192.168.1.126"

	// ------- 	SWITCHBOX 	-------
#define SWITCHBOX_COMPRESSOR 	"192.168.1.245"
#define SWITCHBOX_FAN 			"192.168.1.153"



	// ================================================
	//	Static variables
	// ================================================

static const char *TAG = "MAIN";


	// ================================================
	//	Request Functions
	// ================================================


// Generates proper request for specified command, host ip, on default port 80
void setRequestString_malloc(destination_device *device){

	char *TAG = "setRequestString_malloc";

	if (device->request != NULL) {

		if (SHOW_ALL_LOGS) {
			ESP_LOGI(TAG, "Free alokated memory!");
		}
		free(device->request);
	}

	char temp[128];

	device->request_len = sprintf(temp, "GET %s HTTP/1.1\r\n"
										"Host: %s:80\r\n"
										"User-Agent: esp-idf/1.1 esp32\r\n"
										"\r\n", device->command, device->ip);

	device->request = malloc(device->request_len);

	for(uint8_t i = 0; i < device->request_len; i++){
		device->request[i] = temp[i];
	}

}


// Find Json in response
char* getJsonFromResponse_malloc(char *response, uint16_t response_length){

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

// Parsing temSensor state json response to structure
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

// Parsing gateBox state json response to structure
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

// Send one command (for now only to switchBox)
// Function not prepared to use in new thread!
void http_send_device_command(void *pvParameters)
{

	static const char* TAG = "HTTP_GET_DEVICE_STATE";


	destination_device *device = (destination_device*) pvParameters;
	device_name device_name = device->device_name;


	if (device_name == switchBox) {

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "Destination device: switchBox");
		}

		const struct addrinfo hints = {
		        .ai_family = AF_INET,
		        .ai_socktype = SOCK_STREAM
		};

		struct addrinfo *res;
		struct in_addr *addr;
		int s;

		int err = getaddrinfo(device->ip, WEB_PORT, &hints, &res);

		if (err != 0 || res == NULL) {
			ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}

		/* Code to print the resolved IP.

		 Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
		addr = &((struct sockaddr_in*) res->ai_addr)->sin_addr;
		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));
		}

		s = socket(res->ai_family, res->ai_socktype, 0);
		if (s < 0) {
			ESP_LOGE(TAG, "... Failed to allocate socket.");
			freeaddrinfo(res);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "... allocated socket");
		}

		if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
			ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
			close(s);
			freeaddrinfo(res);
			vTaskDelay(4000 / portTICK_PERIOD_MS);
		}

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "... connected");
		}

		freeaddrinfo(res);

		if (write(s, device->request, device->request_len) < 0) {
			ESP_LOGE(TAG, "... socket send failed");
			close(s);
			vTaskDelay(4000 / portTICK_PERIOD_MS);
		}

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "... socket send success");
		}

		close(s);

	}
	else {
		ESP_LOGE(TAG, "Not supported!");
	}
}


// Function is frequestly asking device api (for example /api/gate/state)
// Data are passed through device structure
void http_keep_asking_api(void *pvParameters)
{

	static const char* TAG = "HTTP_GET_DEVICE_STATE";


	destination_device *device = (destination_device*) pvParameters;
	device_name device_name = device->device_name;


    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    char resoponse[EXPECTED_MAX_RESPONSE_LENGTH];
    char *json;

    do{
        int err = getaddrinfo(device->ip, WEB_PORT, &hints, &res);

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

        if (write(s, device->request, device->request_len) < 0) {
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


		/* Read HTTP response */

		int iter = 0;
		do {
			bzero(recv_buf, sizeof(recv_buf));
			r = read(s, recv_buf, sizeof(recv_buf) - 1);

			for (int i = 0; i < r; i++) {

				if (iter < EXPECTED_MAX_RESPONSE_LENGTH) {
					resoponse[iter] = recv_buf[i];
					iter++;
				} else {
					ESP_LOGE(TAG,
							"Response lenght bigger than expected max response length!");
				}

			}
		} while (r > 0);


		json = getJsonFromResponse_malloc(resoponse,
				(uint16_t) EXPECTED_MAX_RESPONSE_LENGTH);

		if (device_name == gateBox) {

			api_gate_state gatebox_state = get_api_gate_state(json);
			free(json);

			device->data = gatebox_state.currentPos;
			device->data_received = true;

			if (SHOW_ALL_LOGS == true) {
				ESP_LOGI(TAG, "jSON GateBox Value %d: ",
						gatebox_state.currentPos);
			}

		} else if (device_name == tempSensor) {

			api_tempsensor_state_tempSensor_sensors_0 api_tempsensor_state =
					get_api_tempensor_state(json);
			free(json);

			float temperature = ((float) api_tempsensor_state.value) / 100;

			device->data = temperature;
			device->data_received = true;

			if (SHOW_ALL_LOGS == true) {
				ESP_LOGI(TAG, "jSON Tempsensor Value:  %.1f", temperature);
			}

		}


		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG,
					"... done reading from socket. Last read return=%d errno=%d.",
					r, errno);
			ESP_LOGI(TAG, "Waiting %d sec.", device->period);
		}

		close(s);

		vTaskDelay(device->period / portTICK_PERIOD_MS);

		if (SHOW_ALL_LOGS == true) {
			ESP_LOGI(TAG, "Starting again!");
		}


    }while(true);

    vTaskDelete(NULL);
}

// Send one command, don't read response
void switchbox_action(destination_device *switchBox_device, switchBox_command sc){

	if (switchBox_device != NULL) {
		if (switchBox_device->device_name == switchBox) {
			if (sc == ON) {
				switchBox_device->command = SWITCHBOX_ON;
			} else {
				switchBox_device->command = SWITCHBOX_OFF;
			}
			setRequestString_malloc(switchBox_device);
			http_send_device_command(switchBox_device);

		} else {
			ESP_LOGE("switchbox_action", "This device has no actions yet!");
		}
	}else{
		ESP_LOGE("switchbox_action", "NULL pointer!");
	}
}

// Check if Comressor is freezing
bool checkIfCompressorIsFreezing(thermostat_state *thermostat){
	if (thermostat->tempSensor_fuse->data_received){
		thermostat->tempSensor_fuse->data_received = false;
		if (thermostat->tempSensor_fuse->data < thermostat->thermParams->temp_fuse_low){
			thermostat->thermParams->isFreezing = true;
			ESP_LOGE("checkIfCompressorIsFreezing", "Device is freezing!");
		}
		else if (thermostat->tempSensor_fuse->data > thermostat->thermParams->temp_fuse_high){
			thermostat->thermParams->isFreezing = false;
			if (SHOW_ALL_LOGS) {
				ESP_LOGI("checkIfCompressorIsFreezing",
						"The temperature got bigger, turning on again");
			}
		}
		else{
			if (SHOW_ALL_LOGS) {
				ESP_LOGI("checkIfCompressorIsFreezing",
						"Doing nothing temperature between %d and %d!", thermostat->thermParams->temp_fuse_low, thermostat->thermParams->temp_fuse_high);
			}
		}
	}
	else{
		if (SHOW_ALL_LOGS){
			ESP_LOGI("checkIfCompressorIsFreezing", "No fresh data to read!");
		}
	}

	return thermostat->thermParams->isFreezing;
}


esp_err_t heaterControl(thermostat_state *thermostat){
	if (thermostat->thermParams->power){
		if(!checkIfCompressorIsFreezing(thermostat)){
			if (thermostat->tempSensor_main->data_received) {
				thermostat->tempSensor_main->data_received = false;

				if (thermostat->tempSensor_main->data < thermostat->thermParams->temp_low) {
					switchbox_action(thermostat->switchBox_compressor, ON);
					switchbox_action(thermostat->switchBox_fan, ON);

					if (SHOW_MAIN_LOGS == true) {
						ESP_LOGI(TAG, "Turning on compressor");
					}
				} else if (thermostat->tempSensor_main->data > thermostat->thermParams->temp_high) {
					switchbox_action(thermostat->switchBox_compressor, OFF);
					switchbox_action(thermostat->switchBox_fan, OFF);

					if (SHOW_MAIN_LOGS == true) {
						ESP_LOGI(TAG, "Turning off compressor");
					}
				} else {

					if (SHOW_MAIN_LOGS == true) {
						ESP_LOGI(TAG,
								"Doing nothing! Temperature is between %d and %d deg->",
								thermostat->thermParams->temp_low, thermostat->thermParams->temp_high);
					}
				}
			} else {
				ESP_LOGW(TAG,
						"There was no data received yet! Check the tempSensor!");
			}
		}
		else {
			ESP_LOGW(TAG, "Device is freezing! Turning compressor OFF and fan ON");
			switchbox_action(thermostat->switchBox_compressor, OFF);
			switchbox_action(thermostat->switchBox_fan, ON);
		}
	}
	else {
		ESP_LOGI(TAG, "Main power setting is off, turning down compressor and fan");
		switchbox_action(thermostat->switchBox_compressor, OFF);
		switchbox_action(thermostat->switchBox_fan, OFF);
	}

	return ESP_OK;
}


	// ================================================
	//	APP MAIN
	// ================================================

void app_main(void) {

	// AP server start and connect to wifi
	ap_client_start();


	thermostat_params thermParams;
	update_thermostat_params_nvs_flash(&thermParams, true);
	thermParams.ip = "192.168.1.129";

	// declare devices

	// tempSensor main init
	static destination_device tempSensor_main;
	tempSensor_main.device_name = tempSensor;
	tempSensor_main.data = 0;
	tempSensor_main.ip = TEMPSENSOR_MAIN_IP;
	tempSensor_main.command = TEMPSENSOR_STATE;
	tempSensor_main.data_received = false;
	tempSensor_main.period = 3000;
	setRequestString_malloc(&tempSensor_main);

	// tempSensor fuse init
	static destination_device  tempSensor_fuse;
	tempSensor_fuse.device_name = tempSensor;
	tempSensor_fuse.data = 0;
	tempSensor_fuse.ip = TEMPSENSOR_FUSE_IP;
	tempSensor_fuse.command = TEMPSENSOR_STATE;
	tempSensor_fuse.data_received = false;
	tempSensor_fuse.period = 30000;
	setRequestString_malloc(&tempSensor_fuse);


	// switchBox compressor init
	static destination_device switchBox_compressor;
	switchBox_compressor.ip = SWITCHBOX_COMPRESSOR;
	switchBox_compressor.device_name = switchBox;

	// switchBox fan init
	static destination_device switchBox_fan;
	switchBox_fan.ip = SWITCHBOX_FAN;
	switchBox_fan.device_name = switchBox;

	//	thermostat_state init
	thermostat_state thermState;
	thermState.tempSensor_main = &tempSensor_main;
	thermState.tempSensor_fuse = &tempSensor_fuse;
	thermState.switchBox_compressor = &switchBox_compressor;
	thermState.switchBox_fan = &switchBox_fan;
	thermState.thermParams = &thermParams;

	// http server start (for html settings)
	http_server_start(&thermState);


	// Run thread periodically checking state of tempSensor
	xTaskCreate(&http_keep_asking_api, "http_keep_asking_api", 4096, &tempSensor_main, 5, NULL);
	xTaskCreate(&http_keep_asking_api, "http_keep_asking_api", 4096, &tempSensor_fuse, 5, NULL);


	vTaskDelay(5000 / portTICK_PERIOD_MS);


	while(1){

		heaterControl(&thermState);

		ESP_LOGI(TAG, "TempSensor main: %.1f", tempSensor_main.data);
		ESP_LOGI(TAG, "TempSensor fuse: %.1f", tempSensor_fuse.data);

		// TODO : repair control panel
		//setWebPage_malloc(&thermState);

		uint32_t freeHeap = xPortGetFreeHeapSize();
		ESP_LOGI(TAG, "Free heap size: %d", freeHeap);

		if (thermParams.period){
			ESP_LOGI(TAG, "Thermostat period: %d", thermParams.period);
			vTaskDelay(thermParams.period / portTICK_PERIOD_MS);
		}
		else{
			vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}
}

