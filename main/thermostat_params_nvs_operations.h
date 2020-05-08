
#ifndef THERMOSTAT_PARAMS_NVS_OPERATIONS
#define THERMOSTAT_PARAMS_NVS_OPERATIONS

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "typedefs.h"

char *TPNO_TAG = "THERMOSTAT_PARAMS_NVS_OPERATIONS";

/*
 *  If set to false, show only error logs!
 */
#define THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS true


void read_error_check(esp_err_t err){

	switch (err) {
		case ESP_OK:
			if (THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS) {
				ESP_LOGI(TPNO_TAG, "Reading Done. ");
			}
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGW(TPNO_TAG, "The value is not initialized yet!");
			break;
		default :
			ESP_LOGE(TPNO_TAG, "Error (%s) reading/writing!", esp_err_to_name(err));
	}
}


/*
 * 	If read is set to true, function reads thermParams from nvs_flash.
 * 	If read is set to false, function writes thermParams to nvs_flash.
 */
void update_thermostat_params_nvs_flash(thermostat_params *thermParams, bool read){


	// Initialize NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	// Open

		if(THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS){
			ESP_LOGI(TPNO_TAG, "Opening Non-Volatile Storage (NVS) handle... ");
		}

	    nvs_handle_t my_nvs_handle;
	    err = nvs_open("storage", NVS_READWRITE, &my_nvs_handle);
	    if (err != ESP_OK) {
	    	ESP_LOGE(TPNO_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	    } else {
			if (THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS) {
				ESP_LOGI(TPNO_TAG, "Opening Done. Reading restart counter from NVS ... ");
			}

			if(read){
				// Read

				// default values (if there are no in memory)
				thermParams->temp_high = 52;
				thermParams->temp_low = 48;
				thermParams->power = true;
				thermParams->isFreezing = false;
				thermParams->period = 8000;
				thermParams->temp_fuse_high = 7;
				thermParams->temp_fuse_low = 2;


				err = nvs_get_u8(my_nvs_handle, "temp_high", &thermParams->temp_high);
				read_error_check(err);

				err = nvs_get_u8(my_nvs_handle, "temp_low", &thermParams->temp_low);
				read_error_check(err);

				err = nvs_get_u8(my_nvs_handle, "power", &thermParams->power);
				read_error_check(err);

				err = nvs_get_u32(my_nvs_handle, "period", &thermParams->period);
				read_error_check(err);

				err = nvs_get_u8(my_nvs_handle, "temp_fuse_high", &thermParams->temp_fuse_high);
				read_error_check(err);

				err = nvs_get_u8(my_nvs_handle, "temp_fuse_low", &thermParams->temp_fuse_low);
				read_error_check(err);

				if (THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS) {
					ESP_LOGI(TPNO_TAG, "temp_high: %d", thermParams->temp_high);
					ESP_LOGI(TPNO_TAG, "temp_low: %d", thermParams->temp_low);
					ESP_LOGI(TPNO_TAG, "power: %d", thermParams->power);

				}

				// Close
				nvs_close(my_nvs_handle);
			}
			else{
				// Write

				if (THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS) {
					ESP_LOGI(TPNO_TAG, "Updating restart counter in NVS ... ");
				}

				err = nvs_set_u8(my_nvs_handle, "temp_high",
						thermParams->temp_high);
				read_error_check(err);

				err = nvs_set_u8(my_nvs_handle, "temp_low",
						thermParams->temp_low);
				read_error_check(err);

				err = nvs_set_u8(my_nvs_handle, "power",
						thermParams->power);
				read_error_check(err);

				err = nvs_set_u8(my_nvs_handle, "period", thermParams->period);
				read_error_check(err);

				err = nvs_set_u8(my_nvs_handle, "temp_fuse_high",
					thermParams->temp_fuse_high);
				read_error_check(err);

				err = nvs_set_u8(my_nvs_handle, "temp_fuse_low",
					thermParams->temp_fuse_low);
				read_error_check(err);


				// Commit written value.
				// After setting any values, nvs_commit() must be called to ensure changes are written
				// to flash storage. Implementations may write to storage at other times,
				// but this is not guaranteed.
				if (THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS) {
					ESP_LOGI(TPNO_TAG, "Committing updates in NVS ... ");
				}
				err = nvs_commit(my_nvs_handle);
				if (err != ESP_OK){
					ESP_LOGE(TPNO_TAG, "Committing updates Failed");
				}
				else{
					if (THERMOSTAT_PARAMS_NVS_OPERATIONS_SHOW_LOGS) {
						ESP_LOGI(TPNO_TAG, "Committing updates Done");
					}
				}

				// Close
				nvs_close(my_nvs_handle);
			}
	    }

}

#endif
