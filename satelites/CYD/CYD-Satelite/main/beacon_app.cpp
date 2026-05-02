#include "beacon_app.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_pattern.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char* TAG = "BeaconApp";

BeaconApp::BeaconApp(ILedController& leds,
                     IConfig&        config,
                     IWifiManager&   wifi,
                     IMqttManager&   mqtt,
                     WebServer&      web)
    : m_leds(leds), m_config(config), m_wifi(wifi), m_mqtt(mqtt), m_web(web) {}

void BeaconApp::run()
{
    m_wifi.start();
    m_web.start();

    xTaskCreate(mqttTask, "beacon_mqtt", 4096, this, 5, nullptr);
    vTaskDelete(nullptr);
}

// ── MQTT task ─────────────────────────────────────────────────────────────────

void BeaconApp::mqttTask(void* arg)
{
    auto* self = static_cast<BeaconApp*>(arg);

    self->m_wifi.waitForConnection();

    DeviceConfig cfg = self->m_config.get();

    // Auto-populate device_id from WiFi STA MAC if blank
    if (cfg.device_id[0] == '\0') {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        snprintf(cfg.device_id, sizeof(cfg.device_id),
                 "%02x%02x%02x%02x%02x%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        self->m_config.set(cfg);
        self->m_config.save();
        ESP_LOGI(TAG, "Auto device_id: %s", cfg.device_id);
    }

    if (cfg.mqtt_url[0] == '\0') {
        ESP_LOGW(TAG, "MQTT URL not configured — skipping");
        vTaskDelete(nullptr);
        return;
    }

    // Build topics
    char tallyTopic[140], alertTopic[155];
    snprintf(tallyTopic, sizeof(tallyTopic), "tally/device/%s/%s",
             cfg.consumer_id, cfg.device_id);
    snprintf(alertTopic, sizeof(alertTopic), "%s/alert", tallyTopic);

    ESP_LOGI(TAG, "Tally topic: %s", tallyTopic);

    self->m_mqtt.start(cfg.mqtt_url);
    self->m_mqtt.subscribe(tallyTopic,    [self](auto d, auto l){ self->onTally(d, l); });
    self->m_mqtt.subscribe(alertTopic,    [self](auto d, auto l){ self->onAlert(d, l); });
    self->m_mqtt.subscribe("system/info", [self](auto d, auto l){ self->onSystemInfo(d, l); });

    vTaskDelete(nullptr);
}

// ── Message handlers ──────────────────────────────────────────────────────────

void BeaconApp::onTally(const char* data, int len)
{
    // Prefer numeric ss field (DeviceTallyState); fall back to string state
    int ss = extractInt(data, len, "ss", -1);
    if (ss < 0) {
        // Parse state string → numeric equivalent
        char state[16] = {};
        const char* key = "\"state\":\"";
        for (int i = 0; i <= len - 9; i++) {
            if (strncmp(data + i, key, 9) == 0) {
                const char* p = data + i + 9;
                int j = 0;
                while (p + j < data + len && p[j] != '"' && j < 15)
                    state[j] = p[j], j++;
                break;
            }
        }
        if      (strcmp(state, "PROGRAM") == 0) ss = 7;
        else if (strcmp(state, "DANGER")  == 0) ss = 2;
        else if (strcmp(state, "PREVIEW") == 0) ss = 4;
        else if (strcmp(state, "WARNING") == 0) ss = 1;
        else                                    ss = 0;
    }

    Color c = stateColor(ss);
    m_tallyColor = c;
    m_leds.updatePatternTallyColor(c.r, c.g, c.b);
    m_leds.setColor(c.r, c.g, c.b);
    ESP_LOGI(TAG, "Tally ss=%d → rgb(%d,%d,%d)", ss, c.r, c.g, c.b);
}

void BeaconApp::onAlert(const char* data, int len)
{
    int type   = extractInt(data, len, "type",   -1);
    int target = extractInt(data, len, "target", -1);

    // DeviceAlertTarget: OPERATOR=0, TALENT=1, ALL=2  →  LedTarget: OPERATOR=1, TALENT=2, ALL=3
    LedTarget ledTarget = alertTarget(target);

    ESP_LOGI(TAG, "Alert type=%d target=%d", type, target);

    switch (type) {
    case 0: // IDENT — finite white strobe
        m_leds.runPatternForTarget(ledTarget,
                                   PATTERN_IDENT, countof(PATTERN_IDENT),
                                   false,
                                   m_tallyColor.r, m_tallyColor.g, m_tallyColor.b);
        break;
    case 1: // INFO — slow blue pulse, repeating
        m_leds.runPatternForTarget(ledTarget,
                                   PATTERN_INFO, countof(PATTERN_INFO),
                                   true,
                                   m_tallyColor.r, m_tallyColor.g, m_tallyColor.b);
        break;
    case 3: // PRIO — tally color flash, repeating
        m_leds.runPatternForTarget(ledTarget,
                                   PATTERN_PRIO, countof(PATTERN_PRIO),
                                   true,
                                   m_tallyColor.r, m_tallyColor.g, m_tallyColor.b);
        break;
    case 2: // NORMAL
    case 4: // CLEAR
        m_leds.stopPattern(ledTarget);
        break;
    default:
        ESP_LOGW(TAG, "Unknown alert type %d", type);
        break;
    }
}

void BeaconApp::onSystemInfo(const char* /*data*/, int /*len*/)
{
    m_beaconOnline = true;
    m_web.setBeaconOnline(true);
    // Heartbeat is not retained; if we stop receiving it the MQTT disconnect
    // event will propagate through isConnected() anyway.
}

// ── Static helpers ────────────────────────────────────────────────────────────

BeaconApp::Color BeaconApp::stateColor(int ss)
{
    switch (ss) {
    case 7: return {255,   0,   0}; // PROGRAM
    case 2: return {180,   0,   0}; // DANGER
    case 4: return {  0, 255,   0}; // PREVIEW
    case 1: return {255, 255,   0}; // WARNING
    default:return {  0,   0,   0}; // NONE
    }
}

LedTarget BeaconApp::alertTarget(int target)
{
    switch (target) {
    case 0: return LedTarget::OPERATOR;
    case 1: return LedTarget::TALENT;
    case 2: return LedTarget::ALL;
    default:return LedTarget::ALL;
    }
}

int BeaconApp::extractInt(const char* data, int len,
                           const char* key, int fallback)
{
    char search[32];
    snprintf(search, sizeof(search), "\"%s\":", key);
    int searchLen = strlen(search);
    for (int i = 0; i <= len - searchLen; i++) {
        if (strncmp(data + i, search, searchLen) == 0)
            return atoi(data + i + searchLen);
    }
    return fallback;
}
