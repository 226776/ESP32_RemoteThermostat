

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


#define SHOW_ALL_LOGS false


	// ================================================
	//	AP client definitions
	// ================================================


#define WIFI_NIEMCEWICZA


#if defined(WIFI_BLEBOX)

#define WIFI_SSID      "KingsMan"
#define WIFI_PASS      "ILoveBleBox"
#define MAXIMUM_RETRY  5

#elif defined(WIFI_DZIWISZOW)

#define WIFI_SSID      "Macek dlink"
#define WIFI_PASS      "macekmackiewicz"
#define MAXIMUM_RETRY  5

#elif defined(WIFI_NIEMCEWICZA)

#define WIFI_SSID      "SweetestPerfection"
#define WIFI_PASS      "Ni@@Ni@@22"
#define MAXIMUM_RETRY  5

#else

#define WIFI_SSID      "AXDition"
#define WIFI_PASS      "OFFCAPSLOCKIDIOT"
#define MAXIMUM_RETRY  5

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
#define SWITCHBOX_HOST "192.168.9.104"
#define SWITCHBOX_ON "/s/1"
#define SWITCHBOX_OFF "/s/0"

	// ================================================
	//	Static variables
	// ================================================

static const char *TAG = "OvenController";

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
    "Host: "TEMPSENSOR_HOST":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.1 esp32\r\n"
    "\r\n";

static const char *SWITCHBOX_OFF_REQUEST = "GET " SWITCHBOX_OFF " HTTP/1.1\r\n"
    "Host: "TEMPSENSOR_HOST":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.1 esp32\r\n"
    "\r\n";


	// ================================================
	//	Type Definitions
	// ================================================

// **************** General	 	****************

typedef struct {
	uint8_t device_name;
}destination_device;

typedef enum {
	tempSensor = 0,
	gateBox = 1,
}device_name;


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


static void http_send_command(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

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
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        // =============================================================================================


		#define EXPECTED_MAX_RESPONSE_LENGTH 500
        char resoponse[EXPECTED_MAX_RESPONSE_LENGTH];
        char* json;


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

        json = getJsonFromResponse(resoponse, (uint16_t) EXPECTED_MAX_RESPONSE_LENGTH);

        //api_gate_state gatebox_state = get_api_gate_state(json);

        api_tempsensor_state_tempSensor_sensors_0 api_tempsensor_state = get_api_tempensor_state(json);

        //ESP_LOGI(TAG, "jSON GateBox Value %d: ", gatebox_state.currentPos);
        ESP_LOGI(TAG, "jSON Tempsensor Value:  %.1f", ((float)api_tempsensor_state.value)/100);


        // print only json
        printf("\n--------------------------------------------\n");
        if(json == NULL){
        	ESP_LOGE(TAG, "ERRR");
        }
        else{
        	printf("%s", json);
        	printf("\nSomething\n");
        }





		// =============================================================================================


        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 1; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

static void http_get_device_state(void *pvParameters)
{

	char* device_ip = WEB_SERVER;
	char* request = REQUEST;

	destination_device *param = (test_struct*) pvParameters;
	device_name device_name = param->device_name;

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

    while(1) {
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

			ESP_LOGI(TAG, "jSON GateBox Value %d: ", gatebox_state.currentPos);

		} else if (device_name == tempSensor) {

			api_tempsensor_state_tempSensor_sensors_0 api_tempsensor_state = get_api_tempensor_state(json);

			ESP_LOGI(TAG, "jSON Tempsensor Value:  %.1f", ((float )api_tempsensor_state.value) / 100);
		}


		// =============================================================================================

        if (SHOW_ALL_LOGS == true) {
        	ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        }

        close(s);
        for(int countdown = 1; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

	// ================================================
	//	AP client handlers / Init
	// ================================================


/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

void ap_client_start() {
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	wifi_init_sta();
}

	// ================================================
	//	APP MAIN
	// ================================================

void app_main(void) {
	// AP server start
	ap_client_start();

	static destination_device temp;
	static destination_device gate;
	static device_name temp_dn = tempSensor;
	static device_name gate_dn = gateBox;

	temp.device_name = temp_dn;
	gate.device_name = gate_dn;

	//xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
	xTaskCreate(&http_get_device_state, "htpp_get_device_state", 4096, &temp, 5, NULL);
	xTaskCreate(&http_get_device_state, "htpp_get_device_state", 4096, &gate, 5, NULL);
}

