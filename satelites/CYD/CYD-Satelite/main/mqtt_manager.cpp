#include "mqtt_manager.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "MqttManager";

MqttManager::~MqttManager() { stop(); }

void MqttManager::start(const char* url, const char* topic, MessageCb cb)
{
    if (m_client) stop();

    strlcpy(m_topic, topic, sizeof(m_topic));
    m_cb = cb;

    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = url;
    cfg.task.priority      = 18;

    m_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(m_client,
                                   (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                   eventHandler, this);
    esp_mqtt_client_start(m_client);
    ESP_LOGI(TAG, "Started. broker=%s topic=%s", url, topic);
}

void MqttManager::stop()
{
    if (!m_client) return;
    esp_mqtt_client_stop(m_client);
    esp_mqtt_client_destroy(m_client);
    m_client    = nullptr;
    m_connected = false;
}

bool MqttManager::isConnected() const { return m_connected; }

// ── private ──────────────────────────────────────────────────────────────────

void MqttManager::eventHandler(void* arg, esp_event_base_t /*base*/,
                                int32_t id, void* data)
{
    static_cast<MqttManager*>(arg)->onEvent(
        static_cast<esp_mqtt_event_handle_t>(data));
    (void)id;
}

void MqttManager::onEvent(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        m_connected = true;
        ESP_LOGI(TAG, "Connected, subscribing to %s", m_topic);
        esp_mqtt_client_subscribe(m_client, m_topic, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        m_connected = false;
        ESP_LOGW(TAG, "Disconnected");
        break;

    case MQTT_EVENT_DATA:
        if (m_cb)
            m_cb(event->data, event->data_len);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Error");
        break;

    default: break;
    }
}
