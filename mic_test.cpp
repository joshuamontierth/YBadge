#include "sd_test.h"
#include <Audio.h>
#include "Arduino.h"
#include "SD.h"
#include "FS.h"
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_I2CDevice.h>
#include "ybadge.h"

#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS 1
#define BUFFER_SIZE 1024

const i2s_port_t I2S_PORT = I2S_NUM_1;

File wavFile;
size_t bytes_written = 0;
uint32_t data_chunk_pos = 0;
uint32_t data_chunk_size = 0;

void setup() {
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
      while(true) { 
      }
    }
    
  
  File dataFile = SD.open("/example.txt", FILE_WRITE);
  // if the file opened okay, write to it:
  if (dataFile) {
    Serial.println("Writing to example.txt...");
    dataFile.println("This is a test file.");
    dataFile.println("Writing data to the SD card is easy!");
    
    dataFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    
    Serial.println("error opening example.txt");
    while(true);
  }
  esp_err_t err;

  // The I2S config as per the example
  const i2s_config_t i2s_config = {
    
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), // Receive, not transfer
      .sample_rate = 40000,                         // 16KHz
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // could only get it to work with 32bits
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // use right channel
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,     // Interrupt level 1
      .dma_buf_count = 4,                           // number of buffers
      .dma_buf_len = 8                              // 8 samples per buffer (minimum)
  };

  // The pin config as per the setup
  const i2s_pin_config_t pin_config = {
      .bck_io_num = 42,   // Serial Clock (SCK)
      .ws_io_num = 41,    // Word Select (WS)
      .data_out_num = I2S_PIN_NO_CHANGE, // not used (only for speakers)
      .data_in_num = 40   // Serial Data (SD)
  };

  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing driver: %d\n", err);
    while (true);
  }
  REG_SET_BIT(I2S_RX_TIMING_REG(I2S_PORT), BIT(0));
  // I2S_RX_TIMING_REG
  
// Force Philips mode
REG_SET_BIT(I2S_RX_CONF1_REG(I2S_PORT), I2S_RX_MSB_SHIFT);
// REG_CLR_BIT(I2S_RX_CONF1_REG(I2S_PORT), I2S_RX_MSB_SHIFT);
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Failed setting pin: %d\n", err);
    while (true);
  }
  Serial.println("I2S driver installed.");
}

uint32_t sBuffer[8*4*8];
void loop() {
 // False print statements to "lock range" on serial plotter display
  // Change rangelimit value to adjust "sensitivity"
  int rangelimit = -10000;
  Serial.print(-12000);
  Serial.print(" ");
  Serial.print(-4000);
  Serial.print(" ");

  // Read a single sample and log it for the Serial Plotter.
  size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, sBuffer, sizeof(sBuffer), &bytesIn, /*portMAX_DELAY*/ 10); // no timeout
    if (result == ESP_OK && bytesIn > 0)
    {
      int16_t value = (int16_t)((sBuffer[0]>>14) | ((sBuffer[0] & 0x020000)>>2));
      // Serial.printf("%d %X %X\n",value, sBuffer[0],(sBuffer[0] & 0x020000)>>2);
      // Serial.printf("%x %x %x\n",sBuffer[0],sBuffer[0]>>8,sBuffer[0]>>14);
      Serial.println(value);
   }
}


void writeWavHeader(File &file, int sampleRate, int numChannels, int bitsPerSample) {
    const uint8_t riff[] = "RIFF";
    file.write(riff, sizeof(riff) - 1);
    file.write((uint8_t *)"----", 4);  // Placeholder for file size
    const uint8_t wave[] = "WAVE";
    file.write(wave, sizeof(wave) - 1);

 
    const uint8_t fmt[] = "fmt ";
    file.write(fmt, sizeof(fmt) - 1);
    file.write((uint8_t[]){16, 0, 0, 0}, 4);  // Subchunk1Size (16 for PCM)
    file.write((uint8_t[]){1, 0}, 2);         // AudioFormat (1 for PCM)
    file.write((uint8_t *)&numChannels, 2);   // NumChannels
    file.write((uint8_t *)&sampleRate, 4);    // SampleRate
    int byteRate = sampleRate * numChannels * bitsPerSample / 8;
    file.write((uint8_t *)&byteRate, 4);      // ByteRate
    int blockAlign = numChannels * bitsPerSample / 8;
    file.write((uint8_t *)&blockAlign, 2);    // BlockAlign
    file.write((uint8_t *)&bitsPerSample, 2); // BitsPerSample

    // Data chunk header
    const uint8_t data[] = "data";
    file.write(data, sizeof(data) - 1);
    data_chunk_pos = file.position();
    file.write((uint8_t *)"----", 4);  // Placeholder for data chunk size
}

void recordAudio(int duration) {
    uint8_t sBuffer[BUFFER_SIZE];
    size_t bytesIn = 0;

    uint32_t startTime = millis();
    while ((millis() - startTime) < (duration * 1000)) {
        esp_err_t result = i2s_read(I2S_PORT, sBuffer, sizeof(sBuffer), &bytesIn, portMAX_DELAY);
        if (result == ESP_OK && bytesIn > 0) {
            wavFile.write(sBuffer, bytesIn);
            data_chunk_size += bytesIn;
        }
    }
}