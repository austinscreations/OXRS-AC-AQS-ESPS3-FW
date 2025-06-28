/*
 * OXRS_S3.cpp
 */

#include "Arduino.h"
#include "OXRS_S3.h"

#include <WiFi.h>        // Required for Ethernet to get MAC
#include <LittleFS.h>    // For file system access
#include <MqttLogger.h>  // For logging
#include <WiFiManager.h> // For WiFi AP config

// Macro for converting env vars to strings
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

// Network client (for MQTT)/server (for REST API)
WiFiClient _client;
WiFiServer _server(REST_API_PORT);

// MQTT client
PubSubClient _mqttClient(_client);
OXRS_MQTT _mqtt(_mqttClient);

// REST API
OXRS_API _api(_mqtt);

// Logging (topic updated once MQTT connects successfully)
MqttLogger _logger(_mqttClient, "log", MqttLoggerMode::MqttAndSerial);

// Supported firmware config and command schemas
JsonDocument _fwConfigSchema;
JsonDocument _fwCommandSchema;

// MQTT callbacks wrapped by _mqttConfig/_mqttCommand
jsonCallback _onConfig;
jsonCallback _onCommand;

/* JSON helpers */
void _mergeJson(JsonVariant dst, JsonVariantConst src)
{
  if (src.is<JsonObjectConst>())
  {
    for (JsonPairConst kvp : src.as<JsonObjectConst>())
    {
      if (dst[kvp.key()])
      {
        _mergeJson(dst[kvp.key()], kvp.value());
      }
      else
      {
        dst[kvp.key()] = kvp.value();
      }
    }
  }
  else
  {
    dst.set(src);
  }
}

/* Adoption info builders */
void _getFirmwareJson(JsonVariant json)
{
  JsonObject firmware = json.createNestedObject("firmware");

  firmware["name"] = FW_NAME;
  firmware["shortName"] = FW_SHORT_NAME;
  firmware["maker"] = FW_MAKER;
  firmware["version"] = STRINGIFY(FW_VERSION);

#if defined(FW_GITHUB_URL)
  firmware["githubUrl"] = FW_GITHUB_URL;
#endif
}

void _getSystemJson(JsonVariant json)
{
  JsonObject system = json.createNestedObject("system");

  system["heapUsedBytes"] = ESP.getHeapSize();
  system["heapFreeBytes"] = ESP.getFreeHeap();
  system["heapMaxAllocBytes"] = ESP.getMaxAllocHeap();
  system["flashChipSizeBytes"] = ESP.getFlashChipSize();

  system["sketchSpaceUsedBytes"] = ESP.getSketchSize();
  system["sketchSpaceTotalBytes"] = ESP.getFreeSketchSpace();

  system["fileSystemUsedBytes"] = LittleFS.usedBytes();
  system["fileSystemTotalBytes"] = LittleFS.totalBytes();
}

void _getNetworkJson(JsonVariant json)
{
  JsonObject network = json.createNestedObject("network");

  byte mac[6];
  WiFi.macAddress(mac);

  network["mode"] = "wifi";
  network["ip"] = WiFi.localIP();

  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  network["mac"] = mac_display;
}

void _getConfigSchemaJson(JsonVariant json)
{
  JsonObject configSchema = json.createNestedObject("configSchema");

  // Config schema metadata
  configSchema["$schema"] = JSON_SCHEMA_VERSION;
  configSchema["title"] = FW_SHORT_NAME;
  configSchema["type"] = "object";

  JsonObject properties = configSchema.createNestedObject("properties");

  // Firmware config schema (if any)
  if (!_fwConfigSchema.isNull())
  {
    _mergeJson(properties, _fwConfigSchema.as<JsonVariant>());
  }
}

void _getCommandSchemaJson(JsonVariant json)
{
  JsonObject commandSchema = json.createNestedObject("commandSchema");

  // Command schema metadata
  commandSchema["$schema"] = JSON_SCHEMA_VERSION;
  commandSchema["title"] = FW_SHORT_NAME;
  commandSchema["type"] = "object";

  JsonObject properties = commandSchema.createNestedObject("properties");

  // Firmware command schema (if any)
  if (!_fwCommandSchema.isNull())
  {
    _mergeJson(properties, _fwCommandSchema.as<JsonVariant>());
  }

  // Generic commands
  JsonObject restart = properties.createNestedObject("restart");
  restart["title"] = "Restart";
  restart["type"] = "boolean";
}

/* API callbacks */
void _apiAdopt(JsonVariant json)
{
  // Build device adoption info
  _getFirmwareJson(json);
  _getSystemJson(json);
  _getNetworkJson(json);
  _getConfigSchemaJson(json);
  _getCommandSchemaJson(json);
}

/* MQTT callbacks */
void _mqttConnected()
{
  // MqttLogger doesn't copy the logging topic to an internal
  // buffer so we have to use a static array here
  static char logTopic[64];
  _logger.setTopic(_mqtt.getLogTopic(logTopic));

  // Publish device adoption info
  JsonDocument json;
  _mqtt.publishAdopt(_api.getAdopt(json.as<JsonVariant>()));

  // Log the fact we are now connected
  _logger.println("[espS3] mqtt connected");
}

void _mqttDisconnected(int state)
{
  // Log the disconnect reason
  // See https://github.com/knolleary/pubsubclient/blob/2d228f2f862a95846c65a8518c79f48dfc8f188c/src/PubSubClient.h#L44
  switch (state)
  {
  case MQTT_CONNECTION_TIMEOUT:
    _logger.println(F("[espS3] mqtt connection timeout"));
    break;
  case MQTT_CONNECTION_LOST:
    _logger.println(F("[espS3] mqtt connection lost"));
    break;
  case MQTT_CONNECT_FAILED:
    _logger.println(F("[espS3] mqtt connect failed"));
    break;
  case MQTT_DISCONNECTED:
    _logger.println(F("[espS3] mqtt disconnected"));
    break;
  case MQTT_CONNECT_BAD_PROTOCOL:
    _logger.println(F("[espS3] mqtt bad protocol"));
    break;
  case MQTT_CONNECT_BAD_CLIENT_ID:
    _logger.println(F("[espS3] mqtt bad client id"));
    break;
  case MQTT_CONNECT_UNAVAILABLE:
    _logger.println(F("[espS3] mqtt unavailable"));
    break;
  case MQTT_CONNECT_BAD_CREDENTIALS:
    _logger.println(F("[espS3] mqtt bad credentials"));
    break;
  case MQTT_CONNECT_UNAUTHORIZED:
    _logger.println(F("[espS3] mqtt unauthorised"));
    break;
  }
}

void _mqttConfig(JsonVariant json)
{
  // Pass on to the firmware callback
  if (_onConfig)
  {
    _onConfig(json);
  }
}

void _mqttCommand(JsonVariant json)
{
  // Check for GPIO32 commands
  if (json.containsKey("restart") && json["restart"].as<bool>())
  {
    ESP.restart();
  }

  // Pass on to the firmware callback
  if (_onCommand)
  {
    _onCommand(json);
  }
}

void _mqttCallback(char *topic, byte *payload, int length)
{
  // Pass down to our MQTT handler and check it was processed ok
  int state = _mqtt.receive(topic, payload, length);
  switch (state)
  {
  case MQTT_RECEIVE_ZERO_LENGTH:
    _logger.println(F("[espS3] empty mqtt payload received"));
    break;
  case MQTT_RECEIVE_JSON_ERROR:
    _logger.println(F("[espS3] failed to deserialise mqtt json payload"));
    break;
  case MQTT_RECEIVE_NO_CONFIG_HANDLER:
    _logger.println(F("[espS3] no mqtt config handler"));
    break;
  case MQTT_RECEIVE_NO_COMMAND_HANDLER:
    _logger.println(F("[espS3] no mqtt command handler"));
    break;
  }
}

/* Main program */
void OXRS_S3::begin(jsonCallback config, jsonCallback command)
{
  // Get our firmware details
  JsonDocument json;
  _getFirmwareJson(json.as<JsonVariant>());

  // Log firmware details
  _logger.print(F("[espS3] "));
  serializeJson(json, _logger);
  _logger.println();

  // We wrap the callbacks so we can intercept messages intended for the GPIO32
  _onConfig = config;
  _onCommand = command;

  // Set up network and obtain an IP address
  byte mac[6];
  _initialiseNetwork(mac);

  // Set up MQTT (don't attempt to connect yet)
  _initialiseMqtt(mac);

  // Set up the REST API
  _initialiseRestApi();
}

void OXRS_S3::loop(void)
{
  // Check our network connection
  if (_isNetworkConnected())
  {
    // Handle any MQTT messages
    _mqtt.loop();

    // bool checkMqtt = _mqtt.connected();

    if (_mqtt.connected() == true)
    {
      mqttConnected = true;
    }
    else
    {
      mqttConnected = false;
    }

    // Handle any REST API requests
    WiFiClient client = _server.available();
    _api.loop(&client);
  }
}

void OXRS_S3::setConfigSchema(JsonVariant json)
{
  _fwConfigSchema.clear();
  _mergeJson(_fwConfigSchema.as<JsonVariant>(), json);
}

void OXRS_S3::setCommandSchema(JsonVariant json)
{
  _fwCommandSchema.clear();
  _mergeJson(_fwCommandSchema.as<JsonVariant>(), json);
}

OXRS_MQTT *OXRS_S3::getMQTT()
{
  return &_mqtt;
}

OXRS_API *OXRS_S3::getAPI()
{
  return &_api;
}

boolean OXRS_S3::publishStatus(JsonVariant json)
{
  // Exit early if no network connection
  if (!_isNetworkConnected())
  {
    return false;
  }
  return _mqtt.publishStatus(json);
}

boolean OXRS_S3::publishTelemetry(JsonVariant json)
{
  // Exit early if no network connection
  if (!_isNetworkConnected())
  {
    return false;
  }
  return _mqtt.publishTelemetry(json);
}

size_t OXRS_S3::write(uint8_t character)
{
  // Pass to logger - allows firmware to use `GPIO32.println("Log this!")`
  return _logger.write(character);
}

void OXRS_S3::_initialiseNetwork(byte *mac)
{
  // Get WiFi base MAC address
  WiFi.macAddress(mac);

  // Format the MAC address for logging
  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  _logger.print(F("[espS3] wifi mac address: "));
  _logger.println(mac_display);

  // Ensure we are in the correct WiFi mode
  WiFi.mode(WIFI_STA);

  // Connect using saved creds, or start captive portal if none found
  // NOTE: Blocks until connected or the portal is closed
  WiFiManager wm;
  bool success = wm.autoConnect("OXRS_WiFi", "superhouse");

  _logger.print(F("[espS3] ip address: "));
  _logger.println(success ? WiFi.localIP() : IPAddress(0, 0, 0, 0));
}

void OXRS_S3::_initialiseMqtt(byte *mac)
{
  // NOTE: this must be called *before* initialising the REST API since
  //       that will load MQTT config from file, which has precendence

  // Set the default client ID to last 3 bytes of the MAC address
  char clientId[32];
  sprintf_P(clientId, PSTR("%02x%02x%02x"), mac[3], mac[4], mac[5]);
  _mqtt.setClientId(clientId);

  // Register our callbacks
  _mqtt.onConnected(_mqttConnected);
  _mqtt.onDisconnected(_mqttDisconnected);
  _mqtt.onConfig(_mqttConfig);
  _mqtt.onCommand(_mqttCommand);

  // Start listening for MQTT messages
  _mqttClient.setCallback(_mqttCallback);
}

void OXRS_S3::_initialiseRestApi(void)
{
  // NOTE: this must be called *after* initialising MQTT since that sets
  //       the default client id, which has lower precendence than MQTT
  //       settings stored in file and loaded by the API

  // Set up the REST API
  _api.begin();

  // Register our callbacks
  _api.onAdopt(_apiAdopt);

  // Start listening
  _server.begin();
}

boolean OXRS_S3::_isNetworkConnected(void)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    networkConnected = true;
  }
  else
  {
    networkConnected = false;
  }

  return networkConnected;
}

void OXRS_S3::getIPAddressTxt(char *buffer)
{
  IPAddress ip = IPAddress(0, 0, 0, 0);

  if (_isNetworkConnected())
  {
    // #if defined(ETH_MODE)
    //   ip = Ethernet.localIP();
    // #else
      ip = WiFi.localIP();
    // #endif
  }

  if (ip[0] == 0)
  {
    sprintf(buffer, "---.---.---.---");
  }
  else
  {
    sprintf(buffer, "%03d.%03d.%03d.%03d", ip[0], ip[1], ip[2], ip[3]);
  }
}

void OXRS_S3::getMACAddressTxt(char *buffer)
{
  byte mac[6];

// #if defined(ETH_MODE)
//   Ethernet.MACAddress(mac);
// #else
  WiFi.macAddress(mac);
// #endif

  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void OXRS_S3::getMQTTTopicTxt(char *buffer)
{
  char topic[64];

  if (!_mqtt.connected())
  {
    sprintf(buffer, "-/------");
  }
  else
  {
    _mqtt.getWildcardTopic(topic);
    strcpy(buffer, "");
    strncat(buffer, topic, 39);
  }
}