// #pragma once
#include <lvgl.h>
#include <Arduino.h>   // Programming core language and functions

// load external images  icons
extern "C" const lv_img_dsc_t superhouse;
extern "C" const lv_img_dsc_t icons8_wifi_30;
extern "C" const lv_img_dsc_t number_question_50;
extern "C" const lv_img_dsc_t icons8_temperature_50;

// extra text
extern "C" const lv_font_t number_OR_50;

class classScreens
{
    public:
    classScreens();

    void begin();
    void clear();
    void updateInfoScreen(char * xMAC,char * xIP,char * xMQTT);

    lv_obj_t * bootScreen;
    lv_obj_t * normalScreen;
    lv_obj_t * infoScreen;

    // LVGL object for the border
    lv_obj_t * border;

    // LVGL object for the WiFi icon
    lv_obj_t * wifiIcon1;
    lv_obj_t * wifiIcon2;
    lv_obj_t * wifiIcon3;

    // LVGL object for the temperature Text
    lv_obj_t * tempText;

    // LVGL object for the temperature units
    lv_obj_t * labelUnits;

    // LVGL object for the humidity Text
    lv_obj_t * humText;

    // LVGL object for the humidity % sign
    lv_obj_t * humPercent;

    // LVGL object for the iaqText
    lv_obj_t * iaqText;
    // LVGL object for the humidity icon
    lv_obj_t * warnIcon;

    // LVGL object for the co2e Text
    lv_obj_t * text2;
    lv_obj_t * co2eText;

    // LVGL object for the bvoc Text
    lv_obj_t * text3;
    lv_obj_t * bvocText;

    // LVGL object for the pm1.0 Text
    lv_obj_t * text4;
    lv_obj_t * pm1_0Text;

    // LVGL object for the pm2.5 Text
    lv_obj_t * text5;
    lv_obj_t * pm2_5Text;

    // LVGL object for the pm10 Text
    lv_obj_t * text6;
    lv_obj_t * pm10Text;

    private:

    void _bootScreen(void);
    void _normalScreen(void);
    void _infoScreen(void);

    lv_obj_t *_infoTextArea;

    // LVGL object for the temperature icon
    lv_obj_t * _tempIcon;

    // LVGL object for the humidity icon
    lv_obj_t * _humIcon;

    // image pointer for further reference
    const void *_imgSuperhouse = &superhouse;
    const void *_imgWifi = &icons8_wifi_30;
    const void *_imgWarn = &number_question_50;
    const void *_imgTemp = &icons8_temperature_50;

};