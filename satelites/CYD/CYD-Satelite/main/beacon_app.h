#pragma once
#include "interfaces.h"
#include "web_server.h"
#include "wifi_manager.h"

class BeaconApp {
public:
    BeaconApp(ILedController& leds,
              IConfig&        config,
              WifiManager&    wifi,
              IMqttManager&   mqtt,
              WebServer&      web);

    void run();  // does not return

private:
    ILedController& m_leds;
    IConfig&        m_config;
    WifiManager&    m_wifi;
    IMqttManager&   m_mqtt;
    WebServer&      m_web;

    void onMessage(const char* data, int len);
    void applyState(const char* state);

    static void mqttTask(void* arg);
};
