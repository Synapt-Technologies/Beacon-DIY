#include <functional>
#include <stdint.h>
#include "esp_netif_types.h"
#include "types/TallyTypes.hpp"

class IBeaconConnection {
public:
    using TallyCb = std::function<void(TallyState state)>; // TODO change to parse actual usefull mqtt package and make raw package protected.
    using AlertCb = std::function<void(DeviceAlertAction action, DeviceAlertTarget target, uint32_t timeout)>;
    virtual ~IMqttManager() = default;

    
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const {
        return (xTaskGetTickCount() - _lastKeepAlive) < pdMS_TO_TICKS(_aliveTimeout) && _connected;
    };

    virtual void setConsumerAddress(const char* consumer = nullptr) {
        if (consumer) {
            _consumer = consumer;
        } else {
            _consumer = "aedes";
        }
    };
    virtual void setDeviceAddress(const char* device = nullptr) {
        _device = device;
    }

    virtual void setTallyCallback(TallyCb cb) = 0;
    virtual void setAlertCallback(AlertCb cb) = 0;

    virtual void getConsumerAddress(char* out, int len) {
        if (_consumer) {
            strncpy(out, _consumer, len);
            out[len - 1] = '\0';
        } else {
            out[0] = '\0';
        }

        updateSubscription();
    }
    virtual void getDeviceAddress(char* out, int len) {
        if (_device) {
            strncpy(out, _device, len);
            out[len - 1] = '\0'; // TODO check if right
        } else {
            out[0] = '\0';
        }

        updateSubscription();
    }

protected:
    TallyCb _tallyCb.
    AlertCb _alertCb;
    char* _consumer = "aedes";
    char* _device = nullptr;

    uint32_t _aliveTimeout = 2000; // ms

    TickType_t _lastKeepAlive = 0;

    virtual void updateSubscriptions() = 0;
    virtual void clearSubscriptions() = 0;

    virtual void onAlert(const char* data, int len) {
        if (_alertCb) _alertCb(data, len);
    }
}