#include "sd_test.h"
#include <Audio.h>
#include "Arduino.h"
#include "SD.h"
#include "FS.h"
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_I2CDevice.h>
#include "ybadge.h"

Audio audio;
void sd_test_init() {
    pinMode(SD_CS, OUTPUT);      
    digitalWrite(SD_CS, HIGH); 
    pinMode(BUTTON1_PIN, INPUT);
    pinMode(BUTTON2_PIN, INPUT);
    pinMode(SWITCH1_PIN, INPUT);
    pinMode(SWITCH2_PIN, INPUT);
    
    // Initialize SPI bus for microSD Card
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // Start Serial Port
    Serial.begin(115200);
    
    // Start microSD Card
    if(!SD.begin(SD_CS))
    {
      Serial.println("Error accessing microSD card!");
      while(true); 
    }
    
    // Setup I2S 
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    // Set Volume
    audio.setVolume(15);
    
    // Open music file
    audio.connecttoFS(SD,"/Never gonna give you up.mp3");
}

void sd_test_loop() {
    int state = 0;
  
  while(switches_get(1) && switches_get(2)) {
    Serial.printf("State: %d\n",state);
    switch(state){
      case 0:
      {
        if(buttons_get(1))
        {
          while(buttons_get(1));
          state = 1;
        }
        break;
      }
      case 1:
      {
        
        if(buttons_get(2))
        {
          while(buttons_get(2));
          state = 2;
        }
      
        
        if(buttons_get(1))
        {
          while(buttons_get(1));
          state = 0;
        }
        
        break;
      }
      case 2:
      {
        if(buttons_get(2))
        {
          while(buttons_get(2));
          state = 3;
        }
        
        else if(buttons_get(1))
        {
          while(buttons_get(1));
          state = 0;
        }
        break;
      }
      case 3:
      {
        if(buttons_get(1))
        {
          while(buttons_get(1));
          state = 4;
        }
        
        else if(buttons_get(2))
        {
          while(buttons_get(2));
          state = 0;
        }
        break;
      }
    
    case 4: {
      int volume = knob_get();
      Serial.printf("Volume: %d\n",volume);
      audio.setVolume(volume);
      audio.setFileLoop(true);
      audio.loop();
      break;
      }
    }
  }
}
