#include "config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "NvsConfig";

NvsConfig::NvsConfig()
{
    // Reset to defaults
    m_cfg = DeviceConfig{};
}

bool NvsConfig::load()
{
    readStr(NS, "wifi_ssid",   m_cfg.wifi_ssid,    sizeof(m_cfg.wifi_ssid));
    readStr(NS, "wifi_pass",   m_cfg.wifi_pass,    sizeof(m_cfg.wifi_pass));
    readStr(NS, "mqtt_url",    m_cfg.mqtt_url,     sizeof(m_cfg.mqtt_url));
    readStr(NS, "mqtt_topic",  m_cfg.mqtt_topic,   sizeof(m_cfg.mqtt_topic));
    readStr(NS, "device_name", m_cfg.device_name,  sizeof(m_cfg.device_name));
    readU8 (NS, "brightness",  m_cfg.led_brightness);
    ESP_LOGI(TAG, "Loaded: ssid='%s' url='%s' topic='%s' name='%s' brightness=%d",
             m_cfg.wifi_ssid, m_cfg.mqtt_url, m_cfg.mqtt_topic,
             m_cfg.device_name, m_cfg.led_brightness);
    return true;
}

bool NvsConfig::save()
{
    bool ok = true;
    ok &= writeStr(NS, "wifi_ssid",   m_cfg.wifi_ssid);
    ok &= writeStr(NS, "wifi_pass",   m_cfg.wifi_pass);
    ok &= writeStr(NS, "mqtt_url",    m_cfg.mqtt_url);
    ok &= writeStr(NS, "mqtt_topic",  m_cfg.mqtt_topic);
    ok &= writeStr(NS, "device_name", m_cfg.device_name);
    ok &= writeU8 (NS, "brightness",  m_cfg.led_brightness);
    return ok;
}

const DeviceConfig& NvsConfig::get() const { return m_cfg; }
void NvsConfig::set(const DeviceConfig& cfg) { m_cfg = cfg; }

// ── helpers ──────────────────────────────────────────────────────────────────

bool NvsConfig::readStr(const char* ns, const char* key, char* out, size_t maxLen)
{
    nvs_handle_t h;
    if (nvs_open(ns, NVS_READONLY, &h) != ESP_OK) return false;
    size_t len = maxLen;
    esp_err_t err = nvs_get_str(h, key, out, &len);
    nvs_close(h);
    return err == ESP_OK;
}

bool NvsConfig::writeStr(const char* ns, const char* key, const char* val)
{
    nvs_handle_t h;
    if (nvs_open(ns, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t err = nvs_set_str(h, key, val);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}

bool NvsConfig::readU8(const char* ns, const char* key, uint8_t& out)
{
    nvs_handle_t h;
    if (nvs_open(ns, NVS_READONLY, &h) != ESP_OK) return false;
    esp_err_t err = nvs_get_u8(h, key, &out);
    nvs_close(h);
    return err == ESP_OK;
}

bool NvsConfig::writeU8(const char* ns, const char* key, uint8_t val)
{
    nvs_handle_t h;
    if (nvs_open(ns, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t err = nvs_set_u8(h, key, val);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}
