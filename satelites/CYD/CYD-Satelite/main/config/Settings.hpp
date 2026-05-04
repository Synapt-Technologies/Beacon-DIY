#pragma once

#include <stdint.h>
#include "types/TallyTypes.hpp"

struct Settings {

    struct Network {
        char ssid[64]     = {};
        char password[64] = {};
    } network;

    struct Beacon {
        char mqttUrl[128]      = {};
        char consumerId[8][32] = { "aedes" }; // [0] defaults to "aedes", rest empty
        char deviceId[8][48]   = {};           // empty = auto-derive from MAC at runtime
    } beacon;

    struct Display {
        uint8_t           brightness[8]   = {255, 255, 255, 255, 255, 255, 255, 255};
        DeviceAlertTarget alertTarget[8]  = {
            DeviceAlertTarget::ALL, DeviceAlertTarget::ALL,
            DeviceAlertTarget::ALL, DeviceAlertTarget::ALL,
            DeviceAlertTarget::ALL, DeviceAlertTarget::ALL,
            DeviceAlertTarget::ALL, DeviceAlertTarget::ALL,
        };
    } display;

    char deviceName[32] = "Beacon Satellite";
};
