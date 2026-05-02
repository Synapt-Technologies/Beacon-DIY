#pragma once
#include "interfaces.h"
#include "mqtt_client.h"

class MqttManager : public IMqttManager {
public:
    MqttManager() = default;
    ~MqttManager() override;

    void start(const char* url, const char* topic, MessageCb cb) override;
    void stop()                                                   override;
    bool isConnected()                                      const override;

private:
    esp_mqtt_client_handle_t m_client    = nullptr;
    MessageCb                m_cb;
    char                     m_topic[128] = {};
    bool                     m_connected  = false;

    static void eventHandler(void* arg, esp_event_base_t base,
                             int32_t id, void* data);
    void        onEvent(esp_mqtt_event_handle_t event);
};
