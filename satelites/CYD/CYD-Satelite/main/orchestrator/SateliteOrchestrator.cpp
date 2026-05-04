#include "orchestrator/SateliteOrchestrator.hpp"

#include "esp_log.h"
#include <cstdio>
#include <cstring>


// ? Lifecycle

void SateliteOrchestrator::start()
{
    // TODO Add checks. e.g. deviceType can only be Single in this orchestrator.


    _config.onNetworkChanged([this](const Settings::Network& s){ onNetworkChanged(s); });
    _config.onBeaconChanged ([this](const Settings::Beacon&  s){ onBeaconChanged(s);  });
    _config.onDisplayChanged([this](const Settings::Display& s){ onDisplayChanged(s); });

    _beacon.setTallyCallback([this](TallyState state)                                        { applyTally(state);          });
    _beacon.setAlertCallback([this](DeviceAlertAction a, DeviceAlertTarget t, uint32_t ms)   { applyAlert(a, t, ms);       });


    _config.load(); // This will trigger the callbacks and start the beacon if WiFi is connected.

    ESP_LOGI(TAG, "Started");
}

// ? Config callbacks

void SateliteOrchestrator::onNetworkChanged(const Settings::Network& s)
{
    ESP_LOGI(TAG, "Network settings changed, reconfiguring WiFi");
    _wifi.configure(s.ssid, s.password);
    // _beacon.stop(); // TODO Check if needed.
}

void SateliteOrchestrator::onBeaconChanged(const Settings::Beacon& s)
{
    ESP_LOGI(TAG, "Beacon settings changed, reconnecting");

    char consumerId[48];
    if (s.consumerId[0][0] != '\0') {
        strncpy(consumerId, s.consumerId[0], sizeof(consumerId) - 1);
    } else {
        strncpy(consumerId, "aedes", sizeof(consumerId) - 1);
    }

    char deviceId[48];
    if (s.deviceId[0][0] != '\0') {
        strncpy(deviceId, s.deviceId[0], sizeof(deviceId) - 1);
    } else {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        snprintf(deviceId, sizeof(deviceId), "%02x%02x%02x%02x%02x%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    _beacon.setAddress(consumerId, deviceId);

    // if (_wifi.isConnected()) { // TODO Check if beaconConnection handles disconnects well.
    //     char url[128];
    //     strncpy(url, s.mqttUrl, sizeof(url) - 1);
    //     _beacon.start();
    // }
}

void SateliteOrchestrator::onDisplayChanged(const Settings::Display& s)
{
    for (int i = 0; i < _consumerCount; i++) {
        _consumers[i]->setBrightness(s.brightness[i]);
        // TODO Add alert target handeling. Not implemented because of multi target consumers.
    }
}
