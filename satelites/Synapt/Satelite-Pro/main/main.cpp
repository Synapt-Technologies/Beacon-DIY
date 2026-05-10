#include <stdio.h>
#include "hub75.h"


#include "platform/Platform.hpp"
#include "config/NvsSettingsStore.hpp"
#include "config/DeviceProfile.hpp"
#include "networkConnection/StaWifiConnection.hpp"
#include "beaconConnection/TcpMqttBeaconConnection.hpp"
#include "httpServer/EspHttpServer.hpp"
#include "consumer/display/Hub75LvglDisplayConsumer.hpp"

#include "orchestrator/SateliteOrchestrator.hpp"

extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(100));

    Platform::init();
    
    ISettingsStore* settingsStore = new NvsSettingsStore();

    INetworkConnection* network = new StaWifiConnection("CYD_Satellite");

    IBeaconConnection* beacon = new TcpMqttBeaconConnection();

    EspHttpServer httpServer = EspHttpServer();


    Hub75Config config = {};
    config.panel_width = 64;
    config.panel_height = 32;

    config.scan_wiring = Hub75ScanWiring::STANDARD_TWO_SCAN;
    config.shift_driver = Hub75ShiftDriver::MBI5124;

    config.latch_blanking = 1;
    config.double_buffer  = false;
    config.brightness     = 255; // TODO: Override setbrightness

    config.pins.r1 = 39;
    config.pins.g1 = 40;
    config.pins.b1 = 4;
    config.pins.r2 = 38;
    config.pins.g2 = 36;
    config.pins.b2 = 2;
    config.pins.a = 14;
    config.pins.b = 13;
    config.pins.c = 10;
    config.pins.d = 8;
    config.pins.e = 6;
    config.pins.lat = 17;
    config.pins.oe = 21;
    config.pins.clk = 34;

    static const IDisplayConsumer::Zone hub75Zones[] = {
        {   0,   0,     64,  32,  1, DeviceAlertTarget::TALENT,    TallyState::NONE, true }, // background (always visible)
    };

    IConsumer* consumer1 = new Hub75LvglDisplayConsumer(config , hub75Zones, 1);

    IConsumer* consumers[] = { consumer1 };


    DeviceProfile profile = DeviceProfile{
        .deviceType = DeviceType::SINGLE_TOPIC,
        .model = "CYD Satellite",
        .consumerCount = 1,
    };

    SateliteOrchestrator orchestrator = SateliteOrchestrator(*settingsStore, profile, *network, *beacon, consumers, profile.consumerCount, httpServer);
    orchestrator.start();

    // Keep app_main alive so stack-allocated runtime objects (orchestrator/http server)
    // remain valid for the lifetime of the firmware.
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}