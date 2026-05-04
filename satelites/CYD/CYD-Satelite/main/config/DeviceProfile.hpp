#pragma once

#include <stdint.h>

enum class DeviceType : uint8_t {
    SINGLE_TOPIC, // one MQTT subscription → all consumers show the same tally state
    MULTI_TOPIC,  // one MQTT subscription per consumer → each shows its own tally state
};

struct DeviceProfile {
    DeviceType  deviceType    = DeviceType::SINGLE_TOPIC;
    char        model[32]     = "Beacon Satellite";
};
