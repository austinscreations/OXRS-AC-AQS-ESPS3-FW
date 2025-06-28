#include <classTft.h>

classTft::classTft() {};

/*
    lcd interface
    transfer pixel data range to lcd
// */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    int w = (area->x2 - area->x1 + 1);
    int h = (area->y2 - area->y1 + 1);

    tft.startWrite();                            /* Start new TFT transaction */
    tft.setAddrWindow(area->x1, area->y1, w, h); /* set the working window */

    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);

    tft.endWrite();            /* terminate TFT transaction */
    lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

void classTft::begin()
{
    Serial.println(F("[TFT] Starting LVGL and TFT"));

    // start lvgl
    lv_init();
    lv_img_cache_set_size(10);

    // startup TFT driver
    tft.init();

    // fill the screen black
    tft.fillScreen(TFT_BLACK);

    // screen brightness off
    _setBackLight(0);

    // initialise draw buffer
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * 10);

    // initialise the display
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    // settings for display driver
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    _screen.begin();

    // show splash screen
    lv_scr_load(_screen.bootScreen);
    lv_refr_now(NULL);

    _setBackLight(35);

    delay(1000);
}

// keeps the screen running and updates as needed
void classTft::loop()
{
    if ((millis() - _lastLvgl) > _lvglSpeed)
    {
        lv_timer_handler(); /* let the GUI do its work */
        _lastLvgl = millis();
    }

    // first time loading since boot and we have full network so lets stop showing splash screen
    if (_wifiState == true && _mqttState == true && _booted == false)
    {
        _setBackLight(maxBrightness);
        _lastTftTimeoutIntervalMs = millis();
        // reset the brightness timer
        lv_scr_load(_screen.normalScreen);
        currentScreen = _NORMAL_SCREEN;
        _booted = true;
    }

    if (_booted)
    {
        // check brightness and change brightness based on timer
        if (tftTimeoutIntervalMs != 0 && millis() - _lastTftTimeoutIntervalMs >= tftTimeoutIntervalMs)
        {
            _setBackLight(0);
        }
    }

    if (_booted && _pmsFound) // we are booted lets do stuff based on data
    {
        // check PM1.0 values
        if (_PM1_0 >= _setPoint_PM1_0_RED)
        {
            _PM1_0Color = _red;
            lv_obj_set_style_text_color(_screen.pm1_0Text, lv_color_make(255, 0, 0), 0);
        }
        else if (_PM1_0 > _setPoint_PM1_0_YELLOW && _PM1_0 < _setPoint_PM1_0_RED)
        {
            _PM1_0Color = _yellow;
            lv_obj_set_style_text_color(_screen.pm1_0Text, lv_color_make(255, 200, 0), 0);
        }
        else
        {
            _PM1_0Color = _green;
            lv_obj_set_style_text_color(_screen.pm1_0Text, lv_color_make(23, 111, 192), 0);
        }

        // check PM2.5 values
        if (_PM2_5 >= _setPoint_PM2_5_RED)
        {
            _PM2_5Color = _red;
            lv_obj_set_style_text_color(_screen.pm2_5Text, lv_color_make(255, 0, 0), 0);
        }
        else if (_PM2_5 > _setPoint_PM2_5_YELLOW && _PM2_5 < _setPoint_PM2_5_RED)
        {
            _PM2_5Color = _yellow;
            lv_obj_set_style_text_color(_screen.pm2_5Text, lv_color_make(255, 200, 0), 0);
        }
        else
        {
            _PM2_5Color = _green;
            lv_obj_set_style_text_color(_screen.pm2_5Text, lv_color_make(23, 111, 192), 0);
        }

        // Check PM10 values
        if (_PM10 >= _setPoint_PM10_RED)
        {
            _PM10Color = _red;
            lv_obj_set_style_text_color(_screen.pm10Text, lv_color_make(255, 0, 0), 0);
        }
        else if (_PM10 > _setPoint_PM10_YELLOW && _PM10 < _setPoint_PM10_RED)
        {
            _PM10Color = _yellow;
            lv_obj_set_style_text_color(_screen.pm10Text, lv_color_make(255, 200, 0), 0);
        }
        else
        {
            _PM10Color = _green;
            lv_obj_set_style_text_color(_screen.pm10Text, lv_color_make(23, 111, 192), 0);
        }
    }

    // change the border
    // go red
    if (_PM1_0Color == _red || _PM2_5Color == _red || _PM10Color == _red)
    {
        lv_obj_set_style_shadow_color(_screen.border, lv_color_make(255, 0, 0), 0);
        // screen brightness high
    }
    // go orange
    else if (_PM1_0Color != _green && _PM1_0Color != _red || _PM2_5Color != _green && _PM2_5Color != _red || _PM10Color != _green && _PM10Color != _red)
    {
        lv_obj_set_style_shadow_color(_screen.border, lv_color_make(255, 200, 0), 0);
        // allow partial dimming
    }
    // go green
    else
    {
        lv_obj_set_style_shadow_color(_screen.border, lv_color_make(0, 255, 0), 0);
        // allow full dimming
    }
}

// keeps the screen running and updates as needed
void classTft::nextScreen()
{
    // make sure we are actually booted before responding to the button
    if (_booted)
    {
        if (_backLight != maxBrightness)
        {
            _setBackLight(maxBrightness);
            _backLight = maxBrightness;
        }
        else
        {
            if (currentScreen == _NORMAL_SCREEN)
            {
                lv_scr_load(_screen.infoScreen);
                lv_refr_now(NULL);
                currentScreen = _INFO_SCREEN;
            }
            else
            {
                lv_scr_load(_screen.normalScreen);
                lv_refr_now(NULL);
                currentScreen = _NORMAL_SCREEN;
            }
        }
    }
    _lastTftTimeoutIntervalMs = millis();
}

void classTft::setInfoData(char * xMAC,char * xIP,char * xMQTT)
{
    // keep the infoScreen up to date
    _screen.updateInfoScreen(xMAC,xIP,xMQTT);
}

void classTft::sendBmeData(uint8_t xiaqError, uint16_t Xco2e, float Xbvoc, float Xhum, float Xtemp)
{
    _co2e = Xco2e;
    _bvoc = Xbvoc;
    _hum = Xhum;
    _temp = Xtemp;

    // sensor values are no good clear data
    if (xiaqError == 0)
    {
        lv_label_set_text_fmt(_screen.co2eText, "");
        lv_obj_align_to(_screen.co2eText, _screen.text2, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
        lv_label_set_text_fmt(_screen.bvocText, "");
        lv_obj_align_to(_screen.bvocText, _screen.text3, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    }
    // decent values show data
    else
    {
        lv_label_set_text_fmt(_screen.co2eText, "%d", _co2e);
        lv_obj_align_to(_screen.co2eText, _screen.text2, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
        lv_label_set_text_fmt(_screen.bvocText, "%3.1f", _bvoc);
        lv_obj_align_to(_screen.bvocText, _screen.text3, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    }

    // very good data green icon
    if (xiaqError >= 3)
    {
        lv_obj_set_style_img_recolor(_screen.warnIcon, lv_color_make(0, 255, 0), 0);
    }
    // decent data - go orange
    else if (xiaqError == 2)
    {
        lv_obj_set_style_img_recolor(_screen.warnIcon, lv_color_make(255, 230, 0), 0);
    }
    // bad data go red
    else
    {
        lv_obj_set_style_img_recolor(_screen.warnIcon, lv_color_make(255, 0, 0), 0);
    }

    lv_label_set_text_fmt(_screen.tempText, "%3.1f", _temp);
    lv_obj_align_to(_screen.labelUnits, _screen.tempText, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);
    lv_label_set_text_fmt(_screen.humText, "%3.1f", _hum);
    lv_obj_align_to(_screen.humPercent, _screen.humText, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);
}

void classTft::sendPmsData(uint16_t xPM1_0, uint16_t xPM2_5, uint16_t xPM10)
{
    _PM1_0 = xPM1_0;
    _PM2_5 = xPM2_5;
    _PM10 = xPM10;

    _pmsFound = true;

    lv_label_set_text_fmt(_screen.pm1_0Text, "%d", _PM1_0);
    lv_obj_align_to(_screen.pm1_0Text, _screen.text4, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_label_set_text_fmt(_screen.pm2_5Text, "%d", _PM2_5);
    lv_obj_align_to(_screen.pm2_5Text, _screen.text5, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_label_set_text_fmt(_screen.pm10Text, "%d", _PM10);
    lv_obj_align_to(_screen.pm10Text, _screen.text6, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

void classTft::updateWarnLevels(uint16_t xPM1_0_YELLOW, uint16_t xPM1_0_RED, uint16_t xPM2_5_YELLOW, uint16_t xPM2_5_RED, uint16_t xPM10_YELLOW, uint16_t xPM10_RED)
{
    _setPoint_PM1_0_YELLOW = xPM1_0_YELLOW;
    _setPoint_PM1_0_RED = xPM1_0_RED;
    _setPoint_PM2_5_YELLOW = xPM2_5_YELLOW;
    _setPoint_PM2_5_RED = xPM2_5_RED;
    _setPoint_PM10_YELLOW = xPM10_YELLOW;
    _setPoint_PM10_RED = xPM10_RED;

    if (_setPoint_PM1_0_YELLOW > 500)
        _setPoint_PM1_0_YELLOW = 500;
    if (_setPoint_PM1_0_YELLOW < 5)
        _setPoint_PM1_0_YELLOW = 5;

    if (_setPoint_PM1_0_RED > 500)
        _setPoint_PM1_0_RED = 500;
    if (_setPoint_PM1_0_RED < 5)
        _setPoint_PM1_0_RED = 5;

    if (_setPoint_PM2_5_YELLOW > 500)
        _setPoint_PM2_5_YELLOW = 500;
    if (_setPoint_PM2_5_YELLOW < 5)
        _setPoint_PM2_5_YELLOW = 5;

    if (_setPoint_PM2_5_RED > 500)
        _setPoint_PM2_5_RED = 500;
    if (_setPoint_PM2_5_RED < 5)
        _setPoint_PM2_5_RED = 5;

    if (_setPoint_PM1_0_YELLOW > 500)
        _setPoint_PM1_0_YELLOW = 500;
    if (_setPoint_PM1_0_YELLOW < 5)
        _setPoint_PM1_0_YELLOW = 5;

    if (_setPoint_PM10_RED > 500)
        _setPoint_PM10_RED = 500;
    if (_setPoint_PM10_RED < 5)
        _setPoint_PM10_RED = 5;
}

void classTft::updateTempUnits(bool units)
{
    _tempUnits = units;
    if (units == 1)
    {
        lv_label_set_text_fmt(_screen.labelUnits, "°F");
    }
    else
    {
        lv_label_set_text_fmt(_screen.labelUnits, "°C");
    }
}

void classTft::clear()
{
    _screen.clear();
}

void classTft::backLightWake()
{
    _setBackLight(maxBrightness);
    _backLight = maxBrightness;
    _lastTftTimeoutIntervalMs = millis();
}

void classTft::_setBackLight(int val)
{
    if (val > 100)
        val = 100;
    if (val < 0)
        val = 0;

    tft.setBrightness(255 * val / 100);
    _backLight = val;
}

void classTft::setWifiStatus(bool wifiState, bool mqttState)
{
    if (wifiState == 1 && mqttState == 1)
    {
        lv_obj_set_style_img_recolor(_screen.wifiIcon1, lv_color_make(0, 255, 0), 0); // full network     - go green
        lv_obj_set_style_img_recolor(_screen.wifiIcon2, lv_color_make(0, 255, 0), 0); // full network     - go green
        lv_obj_set_style_img_recolor(_screen.wifiIcon3, lv_color_make(0, 255, 0), 0); // full network     - go green
    }
    else if (wifiState == 1 && mqttState == 0)
    {
        lv_obj_set_style_img_recolor(_screen.wifiIcon1, lv_color_make(255, 230, 0), 0); // need mqtt        - go orange
        lv_obj_set_style_img_recolor(_screen.wifiIcon2, lv_color_make(255, 230, 0), 0); // need mqtt        - go orange
        lv_obj_set_style_img_recolor(_screen.wifiIcon3, lv_color_make(255, 230, 0), 0); // need mqtt        - go orange
    }
    else
    {
        lv_obj_set_style_img_recolor(_screen.wifiIcon1, lv_color_make(255, 0, 0), 0); // no network       - go red
        lv_obj_set_style_img_recolor(_screen.wifiIcon2, lv_color_make(255, 0, 0), 0); // no network       - go red
        lv_obj_set_style_img_recolor(_screen.wifiIcon3, lv_color_make(255, 0, 0), 0); // no network       - go red
    }
    lv_refr_now(NULL);

    _wifiState = wifiState;
    _mqttState = mqttState;
}