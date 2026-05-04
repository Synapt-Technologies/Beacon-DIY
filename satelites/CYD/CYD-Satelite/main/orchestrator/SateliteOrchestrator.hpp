#pragma once

#include "orchestrator/IOrchestrator.hpp"

// TODO: implement multi-topic orchestrator.
// Each consumer has its own MQTT subscription and displays an independent tally state.
class SateliteOrchestrator : public IOrchestrator {
public:
    SateliteOrchestrator(ISettingsStore&      store,
                                const DeviceProfile& profile,
                                INetworkConnection&  network,
                                IBeaconConnection&   beacon,
                                IConsumer**          consumers,
                                uint8_t              consumerCount,
                                EspHttpServer&       http) 
                                : IOrchestrator(store, profile, network, beacon, consumers, consumerCount, http){}
    

    void start() override;
    void stop() override;

private:
    static constexpr char TAG[]        = "SateliteOrch";

};
