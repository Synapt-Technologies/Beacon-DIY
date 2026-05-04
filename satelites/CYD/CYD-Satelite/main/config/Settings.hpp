#pragma once

#include <stdint.h>

struct Settings {

    struct Network {
        char ssid[64]     = {};
        char password[64] = {};
    } network;

    struct Beacon {
        char mqttUrl[128]   = {};
        char consumerId[32] = "aedes";
        char deviceId[48]   = {}; // empty = auto-derive from MAC at runtime
    } beacon;

    struct Display {
        uint8_t brightness[5] = {255, 255, 255, 255, 255};
        // LED layout will be added when the LED system is rewritten
    } display;

    char deviceName[32] = "CYD Satellite";
};
