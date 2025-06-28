#include <classScreens.h>

// Macro for converting env vars to strings
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

classScreens::classScreens() {};

void classScreens::begin()
{
    bootScreen = lv_obj_create(NULL);
    normalScreen = lv_obj_create(NULL);
    infoScreen = lv_obj_create(NULL);

    _bootScreen();
    _normalScreen();
    _infoScreen();
}

// clears everything off the current screen
void classScreens::clear()
{
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);
    lv_img_cache_invalidate_src(NULL);
    delay(10);
}

// show SuperHouse Logo on screen
void classScreens::_bootScreen(void)
{
    lv_obj_set_style_bg_color(bootScreen, lv_color_make(0, 0, 0), LV_PART_MAIN);

    // change the border to superhouse teal green as we aren't show status info
    lv_obj_t *border1 = lv_obj_create(bootScreen);
    lv_obj_align(border1, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_size(border1, 200, 200);
    lv_obj_set_style_bg_color(border1, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_border_color(border1, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_shadow_width(border1, 40, 0);
    lv_obj_set_style_shadow_spread(border1, 20, 0);
    lv_obj_set_style_shadow_color(border1, lv_color_make(0, 164, 180), 0);

    // show Super screen
    lv_obj_t *img1 = lv_img_create(bootScreen);
    lv_img_set_src(img1, _imgSuperhouse);
    lv_obj_align(img1, LV_ALIGN_TOP_LEFT, 24, 24);

    wifiIcon1 = lv_img_create(bootScreen);
    lv_img_set_src(wifiIcon1, _imgWifi);
    lv_obj_set_style_img_recolor_opa(wifiIcon1, 255, 0);
    lv_obj_set_style_img_recolor(wifiIcon1, lv_color_make(255, 0, 0), 0);
    lv_obj_align(wifiIcon1, LV_ALIGN_TOP_RIGHT, -23, 20);

    lv_obj_t *text1 = lv_label_create(bootScreen);
    lv_label_set_text(text1, "SuperHouse");
    lv_obj_align(text1, LV_ALIGN_TOP_LEFT, 25, 140);
    lv_obj_set_style_text_font(text1, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(text1, lv_color_make(23, 111, 192), 0);

    lv_obj_t *text2 = lv_label_create(bootScreen);
    lv_label_set_text(text2, "AQS");
    lv_obj_align(text2, LV_ALIGN_TOP_LEFT, 25, 170);
    lv_obj_set_style_text_font(text2, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(text2, lv_color_make(23, 111, 192), 0);
}

// everything is normal
void classScreens::_normalScreen(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(0, 0, 0), LV_PART_MAIN);

    // set border
    border = lv_obj_create(normalScreen);
    lv_obj_align(border, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_size(border, 200, 200);
    lv_obj_set_style_bg_color(border, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_border_color(border, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_shadow_width(border, 40, 0);
    lv_obj_set_style_shadow_spread(border, 20, 0);
    lv_obj_set_style_shadow_color(border, lv_color_make(23, 111, 192), 0);

    wifiIcon2 = lv_img_create(normalScreen);
    lv_img_set_src(wifiIcon2, _imgWifi);
    lv_obj_set_style_img_recolor_opa(wifiIcon2, 255, 0);
    lv_obj_set_style_img_recolor(wifiIcon2, lv_color_make(100, 100, 140), 0);
    lv_obj_align(wifiIcon2, LV_ALIGN_TOP_RIGHT, -23, 20);

    // set Temp Icon
    _tempIcon = lv_img_create(normalScreen);
    lv_img_set_src(_tempIcon, _imgTemp);
    lv_obj_align(_tempIcon, LV_ALIGN_TOP_LEFT, 15, 40);
    lv_obj_set_style_img_recolor_opa(_tempIcon, 255, 0);
    lv_obj_set_style_img_recolor(_tempIcon, lv_color_make(100, 100, 140), 0);

    // set Temp Text
    tempText = lv_label_create(normalScreen);
    lv_label_set_text(tempText, "");
    lv_obj_align(tempText, LV_ALIGN_TOP_LEFT, 60, 25);
    lv_obj_set_style_text_font(tempText, &number_OR_50, 0);
    lv_obj_set_style_text_color(tempText, lv_color_make(23, 111, 192), 0);

    labelUnits = lv_label_create(normalScreen);
    lv_obj_set_size(labelUnits, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(labelUnits, lv_color_make(100, 100, 140), 0);
    lv_obj_set_style_text_font(labelUnits, &lv_font_montserrat_20, 0);
    lv_label_set_text(labelUnits, "°C");
    lv_obj_align_to(labelUnits, tempText, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);

    // set Temp Text
    humText = lv_label_create(normalScreen);
    lv_label_set_text(humText, "");
    lv_obj_align(humText, LV_ALIGN_TOP_LEFT, 60, 70);
    lv_obj_set_style_text_font(humText, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(humText, lv_color_make(23, 111, 192), 0);

    humPercent = lv_label_create(normalScreen);
    lv_obj_set_size(humPercent, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(humPercent, lv_color_make(100, 100, 140), 0);
    lv_obj_set_style_text_font(humPercent, &lv_font_montserrat_20, 0);
    lv_label_set_text(humPercent, "%");
    lv_obj_align_to(humPercent, humText, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);

    // create border around the BME values
    lv_obj_t *obj1;
    obj1 = lv_obj_create(normalScreen);
    lv_obj_align(obj1, LV_ALIGN_TOP_LEFT, 20, 105);
    lv_obj_set_size(obj1, 200, 54);
    lv_obj_set_style_bg_opa(obj1, 0, 0);
    lv_obj_set_style_border_color(obj1, lv_color_make(0, 164, 180), 0);
    lv_obj_set_style_border_width(obj1, 3, 0);

    warnIcon = lv_img_create(normalScreen);
    lv_img_set_src(warnIcon, _imgWarn);
    lv_obj_align_to(warnIcon, obj1, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_zoom(warnIcon, 205); // full size is 256
    lv_obj_set_style_img_recolor_opa(warnIcon, 255, 0);
    lv_obj_set_style_img_recolor(wifiIcon2, lv_color_make(100, 100, 140), 0);

    text2 = lv_label_create(normalScreen);
    lv_label_set_text(text2, "CO2e");
    lv_obj_align(text2, LV_ALIGN_TOP_MID, -70, 110);
    lv_obj_set_style_text_font(text2, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(text2, lv_color_make(100, 100, 140), 0);

    co2eText = lv_label_create(normalScreen);
    lv_obj_set_size(co2eText, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(co2eText, lv_color_make(23, 111, 192), 0);
    lv_obj_set_style_text_font(co2eText, &lv_font_montserrat_16, 0);
    lv_label_set_text(co2eText, "");
    lv_obj_align_to(co2eText, text2, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    text3 = lv_label_create(normalScreen);
    lv_label_set_text(text3, "bVOC");
    lv_obj_align(text3, LV_ALIGN_TOP_MID, 70, 110);
    lv_obj_set_style_text_font(text3, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(text3, lv_color_make(100, 100, 140), 0);

    bvocText = lv_label_create(normalScreen);
    lv_obj_set_size(bvocText, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(bvocText, lv_color_make(23, 111, 192), 0);
    lv_obj_set_style_text_font(bvocText, &lv_font_montserrat_16, 0);
    lv_label_set_text(bvocText, "");
    lv_obj_align_to(bvocText, text3, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // create border around the PMS values
    lv_obj_t *obj2;
    obj2 = lv_obj_create(normalScreen);
    lv_obj_align(obj2, LV_ALIGN_TOP_LEFT, 20, 166);
    lv_obj_set_size(obj2, 200, 54);
    lv_obj_set_style_bg_opa(obj2, 0, 0);
    lv_obj_set_style_border_color(obj2, lv_color_make(0, 164, 180), 0);
    lv_obj_set_style_border_width(obj2, 3, 0);

    text4 = lv_label_create(normalScreen);
    lv_label_set_text(text4, "PM1.0");
    lv_obj_align(text4, LV_ALIGN_TOP_MID, -70, 171);
    lv_obj_set_style_text_font(text4, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(text4, lv_color_make(100, 100, 140), 0);

    pm1_0Text = lv_label_create(normalScreen);
    lv_obj_set_size(pm1_0Text, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(pm1_0Text, lv_color_make(23, 111, 192), 0);
    lv_obj_set_style_text_font(pm1_0Text, &lv_font_montserrat_16, 0);
    lv_label_set_text(pm1_0Text, "");
    lv_obj_align_to(pm1_0Text, text4, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    text5 = lv_label_create(normalScreen);
    lv_label_set_text(text5, "PM2.5");
    lv_obj_align(text5, LV_ALIGN_TOP_MID, 0, 171);
    lv_obj_set_style_text_font(text5, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(text5, lv_color_make(100, 100, 140), 0);

    pm2_5Text = lv_label_create(normalScreen);
    lv_obj_set_size(pm2_5Text, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(pm2_5Text, lv_color_make(23, 111, 192), 0);
    lv_obj_set_style_text_font(pm2_5Text, &lv_font_montserrat_16, 0);
    lv_label_set_text(pm2_5Text, "");
    lv_obj_align_to(pm2_5Text, text5, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    text6 = lv_label_create(normalScreen);
    lv_label_set_text(text6, "PM10");
    lv_obj_align(text6, LV_ALIGN_TOP_MID, 70, 171);
    lv_obj_set_style_text_font(text6, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(text6, lv_color_make(100, 100, 140), 0);

    pm10Text = lv_label_create(normalScreen);
    lv_obj_set_size(pm10Text, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(pm10Text, lv_color_make(23, 111, 192), 0);
    lv_obj_set_style_text_font(pm10Text, &lv_font_montserrat_16, 0);
    lv_label_set_text(pm10Text, "");
    lv_obj_align_to(pm10Text, text6, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

// show system config menu
void classScreens::_infoScreen(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(0, 0, 0), LV_PART_MAIN);

    // change the border to superhouse teal green as we aren't show status info
    lv_obj_t *border2 = lv_obj_create(infoScreen);
    lv_obj_align(border2, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_size(border2, 200, 200);
    lv_obj_set_style_bg_color(border2, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_border_color(border2, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_shadow_width(border2, 40, 0);
    lv_obj_set_style_shadow_spread(border2, 20, 0);
    lv_obj_set_style_shadow_color(border2, lv_color_make(0, 164, 180), 0);

    _infoTextArea = lv_table_create(infoScreen);
    lv_obj_set_size(_infoTextArea, 200, 200);
    lv_obj_align(_infoTextArea, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_radius(_infoTextArea, 5, 0);
    lv_obj_set_style_bg_color(_infoTextArea, lv_color_make(240, 240, 240),0);
    lv_obj_set_style_border_color(_infoTextArea, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_bg_opa(_infoTextArea, 0, 0);
    lv_obj_set_style_border_width(_infoTextArea, 0, 0);
    lv_obj_set_style_pad_all(_infoTextArea, 2, 0);
    lv_obj_set_style_pad_left(_infoTextArea, 4, 0);
    lv_obj_set_style_pad_top(_infoTextArea, 10, 0);
    lv_obj_set_style_border_width(_infoTextArea, 0, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(_infoTextArea, 2, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(_infoTextArea, 0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(_infoTextArea, lv_color_make(23, 111, 192), LV_PART_ITEMS);
    lv_obj_set_style_text_font(_infoTextArea, &lv_font_montserrat_12, LV_PART_ITEMS);

    lv_table_set_col_width(_infoTextArea, 0, 52);
    lv_table_set_col_width(_infoTextArea, 1, 200 - 52 - 10);

    // show a wifi icon at the top right of screen. colored to match current state
    wifiIcon3 = lv_img_create(infoScreen);
    lv_img_set_src(wifiIcon3, _imgWifi);
    lv_obj_align(wifiIcon3, LV_ALIGN_TOP_RIGHT, -23, 20);
    lv_obj_set_style_img_recolor_opa(wifiIcon3, 255, 0);
    lv_obj_set_style_img_recolor(wifiIcon3, lv_color_make(100, 100, 140), 0);

}

void classScreens::updateInfoScreen(char * xMAC,char * xIP,char * xMQTT)
{
    lv_obj_t *table = _infoTextArea;

    lv_table_set_cell_value(table, 0, 0, "Name:");
    lv_table_set_cell_value(table, 0, 1, FW_NAME);
    lv_table_set_cell_value(table, 1, 0, "Maker:");
    lv_table_set_cell_value(table, 1, 1, FW_MAKER);
    lv_table_set_cell_value(table, 2, 0, "Version:");
    lv_table_set_cell_value(table, 2, 1, STRINGIFY(FW_VERSION));

    lv_table_set_cell_value(table, 4, 0, "MAC:");
    lv_table_set_cell_value(table, 4, 1, xMAC);

    lv_table_set_cell_value(table, 5, 0, "IP:");
    lv_table_set_cell_value(table, 5, 1, xIP);

    lv_table_set_cell_value(table, 6, 0, "MODE:");
    // #if defined(ETH_MODE)
    //     lv_table_set_cell_value(table, 6, 1, "Ethernet");
    // #else
        lv_table_set_cell_value(table, 6, 1, "WiFi");
    // #endif

    lv_table_set_cell_value(table, 7, 0, "MQTT:");
    lv_table_set_cell_value(table, 7, 1, xMQTT);
}