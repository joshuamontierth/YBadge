#include "wifi_test.h"
#include "ybadge.h"

const char* ssid = "";
const char* password =  "";
const char* serverUrl = "";
int knobVal;
void wifi_init() {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        printf("Connecting to WiFi..\n");
    }
    knobVal = knob_get();
    
}

void send_data(String httpRequestData, String endpoint) {
    HTTPClient http;
    String fullUrl = serverUrl + endpoint;
    http.begin(fullUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    httpRequestData = "data=" + String(httpRequestData); 
    int httpResponseCode = http.POST(httpRequestData);
    http.end();
}

void send_button_info() {
    if (knobVal != knob_get()) {
        send_data("Knob at " + String(knob_get()) +"%", "knob");
        knobVal = knob_get();
    }

    bool pressed = false;
    static int buttonState = 0;
    for (int i = 1; i <= 3; i++) {
        
        if (buttons_get(i)) {
            pressed = true;
            send_data("Button " + String(i) + " pressed","button");
            buttonState = i;
            while(buttons_get(i)) {}
            
        }
        
    }
    if (!pressed && buttonState != 0) {
        send_data("Nothing pressed","button");
        buttonState = 0;
    }


    static int switchState = 0;

    if (switches_get(1) && switches_get(2)) {
        if (switchState != 3) {
            send_data("Both switches on", "switch");
            switchState = 3;
        }
        
    }
    else if (switches_get(2)) {
        if (switchState != 2) {
            send_data("Switch 2 on", "switch");
            switchState = 2;
        }
        
    }
    else if (switches_get(1)) {
        if (switchState != 1) {
            send_data("Switch 1 on", "switch");
            switchState = 1;
        }
        
    }

    else {
        if (switchState != 0) {
            send_data("Both switches off", "switch");
            switchState = 0;
        }
        
    }

}

