// #pragma once
#include <Arduino.h> // Programming core language and functions

#include "classScreens.h" // custom library with the screen handling

#include "panel/cfgDisplay.hpp" // low level TFT handling and config

// High level for the screen brightness when triggered - 0-100%
#define DEFAULT_BACKLIGHT_HIGH 35
#define DEFAULT_TFT_TIMEOUT_INTERVAL_MS 0 // zero disables timeout
#define TFT_TIMEOUT_INTERVAL_MS_MAX 3600

// defualt level that triggers yellow warning for PM1.0
#define DEFAULT_PM1_0_YELLOW 8
#define DEFAULT_PM1_0_RED 20

#define DEFAULT_PM2_5_YELLOW 10
#define DEFAULT_PM2_5_RED 35

#define DEFAULT_PM10_YELLOW 20
#define DEFAULT_PM10_RED 50

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[TFT_WIDTH * 10];

class classTft
{
public:
    classTft();

    void begin();
    void loop();
    void clear();
    void nextScreen();

    // updates the TFT with correct temp units - C or F
    void updateTempUnits(bool units);

    // updates the warning levels via mqtt
    void updateWarnLevels(uint16_t xPM1_0_YELLOW, uint16_t xPM1_0_RED, uint16_t xPM2_5_YELLOW, uint16_t xPM2_5_RED, uint16_t xPM10_YELLOW, uint16_t xPM10_RED);

    void sendBmeData(uint8_t xiaqError, uint16_t Xco2e, float Xbvoc, float Xhum, float Xtemp); // update the library with new data from sensors
    void sendPmsData(uint16_t xPM1_0, uint16_t xPM2_5, uint16_t xPM10);     // update the library with new data from sensors

    void backLightWake();
    // void setBackLight(int val);

    void setWifiStatus(bool wifiState, bool mqttState);
    void setInfoData(char * xMAC,char * xIP,char * xMQTT);

    uint8_t maxBrightness = DEFAULT_BACKLIGHT_HIGH;
    uint32_t tftTimeoutIntervalMs = DEFAULT_TFT_TIMEOUT_INTERVAL_MS;

private:

    void _setBackLight(int val);

    // current value of the backlight
    int _backLight = DEFAULT_BACKLIGHT_HIGH;

    uint32_t _lastTftTimeoutIntervalMs = 0L;

    #define _BOOT_SCREEN 0
    #define _NORMAL_SCREEN 1
    #define _INFO_SCREEN 2

    uint8_t currentScreen = _BOOT_SCREEN;

    int _backLightHigh = DEFAULT_BACKLIGHT_HIGH;

    // sets the delay timing for the LVGL GUI loop
    uint8_t _lvglSpeed = 10;

    // stores the last time lvgl was run in loop()
    unsigned long _lastLvgl;

    uint8_t _wifiState = -1;

    uint8_t _mqttState = -1;

    bool _tempUnits = 0;

    bool _booted = 0;

    bool _pmsFound = 0;

    uint16_t _co2e = 0;
    float _bvoc = 0.0;
    float _temp = 0.0;
    float _hum = 0.0;

    uint16_t _PM1_0 = 0;
    uint16_t _PM2_5 = 0;
    uint16_t _PM10 = 0;

    uint16_t _setPoint_PM1_0_YELLOW = DEFAULT_PM1_0_YELLOW;
    uint16_t _setPoint_PM1_0_RED = DEFAULT_PM1_0_RED;
    uint16_t _setPoint_PM2_5_YELLOW = DEFAULT_PM2_5_YELLOW;
    uint16_t _setPoint_PM2_5_RED = DEFAULT_PM2_5_RED;
    uint16_t _setPoint_PM10_YELLOW = DEFAULT_PM10_YELLOW;
    uint16_t _setPoint_PM10_RED = DEFAULT_PM10_RED;

    uint8_t _PM1_0Color = 0;
    uint8_t _PM2_5Color = 0;
    uint8_t _PM10Color = 0;

    uint8_t _green = 0;
    uint8_t _yellow = 1;
    uint8_t _red = 2;

    classScreens _screen = classScreens();
};