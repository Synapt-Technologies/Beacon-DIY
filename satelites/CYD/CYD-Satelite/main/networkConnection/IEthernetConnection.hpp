#pragma once

#include "networkConnection/INetworkConnection.hpp"
#include <stdint.h>

class IEthernetConnection : public INetworkConnection {
public:
    virtual ~IEthernetConnection() = default;

    virtual void getMac(uint8_t out[6]) const = 0;
};
