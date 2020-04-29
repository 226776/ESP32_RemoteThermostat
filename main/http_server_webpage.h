#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "cJSON.h"

#include <esp_http_server.h>

#include "web_page.h"
#include "typedefs.h"
#include "thermostat_params_nvs_operations.h"


	// ================================================
	//		HTTP Server Services / WebPage
	// ================================================

static esp_err_t send_thermostat_state_handler(httpd_req_t *req){

	thermostat_state *thermState = (thermostat_state*)req->user_ctx;
	destination_device *tempSensor = (destination_device*)thermState->tempSensor;
	destination_device *gateBox = (destination_device*)thermState->gateBox;
	thermostat_params *thermParams = (thermostat_params*)thermState->thermParams;

	cJSON *root;

	root = cJSON_CreateObject();

	cJSON_AddItemToObject(root, "Current Temperature", cJSON_CreateNumber((double)tempSensor->data));
	if(gateBox->data != 50){
		cJSON_AddItemToObject(root, "Current MATSUI state", cJSON_CreateString("ON"));
	}
	else{
		cJSON_AddItemToObject(root, "Current MATSUI state", cJSON_CreateString("OFF"));
	}
	if(thermParams->power){
		cJSON_AddItemToObject(root, "Current Power Setting", cJSON_CreateString("ON"));
	}
	else{
		cJSON_AddItemToObject(root, "Current Power Setting", cJSON_CreateString("OFF"));
	}
	cJSON_AddItemToObject(root, "High Temperature Threshold", cJSON_CreateNumber((double)thermParams->temp_high));
	cJSON_AddItemToObject(root, "Low Temperature Threshold", cJSON_CreateNumber((double)thermParams->temp_low));

	uint32_t freeHeap = xPortGetFreeHeapSize();
	cJSON_AddItemToObject(root, "Free Heap Size", cJSON_CreateNumber((double)freeHeap));


	char* resp_str = cJSON_Print(root);



	//sprintf(resp_str, "<html><h1>Thermostat State:<\h1><\html> \n Current Temperature %.1f \n ",tempSensor->data);
	httpd_resp_send(req, resp_str, strlen(resp_str));


	cJSON_Delete(root);
	free(resp_str);

	return ESP_OK;
}




httpd_uri_t send_thermostat_state = {
    .uri       = "/state",
    .method    = HTTP_GET,
    .handler   = send_thermostat_state_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


//	************** 	Set Off thermostat		**************

static const char *HTTP_SERVER_TAG = "HTTP_SERVER";

static esp_err_t set_off_thermostat_handler(httpd_req_t *req){

	thermostat_params *thermParams = (thermostat_params*) req->user_ctx;

	thermParams->power = false;

	update_thermostat_params_nvs_flash(thermParams, false);

	ESP_LOGW(HTTP_SERVER_TAG, "Turning thermostat OFF!");

    return ESP_OK;
}


httpd_uri_t set_off_thermostat = {
    .uri       = "/off",
    .method    = HTTP_POST,
    .handler   = set_off_thermostat_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


//	************** 	Set On thermostat 		**************

static esp_err_t set_on_thermostat_handler(httpd_req_t *req){


		thermostat_params *thermParams = (thermostat_params*) req->user_ctx;

		thermParams->power = true;

		update_thermostat_params_nvs_flash(thermParams, false);

		ESP_LOGW(HTTP_SERVER_TAG, "Turning thermostat ON!");

    return ESP_OK;
}


httpd_uri_t set_on_thermostat = {
    .uri       = "/on",
    .method    = HTTP_POST,
    .handler   = set_on_thermostat_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


//	************** 	Set Low threshold 		**************

static esp_err_t set_temp_low_handler(httpd_req_t *req){

    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        float temp;
		char *pend;
		temp = strtof(buf, &pend);

		thermostat_params *thermParams = (thermostat_params*) req->user_ctx;

		if (temp <= thermParams->temp_high) {
			thermParams->temp_low = (uint8_t)temp;

			update_thermostat_params_nvs_flash(thermParams, false);

			ESP_LOGI(HTTP_SERVER_TAG, "Setting up Temp High to : %.0f", temp);

		} else {
			ESP_LOGE(HTTP_SERVER_TAG, "Temp Low can't be bigger than Temp High!");
		}


    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


httpd_uri_t set_temp_low = {
    .uri       = "/s/l",
    .method    = HTTP_POST,
    .handler   = set_temp_low_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


//	************** 	Set High  threshold 	**************

static esp_err_t set_temp_high_handler(httpd_req_t *req){

    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        float temp;
		char *pend;
		temp = strtof(buf, &pend);

		thermostat_params *thermParams = (thermostat_params*) req->user_ctx;
		if(temp >= thermParams->temp_low){
			thermParams->temp_high = (uint8_t)temp;

			update_thermostat_params_nvs_flash(thermParams, false);

			ESP_LOGI(HTTP_SERVER_TAG, "Setting up Temp Low to : %.0f", temp);

		}
		else{
			ESP_LOGE(HTTP_SERVER_TAG, "Temp High can't be lower than Temp Low!");
		}

    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

httpd_uri_t set_temp_high = {
    .uri       = "/s/h",
    .method    = HTTP_POST,
    .handler   = set_temp_high_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};



//	************** 	Control Panel View  	**************

/* An HTTP GET handler */
static esp_err_t control_panel_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTP_SERVER_TAG, "Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTP_SERVER_TAG, "Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTP_SERVER_TAG, "Found URL query parameter => query2=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(HTTP_SERVER_TAG, "Request headers lost");
    }
    return ESP_OK;
}


static const httpd_uri_t control_panel = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = control_panel_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = webPage
};

//	************** 	Error Handlers  	**************

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(HTTP_SERVER_TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(HTTP_SERVER_TAG, "%.*s", ret, buf);
        ESP_LOGI(HTTP_SERVER_TAG, "====================================");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};



/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}



//	************** 	Start WebServer	 	**************

static httpd_handle_t start_webserver(void *pvParameters)
{

	thermostat_state *thermState = (thermostat_state*)pvParameters;
	send_thermostat_state.user_ctx = thermState;


	thermostat_params *thermParams = (thermostat_params*)thermState->thermParams;
	set_temp_high.user_ctx = thermParams;
	set_temp_low.user_ctx = thermParams;
	set_off_thermostat.user_ctx = thermParams;
	set_on_thermostat.user_ctx = thermParams;

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(HTTP_SERVER_TAG, "Params correct: %d, %d", thermParams->temp_high, thermParams->temp_low);

    // Start the httpd server
    ESP_LOGI(HTTP_SERVER_TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(HTTP_SERVER_TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &control_panel);
        httpd_register_uri_handler(server, &set_temp_high);
        httpd_register_uri_handler(server, &set_temp_low);
        httpd_register_uri_handler(server, &set_off_thermostat);
        httpd_register_uri_handler(server, &set_on_thermostat);
        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &send_thermostat_state);
        return server;
    }

    ESP_LOGI(HTTP_SERVER_TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(HTTP_SERVER_TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(HTTP_SERVER_TAG, "Starting webserver");
        *server = start_webserver(NULL);
    }
}


void http_server_start(void *pvParameters){

    static httpd_handle_t server = NULL;

    //ESP_ERROR_CHECK(nvs_flash_init());
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());


#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    /* Start the server for the first time */
    server = start_webserver(pvParameters);


}
