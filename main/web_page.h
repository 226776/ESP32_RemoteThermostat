
#ifndef WEB_PAGE
#define WEB_PAGE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "typedefs.h"

#define SHOW_WEB_PAGE_FUNCTION_LOGS true

void setWebPage_malloc(thermostat_state *thermostat){

	char temp_webPage[6000];
	uint32_t freeHeap;

	if (thermostat->thermParams->controlPanelHTML != NULL) {

		if (SHOW_WEB_PAGE_FUNCTION_LOGS) {
			ESP_LOGI("setWebPage_malloc", "Free alokated memory!");
		}
		free(thermostat->thermParams->controlPanelHTML);
	}

	freeHeap = xPortGetFreeHeapSize();


	thermostat->thermParams->controlPanelHTML_len = sprintf(temp_webPage, "<html>"
			"<head>"
			"    <style>"
			"        body{"
			"            margin-top: 100px;"
			"            margin-left: 0px;"
			"            margin-right: 0px;"
			"            background-color: rgb(0, 217, 255);"
			""
			"        }"
			""
			"        .myHead{"
			"            background-color: black;"
			"            color: white;"
			"            height: 15%%;"
			"            min-height: 100px;"
			"            margin-left: 0px;"
			"            margin-right: 0px;"
			"            display: flex;    "
			"            justify-content: center;     /*horizontal align*/"
			"            align-items: center;        /*vertical align*/"
			"            font-family: sans-serif;"
			"        }"
			""
			"        .Main{"
			"            background-color: rgb(10, 205, 240);"
			"            align-content: center;"
			"            justify-content: center;"
			"            align-items: center;"
			"            display: flex;"
			"            min-height: 150px;"
			"            margin-bottom: 30px;"
			"            border-width: 10%%;"
			"        }"
			""
			"        .inputBox{"
			"            align-content: stretch;"
			"            justify-content: right;"
			"            align-items: center;"
			"            display: flex;"
			"        }"
			""
			"    </style>"
			""
			"</head>"
			"<body>"
			""
			"    <h1 class=\"myHead\">ESP32 Remote Thermostat</h1>"
			""
			"    <div class=\"Main\">"
			"        "
			"        <div>"
			"            <p>Power managment:</p>"
			"            <div>"
			"                <button onclick=\"sendON()\">ON</button> "
			"                <button onclick=\"sendOFF()\">OFF</button> "
			"            </div>"
			"            "
			"        </div>"
			"    </div>"
			""
			"    <div class=\"Main\">"
			"        <div>"
			"            <h1>Thermostat Info</h1>"
			"            <p>Current Main Temperature: %.1f</p>"
			"            <p>Current Fuse Temperature: %.1f</p>"
			"            <p>Current Power Setting: %d</p>"
			"            <p>Free Heap Size: %d</p>"
			"        </div>"
			"    </div>"
			"    "
			"    <div class=\"Main\">"
			"        <div>"
			"            <h1>Thermostat Settings</h1>"
			"            <p>Temperature threshold HIGH:</p>"
			"            <p>Current value: %d</p>"
			"            <div class=\"inputBox\">"
			"                <input id=\"sendHigh_TF\"/>"
			"                <button onclick=\"sendHigh()\">SET</button> "
			"            </div>"
			"            <p>Temperature threshold LOW:</p>"
			"            <p>Current value: %d</p>"
			"            <div class=\"inputBox\">"
			"                <input id=\"sendLow_TF\"/>"
			"                <button onclick=\"sendLow()\">SET</button> "
			"            </div>"
			"            <p>Temperature Fuse threshold HIGH:</p>"
			"            <p>Current value: %d</p>"
			"            <div class=\"inputBox\">"
			"                <input id=\"sendFuseHigh_TF\"/>"
			"                <button onclick=\"sendFuseHigh()\">SET</button> "
			"            </div>"
			"            <p>Temperature Fuse threshold LOW:</p>"
			"            <p>Current value: %d</p>"
			"            <div class=\"inputBox\">"
			"                <input id=\"sendFuseLow_TF\"/>"
			"                <button onclick=\"sendFuseLow()\">SET</button> "
			"            </div>"
			"            <p>Loop Delay Period:</p>"
			"            <p>Current value: %d</p>"
			"            <div class=\"inputBox\">"
			"                <input id=\"sendPeriod_TF\"/>"
			"                <button onclick=\"sendPeriod()\">SET</button> "
			"            </div>"
			"            <p></p>"
			"            "
			"        </div>"
			"    </div>"
			""
			"    <div class=\"Main\">"
			"        <div>"
			"            <div class=\"inputBox\">"
			"                <p>Echo : </p>"
			"                <input id=\"sendEcho_TF\"/>"
			"                <button onclick=\"sendEcho()\">Echo</button> "
			"            </div>"
			"        </div>"
			"    </div>"
			""
			"    "
			""
			"    <script>"
			"        const Http = new XMLHttpRequest();"
			""
			"        const thermostat_ip = %s"
			""
			"        function sendEcho(){"
			"                const url = \"http://\" + thermostat_ip + \"/echo\";"
			"                const message = document.getElementById(\"sendEcho_TF\").value;"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			""
			"        function sendHigh(){"
			"                const url = \"http://\" + thermostat_ip + \"/s/h\";"
			"                const message = document.getElementById(\"sendHigh_TF\").value;"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			""
			"        function sendLow(){"
			"                const url = \"http://\" + thermostat_ip + \"/s/l\";"
			"                const message = document.getElementById(\"sendLow_TF\").value;"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			""
			""
			"        function sendFuseLow(){"
			"                const url = \"http://\" + thermostat_ip + \"/fuse/l\";"
			"                const message = document.getElementById(\"sendFuseLow_TF\").value;"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			""
			"        function sendFuseHigh(){"
			"                const url = \"http://\" + thermostat_ip + \"/fuse/h\";"
			"                const message = document.getElementById(\"sendFuseHigh_TF\").value;"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			""
			"        function sendPeriod(){"
			"                const url = \"http://\" + thermostat_ip + \"/period\";"
			"                const message = document.getElementById(\"sendPeriod_TF\").value;"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			""
			"        "
			"        function sendON(){"
			"                const url = \"http://\" + thermostat_ip + \"/on\";"
			"                const message = \"Turn Thermostat ON\";"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			"        "
			"        function sendOFF(){"
			"                const url = \"http://\" + thermostat_ip + \"/off\";"
			"                const message = \"Turn Thermostat OFF\";"
			"                Http.open(\"POST\", url);"
			"                Http.send(message);"
			"                console.log(url);"
			"            }"
			""
			""
			""
			"        "
			""
			"    </script>"
			""
			"</body>"
			"</html>"

			, thermostat->tempSensor_main->data,
			thermostat->tempSensor_fuse->data,
			(int)thermostat->thermParams->power,
			freeHeap,
			thermostat->thermParams->temp_high,
			thermostat->thermParams->temp_low,
			thermostat->thermParams->temp_fuse_high,
			thermostat->thermParams->temp_fuse_low,
			thermostat->thermParams->period,
			thermostat->thermParams->ip);

	if (SHOW_WEB_PAGE_FUNCTION_LOGS) {
		ESP_LOGI("setWebPage_malloc","Control panel html length: %d",thermostat->thermParams->controlPanelHTML_len);
	}

	thermostat->thermParams->controlPanelHTML = malloc(thermostat->thermParams->controlPanelHTML_len);

	for(uint8_t i = 0; i < thermostat->thermParams->controlPanelHTML_len; i++){
		thermostat->thermParams->controlPanelHTML[i] = temp_webPage[i];
		if (SHOW_WEB_PAGE_FUNCTION_LOGS) {
			putchar(temp_webPage[i]);
		}
	}

}


#endif
