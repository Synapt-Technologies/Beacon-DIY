#include "config/NvsSettingsStore.hpp"
#include "nvs.h"
#include "esp_log.h"

static constexpr char TAG[] = "NvsSettingsStore";

bool NvsSettingsStore::load(Settings& out)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "No saved settings, using defaults");
        return true;
    }

    auto str = [&](const char* key, char* buf, size_t len) {
        nvs_get_str(h, key, buf, &len);
    };

    str("ssid",        out.network.ssid,       sizeof(out.network.ssid));
    str("password",    out.network.password,    sizeof(out.network.password));
    str("mqttUrl",     out.beacon.mqttUrl,      sizeof(out.beacon.mqttUrl));
    str("consumerId",  out.beacon.consumerId,   sizeof(out.beacon.consumerId));
    str("deviceId",    out.beacon.deviceId,     sizeof(out.beacon.deviceId));
    str("deviceName",  out.deviceName,          sizeof(out.deviceName));
    nvs_get_u8(h, "brightness", &out.display.brightness);

    nvs_close(h);
    ESP_LOGI(TAG, "Loaded: ssid='%s' url='%s' consumer='%s'",
             out.network.ssid, out.beacon.mqttUrl, out.beacon.consumerId);
    return true;
}

bool NvsSettingsStore::save(const Settings& in)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return false;

    esp_err_t err = ESP_OK;
    if (err == ESP_OK) err = nvs_set_str(h, "ssid",       in.network.ssid);
    if (err == ESP_OK) err = nvs_set_str(h, "password",   in.network.password);
    if (err == ESP_OK) err = nvs_set_str(h, "mqttUrl",    in.beacon.mqttUrl);
    if (err == ESP_OK) err = nvs_set_str(h, "consumerId", in.beacon.consumerId);
    if (err == ESP_OK) err = nvs_set_str(h, "deviceId",   in.beacon.deviceId);
    if (err == ESP_OK) err = nvs_set_str(h, "deviceName", in.deviceName);
    if (err == ESP_OK) err = nvs_set_u8 (h, "brightness", in.display.brightness);
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err == ESP_OK;
}
