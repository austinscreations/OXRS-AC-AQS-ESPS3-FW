/*
 * OXRS_S3.h
 */

#ifndef OXRS_S3_H
#define OXRS_S3_H

#include <OXRS_MQTT.h>                // For MQTT pub/sub
#include <OXRS_API.h>                 // For REST API

// REST API
#define       REST_API_PORT             80

class OXRS_S3 : public Print
{
  public:
    void begin(jsonCallback config, jsonCallback command);
    void loop(void);

    // Firmware can define the config/commands it supports - for device discovery and adoption
    void setConfigSchema(JsonVariant json);
    void setCommandSchema(JsonVariant json);

    // Return a pointer to the MQTT library
    OXRS_MQTT * getMQTT(void);

    // Return a pointer to the API library
    OXRS_API * getAPI(void);

    // Helpers for publishing to stat/ and tele/ topics
    boolean publishStatus(JsonVariant json);
    boolean publishTelemetry(JsonVariant json);

    // Implement Print.h wrapper
    virtual size_t write(uint8_t);
    using Print::write;

    // for baseFirmWare to know Mqtt state
    bool mqttConnected = false;
    // for baseFirmWare to know network state
    bool networkConnected = false;

    // Helpers for retrieving the connection properties
    void getIPAddressTxt(char *buffer);
    void getMACAddressTxt(char *buffer);
    void getMQTTTopicTxt(char *buffer);


  private:
    void _initialiseNetwork(byte * mac);
    void _initialiseMqtt(byte * mac);
    void _initialiseRestApi(void);

    boolean _isNetworkConnected(void);
};

#endif