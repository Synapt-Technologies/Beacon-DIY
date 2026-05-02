#include "beaconConnection/IBeaconConnection.h"



class TcpMqttBeaconConnection : public IBeaconConnection {
public:
    TcpMqttBeaconConnection();
    ~TcpMqttBeaconConnection() {
        this->stop();
    };

    void start() override {
        if (_client)
            this->stop();

        esp_mqtt_client_config_t cfg = {};
        cfg.broker.address.uri = url;
        cfg.task.priority      = 18;

        m_client = esp_mqtt_client_init(&cfg);
        esp_mqtt_client_register_event(m_client,
                                    (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                    eventHandler, this);
        esp_mqtt_client_start(m_client);
        ESP_LOGI(TAG, "Connecting to %s", url);

        this->updateSubscriptions();
    }
    void stop() override {

        if (!_client) return;
        esp_mqtt_client_stop(_client);
        esp_mqtt_client_destroy(_client);
        _client    = nullptr;
        _connected = false;

        this->clearSubscriptions();
    }

    void setTallyCallback(MessageCb cb) override;
    void setAlertCallback(MessageCb cb) override;

private:
    esp_mqtt_client_handle_t _client = nullptr;
    bool _connected = false;
    Subscription _infoSub;
    Subscription _tallySub;
    Subscription _alertSub;

    void updateSubscriptions() override {
        if (!_client) return;

        char tallyTopic[128] = "tally/device/" + this->_device + "/" + this->_consumer;
        char alertTopic[128] = "tally/device/" + this->_device + "/" + this->_consumer + "/alert";
        char infoTopic[128] = "system/info";

        strlcpy(_infoSub.topic, tallyTopic, sizeof(_tallySub.topic));
        _tallySub.cb = _tallyCb;
        esp_mqtt_client_subscribe(_client, _tallySub.topic, 0);
        ESP_LOGI(TAG, "Subscribed to %s", _tallySub.topic);

        if (_tallyCb) {
            strlcpy(_tallySub.topic, tallyTopic, sizeof(_tallySub.topic));
            _tallySub.cb = [this](auto d, auto l){ onGlobalInfo(d, l); };
            esp_mqtt_client_subscribe(_client, _tallySub.topic, 0);
            ESP_LOGI(TAG, "Subscribed to %s", _tallySub.topic);
        }

        if (_alertCb) {
            strlcpy(_alertSub.topic, alertTopic, sizeof(_alertSub.topic));
            _alertSub.cb = _alertCb;
            esp_mqtt_client_subscribe(_client, _alertSub.topic, 0);
            ESP_LOGI(TAG, "Subscribed to %s", _alertSub.topic);
        }
    }

     void clearSubscriptions() override {
        if (!_client) return;

        if (_tallySub.topic[0] != '\0') {
            esp_mqtt_client_unsubscribe(_client, _tallySub.topic);
            _tallySub = {};
            ESP_LOGI(TAG, "Unsubscribed from tally topic");
        }

        if (_alertSub.topic[0] != '\0') {
            esp_mqtt_client_unsubscribe(_client, _alertSub.topic);
            _alertSub = {};
            ESP_LOGI(TAG, "Unsubscribed from alert topic");
        }
    }

    
    void onGlobalInfo(const char* data, int len) { // TODO add Beacon info storage
        _lastKeepAlive = xTaskGetTickCount();
    }

    void onTally(const char* data, int len) {
        if (!_tallyCb) 
            return;
            
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

        ESP_LOGI(TAG, "Tally ss=%d", ss);

        _tallyCb(static_cast<TallyState>(ss));
    }

    void onAlert(const char* data, int len) {
        if (!_alertCb)
            return;
                
        int type   = extractInt(data, len, "type",   -1);
        int target = extractInt(data, len, "target", -1);
        int time   = extractInt(data, len, "time",   -1);

        // DeviceAlertTarget: OPERATOR=0, TALENT=1, ALL=2  →  LedTarget: OPERATOR=1, TALENT=2, ALL=3
        ESP_LOGI(TAG, "Alert type=%d target=%d time=%d", type, target, time);

        _alertCb(static_cast<DeviceAlertAction>(type), static_cast<DeviceAlertTarget>(target), time);
    }
};