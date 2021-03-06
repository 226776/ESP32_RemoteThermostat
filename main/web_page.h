
#ifndef WEB_PAGE
#define WEB_PAGE

static const char webPage[]  = "<html>"
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
		"            height: 15%;"
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
		"            <p>Power:</p>"
		"            <div>"
		"                <button onclick=\"sendON()\">ON</button> "
		"                <button onclick=\"sendOFF()\">OFF</button> "
		"            </div>"
		"            "
		"        </div>"
		"    </div>"
		"    "
		"    <div class=\"Main\">"
		"        <div>"
		"            <p>Temperature threshold HIGH:</p>"
		"            <div class=\"inputBox\">"
		"                <input id=\"tb2\"/>"
		"                <button onclick=\"sendHigh()\">SET</button> "
		"            </div>"
		"            <p>Temperature threshold LOW:</p>"
		"            <div class=\"inputBox\">"
		"                <input id=\"tb3\"/>"
		"                <button onclick=\"sendLow()\">SET</button> "
		"            </div>"
		"            "
		"        </div>"
		"    </div>"
		""
		"    <div class=\"Main\">"
		"        <div>"
		"            <div class=\"inputBox\">"
		"                <p>Echo : </p>"
		"                <input id=\"tb1\"/>"
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
		"        function sendEcho(){"
		"                const url = \"http://192.168.1.129/echo\";"
		"                const message = document.getElementById(\"tb1\").value;"
		"                Http.open(\"POST\", url);"
		"                Http.send(message);"
		"                console.log(url);"
		"            }"
		""
		"        function sendHigh(){"
		"                const url = \"http://192.168.1.129/s/h\";"
		"                const message = document.getElementById(\"tb2\").value;"
		"                Http.open(\"POST\", url);"
		"                Http.send(message);"
		"                console.log(url);"
		"            }"
		""
		"        function sendLow(){"
		"                const url = \"http://192.168.1.129/s/l\";"
		"                const message = document.getElementById(\"tb3\").value;"
		"                Http.open(\"POST\", url);"
		"                Http.send(message);"
		"                console.log(url);"
		"            }"
		""
		"        function sendON(){"
		"                const url = \"http://192.168.1.129/on\";"
		"                const message = \"Turn Thermostat ON\";"
		"                Http.open(\"POST\", url);"
		"                Http.send(message);"
		"                console.log(url);"
		"            }"
		""
		"        function sendOFF(){"
		"                const url = \"http://192.168.1.129/off\";"
		"                const message = \"Turn Thermostat OFF\";"
		"                Http.open(\"POST\", url);"
		"                Http.send(message);"
		"                console.log(url);"
		"            }"
		""
		"    </script>"
		""
		"</body>"
		"</html>";


#endif
