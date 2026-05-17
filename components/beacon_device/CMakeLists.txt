if(NOT DEFINED CONFIG_BEACON_MAX_FONT_SIZE)
    set(CONFIG_BEACON_MAX_FONT_SIZE 140)
endif()
if(NOT DEFINED CONFIG_BEACON_MIN_FONT_SIZE)
    set(CONFIG_BEACON_MIN_FONT_SIZE 0)
endif()

set(HELVATICA_FONT_SRCS "")
foreach(sz 60 80 100 120 130 140)
    set(_font_file "consumer/display/fonts/helvatica_${sz}.c")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_font_file}"
       AND NOT ${sz} GREATER ${CONFIG_BEACON_MAX_FONT_SIZE}
       AND NOT ${sz} LESS ${CONFIG_BEACON_MIN_FONT_SIZE})
        list(APPEND HELVATICA_FONT_SRCS "${_font_file}")
    endif()
endforeach()

idf_component_register(
    SRCS
        "config/Config.cpp"
        "config/NvsSettingsStore.cpp"
        "consumer/IConsumer.cpp"
        "consumer/IDisplayConsumer.cpp"
        "consumer/display/ILvglDisplayConsumer.cpp"
        "consumer/display/Ili9341LvglDisplayConsumer.cpp"
        "consumer/display/Hub75LvglDisplayConsumer.cpp"
        ${HELVATICA_FONT_SRCS}
        "platform/Platform.cpp"
        "httpServer/EspHttpServer.cpp"
        "httpServer/HttpHandlers.cpp"
        "networkConnection/StaWifiConnection.cpp"
        "orchestrator/IOrchestrator.cpp"
        "orchestrator/SateliteOrchestrator.cpp"
        "beaconConnection/IBeaconConnection.hpp"
        "beaconConnection/TcpMqttBeaconConnection.hpp"
        "config/Config.hpp"
        "config/DeviceProfile.hpp"
        "config/ISettingsStore.hpp"
        "config/NvsSettingsStore.hpp"
        "config/Settings.hpp"
        "consumer/IConsumer.hpp"
        "consumer/IDisplayConsumer.hpp"
        "consumer/SimpleRGBConsumer.hpp"
        "consumer/WS2812Consumer.hpp"
        "consumer/display/ILvglDisplayConsumer.hpp"
        "consumer/display/Ili9341LvglDisplayConsumer.hpp"
        "consumer/display/Hub75LvglDisplayConsumer.hpp"
        "platform/Platform.hpp"
        "httpServer/EspHttpServer.hpp"
        "httpServer/HttpHandlers.hpp"
        "networkConnection/INetworkConnection.hpp"
        "networkConnection/IEthernetConnection.hpp"
        "networkConnection/IWifiConnection.hpp"
        "networkConnection/StaWifiConnection.hpp"
        "mapper/ITallyColorMapper.hpp"
        "mapper/FixedTallyColorMapper.hpp"
        "orchestrator/IOrchestrator.hpp"
        "orchestrator/NodeOrchestrator.hpp"
        "orchestrator/SateliteOrchestrator.hpp"
        "types/TallyTypes.hpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio
        esp_driver_ledc
        esp_driver_spi
        esp_lcd
        led_strip
        mqtt
        lvgl__lvgl
        espressif__esp_lcd_ili9341
        espressif__esp_lvgl_port
        esp_event
        # TODO: Move to PRIV_REQUIRES
        nvs_flash
        esp_http_server
        esp_wifi
        esp_netif
        cjson
        esp-hub75
    # PRIV_REQUIRES
    #     nvs_flash
    #     esp_http_server
    #     esp_wifi
    #     esp_netif
    #     cjson
    EMBED_TXTFILES
        "ui/index.html"
        "ui/ui.css"
        "ui/ui.js"
)

foreach(sz 60 80 100 120 130 140)
    set(_font_file "consumer/display/fonts/helvatica_${sz}.c")
    if("${_font_file}" IN_LIST HELVATICA_FONT_SRCS)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC HELVATICA_FONT_${sz}=1)
    endif()
endforeach()
