#pragma once

#include "beaconConnection/IBeaconConnection.hpp"
#include "mqtt_client.h"
#include "esp_log.h"
#include <cstring>
#include <cstdio>
#include <functional>

class TcpMqttBeaconConnection : public IBeaconConnection {
public:
    TcpMqttBeaconConnection(const char* url) {
        strncpy(_url, url, sizeof(_url) - 1);
    }

    ~TcpMqttBeaconConnection() {
        this->stop();
    }

    void start() override {
        if (_client)
            this->stop();

        esp_mqtt_client_config_t cfg = {};
        cfg.broker.address.uri = _url;
        cfg.task.priority      = 18;

        _client = esp_mqtt_client_init(&cfg);
        esp_mqtt_client_register_event(_client,
                                       (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                       eventHandler, this);
        esp_mqtt_client_start(_client);
        ESP_LOGI(TAG, "Connecting to %s", _url);
    }

    void stop() override {
        if (!_client) return;

        esp_mqtt_client_stop(_client);
        esp_mqtt_client_destroy(_client);
        _client    = nullptr;
        _connected = false;
        _infoSub   = {};
        _tallySub  = {};
        _alertSub  = {};
    }

    void setTallyCallback(TallyCb cb) override { _tallyCb = cb; }
    void setAlertCallback(AlertCb cb) override { _alertCb = cb; }

private:
    static constexpr char TAG[] = "TcpMqtt";

    struct Subscription {
        char topic[128] = {};
        std::function<void(const char*, int)> cb;
    };

    char                     _url[256] = {};
    esp_mqtt_client_handle_t _client   = nullptr;

    // TODO is this struct needed? Topics do not really change, or need to be stored.
    Subscription _infoSub;
    Subscription _tallySub;
    Subscription _alertSub;

    void updateSubscriptions() override {
        if (!_client || !_connected) return;

        clearSubscriptions();

        char tallyTopic[128];
        char alertTopic[128];
        static constexpr char infoTopic[] = "system/info";

        snprintf(tallyTopic, sizeof(tallyTopic), "tally/device/%s/%s", _device, _consumer);
        snprintf(alertTopic, sizeof(alertTopic), "tally/device/%s/%s/alert", _device, _consumer);

        strlcpy(_infoSub.topic, infoTopic, sizeof(_infoSub.topic));
        _infoSub.cb = [this](const char* d, int l) { onGlobalInfo(d, l); };
        esp_mqtt_client_subscribe(_client, _infoSub.topic, 0);
        ESP_LOGI(TAG, "Subscribed to %s", _infoSub.topic);

        if (_tallyCb) {
            strlcpy(_tallySub.topic, tallyTopic, sizeof(_tallySub.topic));
            _tallySub.cb = [this](const char* d, int l) { onTally(d, l); };
            esp_mqtt_client_subscribe(_client, _tallySub.topic, 0);
            ESP_LOGI(TAG, "Subscribed to %s", _tallySub.topic);
        }

        if (_alertCb) {
            strlcpy(_alertSub.topic, alertTopic, sizeof(_alertSub.topic));
            _alertSub.cb = [this](const char* d, int l) { onAlert(d, l); };
            esp_mqtt_client_subscribe(_client, _alertSub.topic, 0);
            ESP_LOGI(TAG, "Subscribed to %s", _alertSub.topic);
        }
    }

    void clearSubscriptions() override {
        if (!_client) return;

        if (_infoSub.topic[0]) {
            esp_mqtt_client_unsubscribe(_client, _infoSub.topic);
            _infoSub = {};
            ESP_LOGI(TAG, "Unsubscribed from info topic");
        }
        if (_tallySub.topic[0]) {
            esp_mqtt_client_unsubscribe(_client, _tallySub.topic);
            _tallySub = {};
            ESP_LOGI(TAG, "Unsubscribed from tally topic");
        }
        if (_alertSub.topic[0]) {
            esp_mqtt_client_unsubscribe(_client, _alertSub.topic);
            _alertSub = {};
            ESP_LOGI(TAG, "Unsubscribed from alert topic");
        }
    }

    static void eventHandler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {
        auto* self  = static_cast<TcpMqttBeaconConnection*>(arg);
        auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);

        switch (event_id) {
            case MQTT_EVENT_CONNECTED:
                self->_connected     = true;
                self->_lastKeepAlive = xTaskGetTickCount();
                self->updateSubscriptions();
                ESP_LOGI(TAG, "Connected");
                break;
            case MQTT_EVENT_DISCONNECTED:
                self->_connected = false;
                ESP_LOGI(TAG, "Disconnected");
                break;
            case MQTT_EVENT_DATA:
                self->handleData(event);
                break;
            default:
                break;
        }
    }

    void handleData(esp_mqtt_event_handle_t event) {
        auto matches = [&](const Subscription& sub) {
            return sub.topic[0] && sub.cb &&
                   static_cast<int>(strlen(sub.topic)) == event->topic_len &&
                   strncmp(event->topic, sub.topic, event->topic_len) == 0;
        };

        if      (matches(_infoSub))  _infoSub.cb(event->data, event->data_len);
        else if (matches(_tallySub)) _tallySub.cb(event->data, event->data_len);
        else if (matches(_alertSub)) _alertSub.cb(event->data, event->data_len);
    }

    void onGlobalInfo(const char* data, int len) {
        _lastKeepAlive = xTaskGetTickCount();
    }

    void onTally(const char* data, int len) {
        if (!_tallyCb)
            return;

        int ss = extractInt(data, len, "ss", -1);
        if (ss < 0) {
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
            if      (strcmp(state, "PROGRAM") == 0) ss = TallyState::PROGRAM;
            else if (strcmp(state, "DANGER")  == 0) ss = TallyState::DANGER;
            else if (strcmp(state, "PREVIEW") == 0) ss = TallyState::PREVIEW;
            else if (strcmp(state, "WARNING") == 0) ss = TallyState::WARNING;
            else                                    ss = TallyState::NONE;
        }

        ESP_LOGI(TAG, "Tally ss=%d", ss);
        _tallyCb(static_cast<TallyState>(ss));
    }

    void onAlert(const char* data, int len) {
        if (!_alertCb)
            return;

        int type   = extractInt(data, len, "type",   -1);
        int target = extractInt(data, len, "target", -1);
        int time   = extractInt(data, len, "time",   -1);

        ESP_LOGI(TAG, "Alert type=%d target=%d time=%d", type, target, time);

        _alertCb(
            static_cast<DeviceAlertAction>(type),
            static_cast<DeviceAlertTarget>(target),
            time >= 0 ? static_cast<uint32_t>(time) : 0
        );
    }

    static int extractInt(const char* data, int len, const char* key, int defaultVal) {
        char search[32];
        snprintf(search, sizeof(search), "\"%s\":", key);
        int keyLen = static_cast<int>(strlen(search));
        for (int i = 0; i <= len - keyLen; i++) {
            if (strncmp(data + i, search, keyLen) == 0) {
                const char* p = data + i + keyLen;
                while (*p == ' ') p++;
                char* end;
                long val = strtol(p, &end, 10);
                if (end != p) return static_cast<int>(val);
            }
        }
        return defaultVal;
    }
};
