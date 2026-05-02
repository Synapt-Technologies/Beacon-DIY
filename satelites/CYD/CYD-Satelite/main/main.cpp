#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "led_strip.h"

#include "config.h"
#include "led_controller.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "web_server.h"
#include "beacon_app.h"

// ── Pin / hardware constants ──────────────────────────────────────────────────

#define FIX_LED_R_GPIO          GPIO_NUM_4
#define FIX_LED_G_GPIO          GPIO_NUM_16
#define FIX_LED_B_GPIO          GPIO_NUM_17
#define ADD_LED_STRIP_GPIO      22
#define ADD_LED_STRIP_LED_NUMBER 21

static led_strip_handle_t createLedStrip()
{
    led_strip_config_t stripCfg = {
        ADD_LED_STRIP_GPIO,
        ADD_LED_STRIP_LED_NUMBER,
        LED_MODEL_WS2812,
        LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        { false },
    };
    led_strip_spi_config_t spiCfg = {
        SPI_CLK_SRC_DEFAULT,
        SPI2_HOST,
        { true },
    };
    led_strip_handle_t strip;
    ESP_ERROR_CHECK(led_strip_new_spi_device(&stripCfg, &spiCfg, &strip));
    return strip;
}

// ── Composition root ──────────────────────────────────────────────────────────

extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(100));

    // Erase and reinit NVS if the partition is full or the layout changed
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // All objects on heap — WifiManager alone is ~1.4 KB due to wifi_ap_record_t[16]
    auto* config = new NvsConfig();
    config->load();

    led_strip_handle_t strip = createLedStrip();
    auto* leds = new CompositeLedController(strip,
                                            FIX_LED_R_GPIO, FIX_LED_G_GPIO, FIX_LED_B_GPIO,
                                            ADD_LED_STRIP_LED_NUMBER,
                                            config->get().led_brightness);
    auto* wifi = new WifiManager(config->get());
    auto* mqtt = new MqttManager();
    auto* web  = new WebServer(*config, *wifi, *mqtt);
    auto* app  = new BeaconApp(*leds, *config, *wifi, *mqtt, *web);

    app->run(); // spawns tasks then deletes the main task
}
