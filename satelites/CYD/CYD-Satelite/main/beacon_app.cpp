#include "beacon_app.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "BeaconApp";

BeaconApp::BeaconApp(ILedController& leds,
                     IConfig&        config,
                     WifiManager&    wifi,
                     IMqttManager&   mqtt,
                     WebServer&      web)
    : m_leds(leds), m_config(config), m_wifi(wifi), m_mqtt(mqtt), m_web(web) {}

void BeaconApp::run()
{
    m_wifi.start();
    m_web.start();

    // Start the MQTT task — it blocks until WiFi is ready, then stays alive
    xTaskCreate(mqttTask, "beacon_mqtt", 4096, this, 5, nullptr);

    // run() never returns; main task suspends
    vTaskSuspend(nullptr);
}

// ── MQTT task ─────────────────────────────────────────────────────────────────

void BeaconApp::mqttTask(void* arg)
{
    auto* self = static_cast<BeaconApp*>(arg);

    // Wait until STA is connected before starting MQTT
    xEventGroupWaitBits(self->m_wifi.eventGroup(),
                        WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    const DeviceConfig& cfg = self->m_config.get();
    if (cfg.mqtt_url[0] == '\0' || cfg.mqtt_topic[0] == '\0') {
        ESP_LOGW(TAG, "MQTT not configured — skipping");
        vTaskDelete(nullptr);
        return;
    }

    self->m_mqtt.start(cfg.mqtt_url, cfg.mqtt_topic,
        [self](const char* data, int len) {
            self->onMessage(data, len);
        });

    vTaskDelete(nullptr);
}

// ── message handling ──────────────────────────────────────────────────────────

void BeaconApp::onMessage(const char* data, int len)
{
    // Extract value of "state" key from JSON: find `"state":"VALUE"`
    char state[32] = {};
    const char* key = "\"state\":\"";
    const char* p   = nullptr;
    for (int i = 0; i <= len - 9; i++) {
        if (strncmp(data + i, key, 9) == 0) { p = data + i + 9; break; }
    }
    if (p) {
        int i = 0;
        while (p + i < data + len && p[i] != '"' && i < 31)
            state[i] = p[i], i++;
    }
    if (state[0] == '\0') {
        ESP_LOGW(TAG, "No state field in payload");
        return;
    }
    applyState(state);
}

void BeaconApp::applyState(const char* state)
{
    ESP_LOGI(TAG, "State: %s", state);
    if      (strcmp(state, "PROGRAM") == 0) m_leds.setColor(255, 0,   0);
    else if (strcmp(state, "PREVIEW") == 0) m_leds.setColor(0,   255, 0);
    else if (strcmp(state, "WARNING") == 0) m_leds.setColor(255, 255, 0);
    else if (strcmp(state, "NONE")    == 0) m_leds.setColor(0,   0,   0);
    else ESP_LOGW(TAG, "Unknown state: %s", state);
}
