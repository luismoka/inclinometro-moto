// Configuracion de TFT_eSPI para la Waveshare ESP32-S3-LCD-1.69 (ST7789V2, 240x280)
// Copiar a la carpeta User_Setups de la libreria y seleccionarlo en User_Setup_Select.h
#define USER_SETUP_ID 469
#define USE_HSPI_PORT
#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 280
#define CGRAM_OFFSET
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_ON
#define TFT_BACKLIGHT_ON 1

#define TFT_BL   15
#define TFT_MISO -1
#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  8

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
