
#ifndef WEB_PAGE
#define WEB_PAGE

static const char webPage[]  = "<html>"
"<head>"
    "<style>"
        "body{"
            "margin-top: 100px;"
            "margin-left: 0px;"
            "margin-right: 0px;"
            "background-color: rgb(0, 217, 255);"

        "}"

        ".myHead{"
            "background-color: black;"
            "color: white;"
            "height: 15%;"
            "min-height: 100px;"
            "margin-left: 0px;"
            "margin-right: 0px;"
            "display: flex;"
            "justify-content: center;"
            "align-items: center;     "
            "font-family: sans-serif;"
        "}"

        ".Main{"
        "    background-color: rgb(10, 205, 240);"
        "    align-content: center;"
        "    justify-content: center;"
        "    align-items: center;"
        "    display: flex;"
        "}"

        ".inputBox{"
        "    align-content: stretch;"
        "    justify-content: right;"
        "    align-items: center;"
        "    display: flex;"
        "}"

    "</style>"

"</head>"
"<body>"

    "<h1 class=\"myHead\">ESP32 Remote Thermostat</h1>"
    "<div class=\"Main\">"
        "<p>Set server parameters:</p>"
        "<div id=\"root\"/>"
            "<div class=\"inputBox\">"
                "<p>TempSensor IP : | </p>"
                "<input id=\"b1\"/>"
            "</div>"
            "<div class=\"inputBox\">"
                "<p>GateBox IP : | </p>"
                "<input id=\"b2\"/>"
            "</div>"

        "</div>"
    "</div>"

"</body>"
"</html>";

#endif
