/**
  Air Quality sensor firmware for the Open eXtensible Rack System

  GitHub repository:
    TBA

  Copyright 2024 SuperHouse Automation

  Written By Austin's Creations
*/

/*--------------------------- Libraries -------------------------------*/
#include <Arduino.h>   // Programming core language and functions
#include <OXRS_Input.h> // For input handling
#include <OXRS_HASS.h> // OXRS Home Assistant Support

#if defined(OXRS_ESP32_S3)
#include <OXRS_S3.h> // generic OXRS s3 library (based on the esp32 library)
OXRS_S3 oxrs;
#endif

#include <SPI.h>  // For SPI
#include <Wire.h> // For I2C

#include <EEPROM.h>
#include <bsec.h> // Library for the BME680 sensor

#include <Plantower_PMS7003.h> // Library for the AQS sensor

#include "classTft.h" // custom library with the Tft handling

/*--------------------------- Constants -------------------------------*/
// Serial
#define SERIAL_BAUD_RATE 115200

// default MQTT timing for sending sensor data
#define DEFAULT_TELEMETRY_INTERVAL_MS 1000
#define TELEMETRY_INTERVAL_MS_MAX 60000

// default TFT timing for sending sensor data
#define DEFAULT_TFT_INTERVAL_MS 1000
#define TFT_INTERVAL_MS_MAX 60000

// Temperature units
#define TEMP_C 0
#define TEMP_F 1

// ESP efuse ID
uint32_t chipId = 0;

/*--------------------------- BME Bosch Helpers -------------------------------*/

const uint8_t bsec_config_iaq[] =
    {
#include "resources/bsec_iaq.txt"
};

bsec_virtual_sensor_t sensorList[5] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
};

uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;

#define STATE_SAVE_PERIOD UINT32_C(360 * 60 * 1000) // 360 minutes - 4 times a day

/*--------------------------- Global Variables ------------------------*/
// How often to send sensor data to MQTT
uint32_t telemetryIntervalMs = DEFAULT_TELEMETRY_INTERVAL_MS;

// How often to send sensor data to TFT
uint32_t tftIntervalMs = DEFAULT_TFT_INTERVAL_MS;

// Last time sensor data was sent to MQTT
uint32_t lastTelemetryMs = 0L;

// Last time sensor data was sent to TFT
uint32_t lastTftMs = 0L;

// Publish Home Assistant self-discovery config for each sensor
bool hassDiscoveryPublished[8];

// Saving gathered sensor data from PMS sensor
uint16_t PM1_0 = 0;
uint16_t PM2_5 = 0;
uint16_t PM10 = 0;

// Saving gathered sensor data from BME sensor
uint8_t iaqAccuracy = 0;
uint16_t co2e = 0;
float bvoc = 0.0;
float temp = 0.0;
float hum = 0.0;

// Variable for mode that the button is in - true means button directly controls screen
bool buttonControl = true;

// BME IC Found
bool bmeFound = false;

// PMS IC Found
bool pmsFound = false;

// unit used for BME sensor output
uint8_t tempUnits = TEMP_C;

// used to build Home assitant auto discovery
const char *name[8] = {"Temperature", "Humidity", "CO2 Equivalent", "Breath VOC", "AQI Accuracy", "PM1.0", "PM2.5", "PM10"};
const char *nameClass[8] = {"temperature", "humidity", "aqi", "aqi", "aqi", "PM1", "PM25", "PM10"};
const char *nameMqtt[8] = {"temperature", "humidity", "co2e", "bvoc", "iaqAccuracy", "PM1_0", "PM2_5", "PM10"};
const char *nameUnits[8] = {"0", "%", "PPM", "PPM", "#", "µg/m³", "µg/m³", "µg/m³"};

// used for the button library mqtt message
char mqttMessageBuffer[64];

/*--------------------------- Instantiate Globals ---------------------*/
// home assistant discovery config
OXRS_HASS hass(oxrs.getMQTT());

// Button
OXRS_Input oxrsInput;

// ESP32 S3 COMs
HardwareSerial &comm = Serial1;

// BME680
Bsec bme;

// PMS7003
Plantower_PMS7003 pms7003 = Plantower_PMS7003();

// TFT
classTft display = classTft();

/*--------------------------- Bosch BME helpers ---------------------------------*/

void bmeLoadState(void)
{
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE)
  {
    // Existing state in EEPROM
    Serial.println("Reading state from EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
    {
      bsecState[i] = EEPROM.read(i + 1);
    }

    bme.setState(bsecState);
  }
  else
  {
    // Erase the EEPROM with zeroes
    Serial.println("Erasing EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++)
      EEPROM.write(i, 0);

    EEPROM.commit();
  }
}

void bmeUpdateState(void)
{
  bool update = false;
  if (stateUpdateCounter == 0)
  {
    /* First state update when IAQ accuracy is >= 3 */
    if (bme.iaqAccuracy >= 3)
    {
      update = true;
      stateUpdateCounter++;
    }
  }
  else
  {
    /* Update every STATE_SAVE_PERIOD minutes */
    if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis())
    {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update)
  {
    bme.getState(bsecState);

    if (bme.bsecStatus == BSEC_OK && bme.bme68xStatus == BME68X_OK)
    {
      Serial.println("Writing state to EEPROM");

      for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
      {
        EEPROM.write(i + 1, bsecState[i]);
      }

      EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
      EEPROM.commit();
    }
  }
}

/*--------------------------- MQTT ---------------------------------*/

void setCommandSchema()
{
  JsonDocument json;

  JsonObject backLight = json["backLight"].to<JsonObject>();
  backLight["title"] = "BackLight";
  backLight["description"] = "Can be used to remote wake up the screen - screen will auto dim after timeout again";
  backLight["type"] = "boolean";

  JsonObject nextScreen = json["nextScreen"].to<JsonObject>();
  nextScreen["title"] = "Next Screen";
  nextScreen["description"] = "Can be used to remote change between screens - currently the Data screen and device Info screen";
  nextScreen["type"] = "boolean";

  oxrs.setCommandSchema(json.as<JsonVariant>());
}

void jsonCommand(JsonVariant json)
{
  if (json["backLight"].is<bool>())
  {
    display.backLightWake();
  }

  if (json["nextScreen"].is<bool>())
  {
    display.backLightWake();
    display.nextScreen();
  }
}

void setConfigSchema()
{
  // Define our config schema
  JsonDocument json;

  JsonObject telemetryIntervalMs = json["telemetryIntervalMs"].to<JsonObject>();
  telemetryIntervalMs["title"] = "Telemetry Interval (ms)";
  telemetryIntervalMs["description"] = "How often to publish telemetry data (defaults to 1000ms, i.e. 1 second)";
  telemetryIntervalMs["type"] = "integer";
  telemetryIntervalMs["minimum"] = 1;
  telemetryIntervalMs["maximum"] = TELEMETRY_INTERVAL_MS_MAX;

  JsonObject tftIntervalMs = json["tftIntervalMs"].to<JsonObject>();
  tftIntervalMs["title"] = "Tft Interval (ms)";
  tftIntervalMs["description"] = "How often to update the screen data (defaults to 1000ms, i.e. 1 second)";
  tftIntervalMs["type"] = "integer";
  tftIntervalMs["minimum"] = 1;
  tftIntervalMs["maximum"] = TFT_INTERVAL_MS_MAX;

  JsonObject tempOffset = json["tempOffset"].to<JsonObject>();
  tempOffset["title"] = "BME Temperature Offset";
  tempOffset["description"] = "Sets the tempurature offset for value reported by BME (defaults to 0) Allows precision to one decimal place";
  tempOffset["type"] = "number";
  tempOffset["multipleOf"] = 0.1;
  tempOffset["minimum"] = -50;
  tempOffset["maximum"] = 50;

  JsonObject tempUnits = json["sensorTempUnits"].to<JsonObject>();
  tempUnits["title"] = "Sensor Temperature Units";
  tempUnits["description"] = "Publish temperature reports in celcius (default) or farenhite";
  tempUnits["type"] = "string";
  JsonArray tempUnitsEnum = tempUnits["enum"].to<JsonArray>();
  tempUnitsEnum.add("c");
  tempUnitsEnum.add("f");
  JsonArray tempUnitsEnumNames = tempUnits["enumNames"].to<JsonArray>();
  tempUnitsEnumNames.add("celcius");
  tempUnitsEnumNames.add("farenhite");

  // noActivity timeout
  JsonObject noActivitySecondsToSleep = json["noActivitySecondsToSleep"].to<JsonObject>();
  noActivitySecondsToSleep["title"] = "Screen Sleep Timeout (seconds)";
  noActivitySecondsToSleep["description"] = "Turn off screen backlight after a period of in-activity (defaults to 0 which disables the timeout). Must be a number between 0 and 3600 (i.e. 1 hour).";
  noActivitySecondsToSleep["type"] = "integer";
  noActivitySecondsToSleep["minimum"] = 0;
  noActivitySecondsToSleep["maximum"] = TFT_TIMEOUT_INTERVAL_MS_MAX;

  // max screen brightness
  JsonObject maxBrightness = json["maxBrightness"].to<JsonObject>();
  maxBrightness["title"] = "Screen Max Brightness (percent)";
  maxBrightness["description"] = "Set the max brightness of the screen. 1-100 percent - DEFAULT is 35 percent";
  maxBrightness["type"] = "integer";
  maxBrightness["minimum"] = 1;
  maxBrightness["maximum"] = 100;

  // enable all button presses direct to mqtt vs single press controlling screen
  JsonObject button = json["button"].to<JsonObject>();
  button["title"] = "Button Local Control";
  button["description"] = "Enable single press on button to control screen directly or to bypass and send to mqtt - DEFAULTS to local";
  button["type"] = "string";
  JsonArray buttonEnum = button["enum"].to<JsonArray>();
  buttonEnum.add("local");
  buttonEnum.add("mqtt");
  JsonArray buttonEnumNames = button["enumNames"].to<JsonArray>();
  buttonEnumNames.add("enable");
  buttonEnumNames.add("disable");

  JsonObject warningLevels = json["warningLevels"].to<JsonObject>();
  warningLevels["type"] = "array";
  warningLevels["description"] = "Set the levels for PM warning color change";
  warningLevels["maxItems"] = 1;

  JsonObject warningLevelsItems = warningLevels["items"].to<JsonObject>();
  warningLevelsItems["type"] = "object";

  JsonObject warningLevelsProperties = warningLevelsItems["properties"].to<JsonObject>();

  JsonObject redWarn1_0 = warningLevelsProperties["redWarn1_0"].to<JsonObject>();
  redWarn1_0["title"] = "Red Warning PM1.0";
  redWarn1_0["description"] = "Particles in PM1.0 equal and above this range will trigger RED warning on TFT (defaults to 20 ug/m^3) (Allowed range is 5 - 450)";
  redWarn1_0["type"] = "integer";
  redWarn1_0["minimum"] = 5;
  redWarn1_0["maximum"] = 450;

  JsonObject yellowWarn1_0 = warningLevelsProperties["yellowWarn1_0"].to<JsonObject>();
  yellowWarn1_0["title"] = "Yellow Warning PM1.0";
  yellowWarn1_0["description"] = "Particles in PM1.0 above this range will trigger YELLOW warning on TFT (defaults to 8 ug/m^3) (Allowed range is 5 - 450)";
  yellowWarn1_0["type"] = "integer";
  yellowWarn1_0["minimum"] = 5;
  yellowWarn1_0["maximum"] = 450;

  JsonObject redWarn2_5 = warningLevelsProperties["redWarn2_5"].to<JsonObject>();
  redWarn2_5["title"] = "Red Warning PM2.5";
  redWarn2_5["description"] = "Particles in PM2.5 equal and above this range will trigger RED warning on TFT (defaults to 35 ug/m^3) (Allowed range is 5 - 450)";
  redWarn2_5["type"] = "integer";
  redWarn2_5["minimum"] = 5;
  redWarn2_5["maximum"] = 450;

  JsonObject yellowWarn2_5 = warningLevelsProperties["yellowWarn2_5"].to<JsonObject>();
  yellowWarn2_5["title"] = "Yellow Warning PM2.5";
  yellowWarn2_5["description"] = "Particles in PM2.5 above this range will trigger YELLOW warning on TFT (defaults to 10 ug/m^3) (Allowed range is 5 - 450)";
  yellowWarn2_5["type"] = "integer";
  yellowWarn2_5["minimum"] = 5;
  yellowWarn2_5["maximum"] = 450;

  JsonObject redWarn10 = warningLevelsProperties["redWarn10"].to<JsonObject>();
  redWarn10["title"] = "Yellow Warning PM10";
  redWarn10["description"] = "Particles in PM10 equal and above this range will trigger RED warning on TFT (defaults to 50 ug/m^3) (Allowed range is 5 - 450)";
  redWarn10["type"] = "integer";
  redWarn10["minimum"] = 5;
  redWarn10["maximum"] = 450;

  JsonObject yellowWarn10 = warningLevelsProperties["yellowWarn10"].to<JsonObject>();
  yellowWarn10["title"] = "Red Warning PM10";
  yellowWarn10["description"] = "Particles in PM10 above this range will trigger YELLOW warning on TFT (defaults to 20 ug/m^3) (Allowed range is 5 - 450)";
  yellowWarn10["type"] = "integer";
  yellowWarn10["minimum"] = 5;
  yellowWarn10["maximum"] = 450;

  JsonArray required = warningLevelsItems["required"].to<JsonArray>();
  required.add("redWarn1_0");
  required.add("yellowWarn1_0");
  required.add("redWarn2_5");
  required.add("yellowWarn2_5");
  required.add("redWarn10");
  required.add("yellowWarn10");

  // Add any Home Assistant config
  hass.setConfigSchema(json);

  // Pass our config schema down to the Room8266 library
  oxrs.setConfigSchema(json.as<JsonVariant>());
}

void jsonConfig(JsonVariant json)
{
  if (json["telemetryIntervalMs"].is<int>())
  {
    telemetryIntervalMs = min(json["telemetryIntervalMs"].as<int>(), TELEMETRY_INTERVAL_MS_MAX);
  }

  if (json["sensorTempUnits"].is<const char *>())
  {
    if (strcmp(json["sensorTempUnits"], "c") == 0)
    {
      tempUnits = TEMP_C;
      display.updateTempUnits(tempUnits);
    }
    else if (strcmp(json["sensorTempUnits"], "f") == 0)
    {
      tempUnits = TEMP_F;
      display.updateTempUnits(tempUnits);
    }
  }

  if (json["warningLevels"].is<JsonVariant>())
  {
    JsonObject warningLevels_0 = json["warningLevels"][0];
    display.updateWarnLevels(warningLevels_0["yellowWarn1_0"], warningLevels_0["redWarn1_0"], warningLevels_0["yellowWarn2_5"], warningLevels_0["redWarn2_5"], warningLevels_0["yellowWarn10"], warningLevels_0["redWarn10"]);
  }

  if (json["tempOffset"].is<float>())
  {
    if (bmeFound)
    {
      bme.setTemperatureOffset(json["tempOffset"].as<float>());
    }
  }

  if (json["button"].is<const char *>())
  {
    if (strcmp(json["button"], "local") == 0)
    {
      buttonControl = true;
    }
    else if (strcmp(json["button"], "mqtt") == 0)
    {
      buttonControl = false;
    }
  }

  if (json["noActivitySecondsToSleep"].is<int>())
  {
    display.backLightWake();
    display.tftTimeoutIntervalMs = (json["noActivitySecondsToSleep"].as<int>() * 1000);
    display.backLightWake();
  }

  if (json["maxBrightness"].is<uint8_t>())
  {
    display.maxBrightness = json["maxBrightness"].as<uint8_t>();
    display.backLightWake();
  }

  // Handle any Home Assistant config
  hass.parseConfig(json);
}

void publishHassDiscovery()
{
  char topic[64];

  char component[8];
  sprintf_P(component, PSTR("sensor"));

  char id[8];
  char valueTemplate[128];

  for (int x = 0; x < 8; x++)
  {
    if (hassDiscoveryPublished[x])
      return;

    sprintf_P(id, PSTR("AQS_%d"), x);

    JsonDocument json;
    hass.getDiscoveryJson(json, id);

    if (x == 0) // temp sensor change the units if needed
    {
      if (tempUnits == TEMP_F)
      {
        json["unit_of_meas"] = "°F";
      }
      else
      {
        json["unit_of_meas"] = "°C";
      }
    }
    else
    {
      json["unit_of_meas"] = nameUnits[x];
    }

    sprintf_P(valueTemplate, PSTR("{{value_json.%s }}"), nameMqtt[x]);

    json["name"] = name[x];
    json["dev_cla"] = nameClass[x];
    json["val_tpl"] = valueTemplate;
    json["stat_t"] = oxrs.getMQTT()->getTelemetryTopic(topic);
    json["frc_upd"] = true;

    hassDiscoveryPublished[x] = hass.publishDiscoveryJson(json, component, id);
  }
}

/*--------------------------- Button helpers ---------------------------------*/

void getEventType(char eventType[], uint8_t type, uint8_t state)
{
  // Determine what event we need to publish
  sprintf_P(eventType, PSTR("error"));
  switch (type)
  {
  case BUTTON:
    switch (state)
    {
    case HOLD_EVENT:
      sprintf_P(eventType, PSTR("hold"));
      break;
    case RELEASE_EVENT:
      sprintf_P(eventType, PSTR("release"));
      break;
    case 1:
      sprintf_P(eventType, PSTR("single"));
      break;
    case 2:
      sprintf_P(eventType, PSTR("double"));
      break;
    case 3:
      sprintf_P(eventType, PSTR("triple"));
      break;
    case 4:
      sprintf_P(eventType, PSTR("quad"));
      break;
    case 5:
      sprintf_P(eventType, PSTR("penta"));
      break;
    }
    break;
  }
}

void publishEvent(uint8_t index, uint8_t type, uint8_t state)
{
  char inputType[9];
  sprintf_P(inputType, PSTR("button")); // only one button being used and it is static config
  char eventType[8];
  getEventType(eventType, type, state);

  JsonDocument json;
  json["index"] = index;
  json["type"] = inputType;
  json["event"] = eventType;

  if (buttonControl && strcmp(eventType, "single") == 0)
  {
    display.nextScreen();
  }
  else
  {
    oxrs.publishStatus(json.as<JsonVariant>());
  }
}

/**
  Event handlers
*/
void inputEvent(uint8_t id, uint8_t input, uint8_t type, uint8_t state)
{
  // Determine the index for this input event (1-based)
  uint8_t index = input + 1;

  // Publish the event
  publishEvent(index, type, state);
}

/*--------------------------- Program ---------------------------------*/
// rounds a float value to 1 decimal point
double roundTo1Dp(double value)
{
  return (int)(value * 10.0) / 10.0;
}

// scans an I2C address for a valid device
bool scanI2CAddress(byte address, const char *name)
{
  Serial.print(F("[MAIN] - 0x"));
  Serial.print(address, HEX);
  Serial.print(F("..."));

  // Check if there is anything responding on this address
  Wire.beginTransmission(address);
  if (Wire.endTransmission() == 0)
  {
    Serial.println(name);
    return true;
  }
  else
  {
    Serial.println(F("empty"));
    return false;
  }
}

void printEspInfo()
{
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("Number of Cores = %d \n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.println(chipId);

  Serial.printf("Flash Size: %d \n", ESP.getFlashChipSize());
  Serial.printf("Psram Size: %d \n", ESP.getPsramSize());
  Serial.printf("Ram Size: %d \n", ESP.getHeapSize());
}

/**
  Setup
*/
void setup()
{
  // Start serial and let settle
  delay(5000);
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  Serial.println(F("[AQS] starting up..."));

  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);

  // Prints out the ESP Chip Information
  printEspInfo();

  // Starts up LVGL + TFT
  display.begin();

  // // add control for the PMS sensor pins
  pinMode(PMS_SET, OUTPUT);   // sleep control - low is sleep
  pinMode(PMS_RESET, OUTPUT); // Reset control - low is reset

  // //Turn the PMS sensor ON
  digitalWrite(PMS_SET, HIGH);
  digitalWrite(PMS_RESET, HIGH);
  delay(500);

  comm.begin(PMS_BAUD, SERIAL_8N1, PMS_TX, PMS_RX);
  delay(250);
  pms7003.init(&comm);

  // Start the I2C bus
  Wire.begin(I2C_SDA, I2C_SCL);

  if (scanI2CAddress(BME_I2C_ADDRESS, "BME680"))
  {
    bme.begin(BME_I2C_ADDRESS, Wire);

    if (bme.bsecStatus == BSEC_OK && bme.bme68xStatus == BME68X_OK)
    {
      bmeFound = true;
      bme.setConfig(bsec_config_iaq);
      bmeLoadState();
      // bme.updateSubscription(sensorList, 5, BSEC_SAMPLE_RATE_CONT);
      bme.updateSubscription(sensorList, 5, BSEC_SAMPLE_RATE_LP);
      // bme.updateSubscription(sensorList, 5, BSEC_SAMPLE_RATE_ULP);
      if (bme.bsecStatus != BSEC_OK && bme.bme68xStatus != BME68X_OK)
      {
        bmeFound = false;
      }
    }
  }

  // uses oxrs input handler
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  // Initialise input handlers (default to BUTTON)
  oxrsInput.begin(inputEvent, BUTTON);

  // // Start S3 hardware
  oxrs.begin(jsonConfig, jsonCommand);

  // Set up schema (for self-discovery and adoption)
  setConfigSchema();
  setCommandSchema();
}

/**
  Main processing loop
*/
void loop()
{
  // Let S3 hardware handle any events etc
  oxrs.loop();

  // Check for any input events
  bool inputState = digitalRead(MODE_BUTTON);
  oxrsInput.processInput(0, 0, inputState);

  // looks for updated information from PMS sensor - when complete hasNewData() == true
  pms7003.updateFrame();

  // we have new PMS data - update variables
  if (pms7003.hasNewData())
  {
    pmsFound = true;
    PM1_0 = pms7003.getPM_1_0();
    PM2_5 = pms7003.getPM_2_5();
    PM10 = pms7003.getPM_10_0();
  }

  if (bmeFound) // bme was found and initilized
  {
    if (bme.run()) // bme has new data
    {
      bmeUpdateState();
      co2e = int(trunc(bme.co2Equivalent));
      bvoc = roundTo1Dp(bme.breathVocEquivalent);
      hum = roundTo1Dp(bme.humidity);
      iaqAccuracy = bme.iaqAccuracy;

      if (tempUnits == TEMP_F)
      {
        temp = roundTo1Dp((bme.temperature * 1.8) + 32);
      }
      else
      {
        temp = roundTo1Dp(bme.temperature);
      }
    }
  }

  // Check if we need to update Tft
  if (millis() - lastTftMs >= tftIntervalMs)
  {
    display.setWifiStatus(oxrs.networkConnected, oxrs.mqttConnected);
    char buffer0[40];
    char buffer1[40];
    char buffer2[40];
    oxrs.getMACAddressTxt(buffer0);
    oxrs.getIPAddressTxt(buffer1);
    oxrs.getMQTTTopicTxt(buffer2);
    display.setInfoData(buffer0,buffer1,buffer2);
    if (bmeFound)
    {
      display.sendBmeData(iaqAccuracy, co2e, bvoc, hum, temp);
    }
    if (pmsFound)
    {
      display.sendPmsData(PM1_0, PM2_5, PM10);
    }

    display.loop();
    lastTftMs = millis();
  }

  // Check if we need to send telemetry
  if (millis() - lastTelemetryMs >= telemetryIntervalMs)
  {
    // Build telemetry payload
    JsonDocument json;

    if (pmsFound)
    {
      json["PM1_0"] = PM1_0;
      json["PM2_5"] = PM2_5;
      json["PM10"] = PM10;
    }

    if (bmeFound)
    {
      json["temperature"] = temp;
      json["humidity"] = hum;
      // if accuracy is considered too low don't send data
      if (iaqAccuracy > 0)
      {
        json["co2e"] = co2e;
        json["bvoc"] = bvoc;
        json["iaqAccuracy"] = iaqAccuracy;
      }
      else
      {
        json["co2e"] = 0;
        json["bvoc"] = 0;
        json["iaqAccuracy"] = 0;
      }
    }

    // Publish telemetry and reset loop variables if successful
    bool haveData = !json.isNull();
    if (haveData)
    {
      if (oxrs.publishTelemetry(json))
      {
        lastTelemetryMs = millis();
      }
    }
    else
    {
      lastTelemetryMs = millis();
    }
  }

  // Check if we need to publish any Home Assistant discovery payloads
  if (hass.isDiscoveryEnabled())
  {
    publishHassDiscovery();
  }
}
