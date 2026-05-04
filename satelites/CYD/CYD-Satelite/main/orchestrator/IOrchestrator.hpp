#pragma once

#include "config/Config.hpp"
#include "config/ISettingsStore.hpp"
#include "config/DeviceProfile.hpp"
#include "networkConnection/INetworkConnection.hpp"
#include "beaconConnection/IBeaconConnection.hpp"
#include "consumer/IConsumer.hpp"
#include "httpServer/EspHttpServer.hpp"
#include <cstring>

class IOrchestrator {
public:
    static constexpr uint8_t MAX_CONSUMERS = 8;
    
    IOrchestrator(  ISettingsStore&      store,
                    const DeviceProfile& profile,
                    INetworkConnection&  network,
                    IBeaconConnection&   beacon,
                    IConsumer**          consumers,
                    uint8_t              consumerCount,
                    EspHttpServer&       http
                )
        , _profile(profile)
        , _network(network)
        , _beacon(beacon)
        , _http(http)
    {
        _config = new Config(store);
        
        memset(_consumers, 0, sizeof(_consumers));
        memcpy(_consumers, consumers, consumerCount * sizeof(IConsumer*));
    }

    virtual ~IOrchestrator() = default;
    // TODO Deconstructor.

    virtual void start() = 0;
    virtual void stop()  = 0;

protected:
    Config               _config;
    const DeviceProfile& _profile;
    INetworkConnection&  _network;
    IBeaconConnection&   _beacon;
    IConsumer*           _consumers[MAX_CONSUMERS];
    EspHttpServer&       _http;
};
