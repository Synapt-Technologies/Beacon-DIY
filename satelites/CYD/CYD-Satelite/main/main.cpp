#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "mqtt_client.h"

#define WIFI_SSID               "WIFI-SSID"
#define WIFI_PASSWORD           "PASSWORD"
#define BROKER_URL              "mqtt://192.168.10.142"
#define BEACON_TOPIC            "tally/device/gpio/4"

#define FIX_LED_R_GPIO          GPIO_NUM_4
#define FIX_LED_G_GPIO          GPIO_NUM_16
#define FIX_LED_B_GPIO          GPIO_NUM_17
#define ADD_LED_STRIP_GPIO      22
#define ADD_LED_STRIP_LED_NUMBER 21

#define WIFI_CONNECTED_BIT      BIT0

static const char *TAG = "example";
static led_strip_handle_t s_led_strip = NULL;
static EventGroupHandle_t s_wifi_event_group;

// ── LED helpers ──────────────────────────────────────────────────────────────

static led_strip_handle_t configure_led(void)
{
    led_strip_config_t strip_config = {
        ADD_LED_STRIP_GPIO,
        ADD_LED_STRIP_LED_NUMBER,
        LED_MODEL_WS2812,
        LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        { false },
    };
    led_strip_spi_config_t spi_config = {
        SPI_CLK_SRC_DEFAULT,
        SPI2_HOST,
        { true },
    };
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    return led_strip;
}

static void set_led(uint8_t r, uint8_t g, uint8_t b)
{
    // Fixed RGB LED is active-low
    gpio_set_level(FIX_LED_R_GPIO, r == 0 ? 1 : 0);
    gpio_set_level(FIX_LED_G_GPIO, g == 0 ? 1 : 0);
    gpio_set_level(FIX_LED_B_GPIO, b == 0 ? 1 : 0);

    if (s_led_strip) {
        if (r == 0 && g == 0 && b == 0) {
            led_strip_clear(s_led_strip);
        } else {
            for (int i = 0; i < ADD_LED_STRIP_LED_NUMBER; i++)
                led_strip_set_pixel(s_led_strip, i, r, g, b);
            led_strip_refresh(s_led_strip);
        }
    }
}

// ── Commented-out LED cycle tasks ────────────────────────────────────────────

// void add_led_cycle(void *arg) {
//     led_strip_handle_t led_strip = configure_led();
//     while (1) {
//         for (int i = 0; i < ADD_LED_STRIP_LED_NUMBER; i++) led_strip_set_pixel(led_strip, i, 255, 0, 0);
//         led_strip_refresh(led_strip); vTaskDelay(pdMS_TO_TICKS(500));
//         led_strip_clear(led_strip);   vTaskDelay(pdMS_TO_TICKS(500));
//         for (int i = 0; i < ADD_LED_STRIP_LED_NUMBER; i++) led_strip_set_pixel(led_strip, i, 0, 255, 0);
//         led_strip_refresh(led_strip); vTaskDelay(pdMS_TO_TICKS(500));
//         led_strip_clear(led_strip);   vTaskDelay(pdMS_TO_TICKS(500));
//         for (int i = 0; i < ADD_LED_STRIP_LED_NUMBER; i++) led_strip_set_pixel(led_strip, i, 0, 0, 255);
//         led_strip_refresh(led_strip); vTaskDelay(pdMS_TO_TICKS(500));
//         led_strip_clear(led_strip);   vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

// void fix_led_cycle(void *arg) {
//     gpio_set_direction(FIX_LED_R_GPIO, GPIO_MODE_OUTPUT);
//     gpio_set_direction(FIX_LED_G_GPIO, GPIO_MODE_OUTPUT);
//     gpio_set_direction(FIX_LED_B_GPIO, GPIO_MODE_OUTPUT);
//     while (1) {
//         gpio_set_level(FIX_LED_R_GPIO, 0); gpio_set_level(FIX_LED_G_GPIO, 1); gpio_set_level(FIX_LED_B_GPIO, 1); vTaskDelay(pdMS_TO_TICKS(1000));
//         gpio_set_level(FIX_LED_R_GPIO, 1); gpio_set_level(FIX_LED_G_GPIO, 0); gpio_set_level(FIX_LED_B_GPIO, 1); vTaskDelay(pdMS_TO_TICKS(1000));
//         gpio_set_level(FIX_LED_R_GPIO, 1); gpio_set_level(FIX_LED_G_GPIO, 1); gpio_set_level(FIX_LED_B_GPIO, 0); vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

// ── WiFi ─────────────────────────────────────────────────────────────────────

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *d = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "WiFi disconnected, reason: %d, reconnecting...", d->reason);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_config = {};
    strlcpy((char *)wifi_config.sta.ssid,     WIFI_SSID,     sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_ps(WIFI_PS_NONE);

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

// ── MQTT ─────────────────────────────────────────────────────────────────────

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected, subscribing to %s", BEACON_TOPIC);
        esp_mqtt_client_subscribe(client, BEACON_TOPIC, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        break;

    case MQTT_EVENT_DATA: {
        // Extract value of "state" key: find `"state":"VALUE"`
        char state[32] = {};
        const char *key = "\"state\":\"";
        const char *p = NULL;
        for (int i = 0; i <= event->data_len - 9; i++) {
            if (strncmp(event->data + i, key, 9) == 0) { p = event->data + i + 9; break; }
        }
        if (p) {
            int i = 0;
            while (p + i < event->data + event->data_len && p[i] != '"' && i < 31)
                state[i] = p[i], i++;
        }
        if (state[0] == '\0') { ESP_LOGW(TAG, "no state in beacon payload"); break; }
        ESP_LOGI(TAG, "beacon state: %s", state);
        if      (strcmp(state, "PROGRAM") == 0) set_led(255, 0,   0);
        else if (strcmp(state, "PREVIEW") == 0) set_led(0,   255, 0);
        else if (strcmp(state, "WARNING") == 0) set_led(255, 255, 0);
        else if (strcmp(state, "NONE")    == 0) set_led(0,   0,   0);
        else ESP_LOGW(TAG, "unknown state: %s", state);
        break;
    }

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;

    default:
        break;
    }
}

static void beacon_mqtt_task(void *arg)
{
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = BROKER_URL;
    mqtt_cfg.task.priority = 18; // match lwIP task priority so we're scheduled immediately on receive
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    // MQTT is event-driven; this task's work is done
    vTaskDelete(NULL);
}

// ── Entry point ───────────────────────────────────────────────────────────────

extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    // Init fixed RGB LED pins
    gpio_set_direction(FIX_LED_R_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(FIX_LED_G_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(FIX_LED_B_GPIO, GPIO_MODE_OUTPUT);
    set_led(0, 0, 0); // all off

    // Init LED strip
    s_led_strip = configure_led();

    // xTaskCreate(add_led_cycle, "add_led_cycle", 4096, NULL, 5, NULL);
    // xTaskCreate(fix_led_cycle, "fix_led_cycle", 4096, NULL, 5, NULL);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init();

    xTaskCreate(beacon_mqtt_task, "beacon_mqtt", 4096, NULL, 5, NULL);
}
