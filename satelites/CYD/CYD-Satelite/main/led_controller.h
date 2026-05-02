#pragma once
#include "interfaces.h"
#include "driver/gpio.h"
#include "led_strip.h"

class CompositeLedController : public ILedController {
public:
    CompositeLedController(led_strip_handle_t strip,
                           gpio_num_t rPin, gpio_num_t gPin, gpio_num_t bPin,
                           uint8_t brightness = 255);

    void setColor(uint8_t r, uint8_t g, uint8_t b) override;
    void setBrightness(uint8_t brightness);

private:
    led_strip_handle_t m_strip;
    gpio_num_t         m_rPin, m_gPin, m_bPin;
    uint8_t            m_brightness;
    int                m_ledCount;

    uint8_t scale(uint8_t value) const;
};
