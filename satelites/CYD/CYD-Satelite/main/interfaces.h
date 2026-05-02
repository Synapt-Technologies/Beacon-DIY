#pragma once

#include <functional>
#include <stdint.h>
#include "esp_netif_types.h"
#include "esp_wifi_types.h"

struct DeviceConfig {
    char    wifi_ssid[64]   = {};
    char    wifi_pass[64]   = {};
    char    mqtt_url[128]   = {};
    char    mqtt_topic[128] = {};
    char    device_name[32] = "CYD Satelite";
    uint8_t led_brightness  = 255;
};

class IConfig {
public:
    virtual ~IConfig() = default;
    virtual bool               load()                    = 0;
    virtual bool               save()                    = 0;
    virtual const DeviceConfig& get()              const = 0;
    virtual void               set(const DeviceConfig&)  = 0;
};

class ILedController {
public:
    virtual ~ILedController() = default;
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;
};

class IWifiManager {
public:
    virtual ~IWifiManager() = default;
    virtual void           start()                                           = 0;
    virtual bool           isConnected()                               const = 0;
    virtual esp_ip4_addr_t getStaIp()                                  const = 0;
    virtual int            getApRecords(wifi_ap_record_t* out,
                                        uint16_t max)                  const = 0;
};

class IMqttManager {
public:
    using MessageCb = std::function<void(const char* data, int len)>;
    virtual ~IMqttManager() = default;
    virtual void start(const char* url, const char* topic, MessageCb cb) = 0;
    virtual void stop()                                                   = 0;
    virtual bool isConnected()                                      const = 0;
};
