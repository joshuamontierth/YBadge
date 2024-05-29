#include <Audio.h>
#include "Arduino.h"
#include "SD.h"
#include "FS.h"
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_I2CDevice.h>
#include "ybadge.h"


// microSD Card Reader connections
#define SD_CS         10
#define SPI_MOSI      11 
#define SPI_MISO      13
#define SPI_SCK       12

#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (32)
#define I2S_READ_LEN      (32 * 1024)
#define I2S_CHANNEL_NUM   (1)

const i2s_port_t I2S_PORT = I2S_NUM_1;
File dataFile;
size_t bytes_written = 0;
uint32_t data_chunk_pos = 0;
uint32_t data_chunk_size = 0;
const int headerSize = 44;
int flash_record_size = 0;

//custom Wav header for 32 bit audio
void wavHeader(byte* header, int wavSize) {
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    unsigned int fileSize = wavSize + headerSize - 8;
    header[4] = (byte)(fileSize & 0xFF);
    header[5] = (byte)((fileSize >> 8) & 0xFF);
    header[6] = (byte)((fileSize >> 16) & 0xFF);
    header[7] = (byte)((fileSize >> 24) & 0xFF);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 0x10;
    header[17] = 0x00;
    header[18] = 0x00;
    header[19] = 0x00;
    header[20] = 0x01;
    header[21] = 0x00;
    header[22] = 0x01;
    header[23] = 0x00;
    header[24] = 0x80;
    header[25] = 0x3E;
    header[26] = 0x00;
    header[27] = 0x00;
    header[28] = 0x00;
    header[29] = 0xFA;
    header[30] = 0x00;
    header[31] = 0x00;
    header[32] = 0x04;
    header[33] = 0x00;
    header[34] = 0x20;
    header[35] = 0x00;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (byte)(wavSize & 0xFF);
    header[41] = (byte)((wavSize >> 8) & 0xFF);
    header[42] = (byte)((wavSize >> 16) & 0xFF);
    header[43] = (byte)((wavSize >> 24) & 0xFF);
}

void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len) {
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 2048;
    }
}
//for debugging. Could be used to have students see the data in real time
void display_buf(uint8_t* buf, int length) {
    printf("======\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 4 == 0) {
            printf("\n");
        }
    }
    printf("======\n");
}

void i2s_adc() {
    int i2s_read_len = I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read;

    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    if (i2s_read_buff == NULL || flash_write_buff == NULL) {
        Serial.println("Failed to allocate memory for buffers");
        return;
    }

    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

    Serial.println(" *** Recording Start *** ");
    while (flash_wr_size < flash_record_size) {
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
        display_buf((uint8_t*) i2s_read_buff, 64);

        if (dataFile.write((const byte*) i2s_read_buff, i2s_read_len) != i2s_read_len) {
            Serial.println("Failed to write data to file");
            break;
        }

        flash_wr_size += i2s_read_len;
        ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / flash_record_size);
    }

    dataFile.close();
    Serial.println(" *** Recording End *** ");

    free(i2s_read_buff);
    free(flash_write_buff);
    
}
//start the i2s driver for the mic
void i2sInit() {
  esp_err_t err;
    
    const i2s_config_t i2s_config = {
    
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), // Receive, not transfer
      .sample_rate = I2S_SAMPLE_RATE,                        
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // could only get it to work with 32bits
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // use left channel
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,     // Interrupt level 1
      
      .dma_buf_count = 32,
      .dma_buf_len = 1024,
      .use_apll = 1
  };

    const i2s_pin_config_t pin_config = {
        .bck_io_num = 42,
        .ws_io_num = 41,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = 40
    };

    err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        while (true) {
            Serial.printf("Failed installing driver: %d\n", err);
        }
    }

    REG_SET_BIT(I2S_RX_TIMING_REG(I2S_PORT), BIT(0));
    REG_SET_BIT(I2S_RX_CONF1_REG(I2S_PORT), I2S_RX_MSB_SHIFT);

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed setting pin: %d\n", err);
        while (true);
    }

}
void sd_init(string filename) {
    
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    Serial.begin(115200);

    if (!SD.begin(SD_CS)) {
        while (true) {
            Serial.println("Error accessing microSD card!");
        }
    }
    SD.exists("/recordings") || SD.mkdir("/recordings");
    filename = "/recordings/" + filename + ".wav";
    char* filepath = (char*)filename.c_str();
    dataFile = SD.open(filepath, FILE_WRITE);
    if (dataFile) {
        Serial.println("File opened successfully");
    } else {
        while (true) {
            Serial.println("error opening/creating file");
        }
    }
}


void record_audio(int seconds, string filename) {

    flash_record_size = I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * seconds;
    sd_init(filename);
    byte header[headerSize];
    wavHeader(header, flash_record_size);
    dataFile.write(header, headerSize);

    i2sInit();
    Serial.println("I2S driver installed.");
    i2s_adc();
    Serial.println("Audio recorded.");

}
void setup() {
    record_audio(5, "test");
    
}

void loop() {
}
