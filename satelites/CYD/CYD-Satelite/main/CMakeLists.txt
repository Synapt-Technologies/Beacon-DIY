idf_component_register(
    SRCS
        "main.cpp"
        "config.cpp"
        "led_controller.cpp"
        "led_layout.cpp"
        "wifi_manager.cpp"
        "mqtt_manager.cpp"
        "web_server.cpp"
        "beacon_app.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio led_strip
        mqtt esp_netif esp_wifi nvs_flash
        esp_http_server
)
