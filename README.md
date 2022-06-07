Microcontroller ESP32 in AZDelivery Weather Station HW Setup

Controls up to 7 power switches
2 Humidity Sensor Inputs
1 Servo Output

based on some great libraries
- lvgl based GUI
- OTA update (Async Web Server)
- AutoConnect to bootstrap IP in foreign networks

< more fancy stuff to follow here ... >

Some lib modifications / configs:
src/User_setup.h (TFT_eSPI, set some PINs, also for touch!)
// The hardware SPI can be mapped to any pins
  #define TFT_MISO 19
  #define TFT_MOSI 23
  #define TFT_SCLK 18
  #define TFT_CS    5  // Chip select control pin
  #define TFT_DC    4  // Data Command control pin
  #define TFT_RST  22  // Reset pin (could connect to RST pin)
  //#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
  #define TOUCH_CS 14     // Chip select pin (T_CS) of touch screen

-> and undef defaults

src/lv_conf.h    (symlink into main lvgl dir)
PageBuilder.h   ...<fs::File> _file (~ line 238)
AutoConnect.h    make _CSS_BASE::... 'public:'


