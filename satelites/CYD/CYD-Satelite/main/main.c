#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"

#define LED_STRIP_GPIO  22
#define LED_STRIP_LED_NUMBER 28

static const char *TAG = "example";

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBER,    // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,      // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // Pixel format of your LED strip
        .flags.invert_out = false,          // whether to invert the output signal
    };

    // LED strip backend configuration: SPI
    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .flags.with_dma = true,         // Using DMA can improve performance and help drive more LEDs
        .spi_bus = SPI2_HOST,           // SPI bus ID
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with SPI backend");
    return led_strip;
}

void app_main(void)
{
    led_strip_handle_t led_strip = configure_led();

    ESP_LOGI(TAG, "Start blinking LED strip");
    while (1) {
        for (int i = 0; i < LED_STRIP_LED_NUMBER; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 0, 0));
        }
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI(TAG, "LED ON!");

        vTaskDelay(pdMS_TO_TICKS(500));

        
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        ESP_LOGI(TAG, "LED OFF!");

        vTaskDelay(pdMS_TO_TICKS(500));
        
        for (int i = 0; i < LED_STRIP_LED_NUMBER; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 255, 0));
        }
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI(TAG, "LED ON!");

        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        ESP_LOGI(TAG, "LED OFF!");

        vTaskDelay(pdMS_TO_TICKS(500));
        
        for (int i = 0; i < LED_STRIP_LED_NUMBER; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 0, 255));
        }
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI(TAG, "LED ON!");

        vTaskDelay(pdMS_TO_TICKS(500));

        
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        ESP_LOGI(TAG, "LED OFF!");

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}