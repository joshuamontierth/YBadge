#include "hardware_test/hardware_test.h"
#include "light_show/light_show.h"
#include "wifi_sniffer/wifi_sniffer.h"
#include "wifi_test/wifi_test.h"




void setup() {
    wifi_init();
    //wifi_sniffer_init();
    // hardware_test_init();
    //light_show_init();
}

void loop() {
    
    send_button_info();
    //wifi_sniffer_loop();
    // hardware_test_loop();
    //light_show_loop();
}
