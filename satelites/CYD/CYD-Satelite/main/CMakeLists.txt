idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES
                        esp_driver_gpio led_strip
                        mqtt esp_netif esp_wifi nvs_flash
                )