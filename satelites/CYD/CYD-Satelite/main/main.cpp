#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "led_strip.h"

#define FIX_LED_R_GPIO GPIO_NUM_4
#define FIX_LED_G_GPIO GPIO_NUM_16
#define FIX_LED_B_GPIO GPIO_NUM_17

#define ADD_LED_STRIP_GPIO  22
#define ADD_LED_STRIP_LED_NUMBER 21

static const char *TAG = "example";

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        ADD_LED_STRIP_GPIO,                     // The GPIO that connected to the LED strip's data line
        ADD_LED_STRIP_LED_NUMBER,               // The number of LEDs in the strip,
        LED_MODEL_WS2812,                   // LED strip model
        LED_STRIP_COLOR_COMPONENT_FMT_GRB,  // Pixel format of your LED strip
        { false },                          // whether to invert the output signal
    };

    // LED strip backend configuration: SPI
    led_strip_spi_config_t spi_config = {
        SPI_CLK_SRC_DEFAULT,  // different clock source can lead to different power consumption
        SPI2_HOST,            // SPI bus ID
        { true },             // Using DMA can improve performance and help drive more LEDs
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with SPI backend");
    return led_strip;
}



void add_led_cycle(void*arg) {
led_strip_handle_t led_strip = configure_led();

    ESP_LOGI(TAG, "Start blinking LED strip");
    while (1) {
        for (int i = 0; i < ADD_LED_STRIP_LED_NUMBER; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 0, 0));
        }
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI(TAG, "LED ON!");

        vTaskDelay(pdMS_TO_TICKS(500));

        
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        ESP_LOGI(TAG, "LED OFF!");

        vTaskDelay(pdMS_TO_TICKS(500));
        
        for (int i = 0; i < ADD_LED_STRIP_LED_NUMBER; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 255, 0));
        }
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI(TAG, "LED ON!");

        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        ESP_LOGI(TAG, "LED OFF!");

        vTaskDelay(pdMS_TO_TICKS(500));
        
        for (int i = 0; i < ADD_LED_STRIP_LED_NUMBER; i++) {
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

void fix_led_cycle(void*arg) {

    gpio_set_direction(FIX_LED_R_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(FIX_LED_G_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(FIX_LED_B_GPIO, GPIO_MODE_OUTPUT);
    while(1) {

        gpio_set_level(FIX_LED_R_GPIO, 0);
        gpio_set_level(FIX_LED_G_GPIO, 1);
        gpio_set_level(FIX_LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(FIX_LED_R_GPIO, 1);
        gpio_set_level(FIX_LED_G_GPIO, 0);
        gpio_set_level(FIX_LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(FIX_LED_R_GPIO, 1);
        gpio_set_level(FIX_LED_G_GPIO, 1);
        gpio_set_level(FIX_LED_B_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


extern "C" void app_main(void)
{
    //Allow other core to finish initialization
    vTaskDelay(pdMS_TO_TICKS(100));

    xTaskCreate(add_led_cycle, "add_led_cycle", 4096, NULL, 5, NULL);
    xTaskCreate(fix_led_cycle, "fix_led_cycle", 4096, NULL, 5, NULL);
    
}