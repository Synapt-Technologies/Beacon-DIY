#include "networkConnection/StaWifiConnection.hpp"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include <cstring>
#include <cstdio>

// ── INetworkConnection ────────────────────────────────────────────────────────

void StaWifiConnection::start()
{
    _staNetif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t initCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&initCfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    eventHandler, this);
    esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, eventHandler, this);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    applyStaConfig();
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_ps(WIFI_PS_NONE);

    wifi_scan_config_t scanCfg = {};
    esp_wifi_scan_start(&scanCfg, false);

    ESP_LOGI(TAG, "Started");
}

void StaWifiConnection::stop()
{
    if (!_staNetif) return;

    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,    eventHandler);
    esp_event_handler_unregister(IP_EVENT,   IP_EVENT_STA_GOT_IP, eventHandler);

    esp_wifi_stop();
    esp_wifi_deinit();

    esp_netif_destroy(_staNetif); _staNetif = nullptr;
    if (_apNetif) { esp_netif_destroy(_apNetif); _apNetif = nullptr; }
    _apActive = false;

    _ip     = {};
    _status = NetworkStatus::DISCONNECTED;
    fireCallback(NetworkStatus::DISCONNECTED);
}

NetworkStatus StaWifiConnection::getStatus() const
{
    return _status;
}

esp_ip4_addr_t StaWifiConnection::getIp() const
{
    return _ip;
}

void StaWifiConnection::setConnectionCallback(ConnectionCb cb)
{
    _cb = cb;
}

// ── IWifiConnection — STA ─────────────────────────────────────────────────────

void StaWifiConnection::configure(const char* ssid, const char* password)
{
    strncpy(_ssid, ssid     ? ssid     : "", sizeof(_ssid) - 1);
    strncpy(_pass, password ? password : "", sizeof(_pass) - 1);
    _ssid[sizeof(_ssid) - 1] = '\0';
    _pass[sizeof(_pass) - 1] = '\0';

    if (_staNetif) applyStaConfig(); // update driver in-place; orchestrator triggers reconnect
}

int8_t StaWifiConnection::getRssi() const
{
    int8_t rssi = 0;
    esp_wifi_sta_get_rssi(&rssi);
    return rssi;
}

void StaWifiConnection::triggerScan()
{
    wifi_scan_config_t cfg = {};
    esp_wifi_scan_start(&cfg, false);
}

int StaWifiConnection::getScanResults(WifiScanResult* out, int maxCount)
{
    int count = _scanCount < maxCount ? _scanCount : maxCount;
    memcpy(out, _scanResults, count * sizeof(WifiScanResult));
    return count;
}

// ── IWifiConnection — AP ──────────────────────────────────────────────────────

void StaWifiConnection::startAp(const char* namePrefix, const char* password)
{
    if (!_apNetif)
        _apNetif = esp_netif_create_default_wifi_ap();

    char ssid[32] = {};
    buildApSsid(ssid, sizeof(ssid), namePrefix);

    wifi_config_t apCfg = {};
    strlcpy((char*)apCfg.ap.ssid, ssid, sizeof(apCfg.ap.ssid));
    apCfg.ap.ssid_len       = static_cast<uint8_t>(strlen(ssid));
    apCfg.ap.max_connection = 4;

    if (password && password[0]) {
        strlcpy((char*)apCfg.ap.password, password, sizeof(apCfg.ap.password));
        apCfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        apCfg.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apCfg));

    esp_netif_ip_info_t ipInfo = {};
    esp_netif_get_ip_info(_apNetif, &ipInfo);
    _apIp    = ipInfo.ip;
    _apActive = true;

    ESP_LOGI(TAG, "AP started: %s", ssid);
}

void StaWifiConnection::stopAp()
{
    if (!_apActive) return;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    if (_apNetif) { esp_netif_destroy(_apNetif); _apNetif = nullptr; }
    _apActive = false;
    _apIp     = {};

    ESP_LOGI(TAG, "AP stopped");
}

bool StaWifiConnection::isApActive() const
{
    return _apActive;
}

esp_ip4_addr_t StaWifiConnection::getApIp() const
{
    return _apIp;
}

// ── Private ───────────────────────────────────────────────────────────────────

void StaWifiConnection::eventHandler(void* arg, esp_event_base_t base,
                                     int32_t id, void* data)
{
    static_cast<StaWifiConnection*>(arg)->onEvent(base, id, data);
}

void StaWifiConnection::onEvent(esp_event_base_t base, int32_t id, void* data)
{
    if (base == WIFI_EVENT) {
        switch (id) {
        case WIFI_EVENT_STA_START:
            if (_ssid[0]) {
                esp_wifi_connect();
                _status = NetworkStatus::CONNECTING;
                fireCallback(NetworkStatus::CONNECTING);
            }
            break;

        case WIFI_EVENT_STA_DISCONNECTED: {
            _ip     = {};
            _status = NetworkStatus::DISCONNECTED;
            fireCallback(NetworkStatus::DISCONNECTED);

            auto* e = static_cast<wifi_event_sta_disconnected_t*>(data);
            ESP_LOGW(TAG, "Disconnected reason=%d", e->reason);

            if (_ssid[0]) {
                esp_wifi_connect();
                _status = NetworkStatus::CONNECTING;
                fireCallback(NetworkStatus::CONNECTING);
            }
            break;
        }

        case WIFI_EVENT_SCAN_DONE: {
            uint16_t count = SCAN_MAX;
            wifi_ap_record_t raw[SCAN_MAX];
            esp_wifi_scan_get_ap_records(&count, raw);
            _scanCount = count;
            for (int i = 0; i < count; i++) {
                strncpy(_scanResults[i].ssid, (char*)raw[i].ssid, 32);
                _scanResults[i].ssid[32] = '\0';
                memcpy(_scanResults[i].bssid, raw[i].bssid, 6);
                _scanResults[i].channel  = raw[i].primary;
                _scanResults[i].rssi     = raw[i].rssi;
                _scanResults[i].authmode = raw[i].authmode;
            }
            ESP_LOGI(TAG, "Scan done: %d APs", count);
            break;
        }

        default: break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto* e = static_cast<ip_event_got_ip_t*>(data);
        _ip     = e->ip_info.ip;
        _status = NetworkStatus::CONNECTED;
        fireCallback(NetworkStatus::CONNECTED);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&_ip));
    }
}

void StaWifiConnection::fireCallback(NetworkStatus status)
{
    if (_cb) _cb(status, _ip);
}

void StaWifiConnection::buildApSsid(char* out, size_t len, const char* namePrefix)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(out, len, "%s-%02X%02X%02X", namePrefix, mac[3], mac[4], mac[5]);
}

void StaWifiConnection::applyStaConfig()
{
    wifi_config_t staCfg = {};
    strlcpy((char*)staCfg.sta.ssid,     _ssid, sizeof(staCfg.sta.ssid));
    strlcpy((char*)staCfg.sta.password, _pass, sizeof(staCfg.sta.password));
    staCfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &staCfg));
}
