#pragma once

#include "networkConnection/IWifiConnection.hpp"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"

class StaWifiConnection : public IWifiConnection {
public:
    StaWifiConnection()  = default;
    ~StaWifiConnection() { stop(); }

    // INetworkConnection
    void           start()                                      override;
    void           stop()                                       override;
    NetworkStatus  getStatus()                            const override;
    esp_ip4_addr_t getIp()                                const override;
    void           setConnectionCallback(ConnectionCb cb)       override;

    // IWifiConnection — STA
    void   configure(const char* ssid, const char* password)    override;
    int8_t getRssi()                                      const override;
    void   triggerScan()                                        override;
    int    getScanResults(WifiScanResult* out, int maxCount)     override;

    // IWifiConnection — AP
    void           startAp(const char* namePrefix, const char* password = nullptr) override;
    void           stopAp()                                     override;
    bool           isApActive()                           const override;
    esp_ip4_addr_t getApIp()                              const override;

private:
    static constexpr char TAG[]    = "StaWifi";
    static constexpr int  SCAN_MAX = 32;

    ConnectionCb _cb;

    esp_netif_t*   _staNetif = nullptr;
    esp_netif_t*   _apNetif  = nullptr;
    bool           _apActive = false;
    esp_ip4_addr_t _apIp     = {};

    char _ssid[64] = {};
    char _pass[64] = {};

    WifiScanResult _scanResults[SCAN_MAX] = {};
    int            _scanCount             = 0;

    static void eventHandler(void* arg, esp_event_base_t base,
                             int32_t id, void* data);
    void        onEvent(esp_event_base_t base, int32_t id, void* data);
    void        fireCallback(NetworkStatus status);

    static void buildApSsid(char* out, size_t len, const char* namePrefix);
    void        applyStaConfig();
};
