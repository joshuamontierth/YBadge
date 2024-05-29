#ifndef SD_TEST_H
#define SD_TEST_H


// microSD Card Reader connections
#define SD_CS         10
#define SPI_MOSI      11 
#define SPI_MISO      13
#define SPI_SCK       12
 
// I2S Connections
#define I2S_DOUT      14
#define I2S_BCLK      21
#define I2S_LRC       47
 
// Create Audio object


void sd_test_init();
void sd_test_loop();

#endif /* SD_TEST_H */
