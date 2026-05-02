#include "led_controller.h"

CompositeLedController::CompositeLedController(led_strip_handle_t strip,
                                               gpio_num_t rPin,
                                               gpio_num_t gPin,
                                               gpio_num_t bPin,
                                               int ledCount,
                                               uint8_t brightness)
    : m_strip(strip), m_rPin(rPin), m_gPin(gPin), m_bPin(bPin),
      m_brightness(brightness), m_ledCount(ledCount)
{
    gpio_set_direction(m_rPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(m_gPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(m_bPin, GPIO_MODE_OUTPUT);
    setColor(0, 0, 0);
}

void CompositeLedController::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    const uint8_t sr = scale(r);
    const uint8_t sg = scale(g);
    const uint8_t sb = scale(b);

    // Fixed RGB LED — active-low: low = on
    gpio_set_level(m_rPin, sr > 0 ? 0 : 1);
    gpio_set_level(m_gPin, sg > 0 ? 0 : 1);
    gpio_set_level(m_bPin, sb > 0 ? 0 : 1);

    if (sr == 0 && sg == 0 && sb == 0) {
        led_strip_clear(m_strip);
    } else {
        for (int i = 0; i < m_ledCount; i++)
            led_strip_set_pixel(m_strip, i, sr, sg, sb);
        led_strip_refresh(m_strip);
    }
}

void CompositeLedController::setBrightness(uint8_t brightness)
{
    m_brightness = brightness;
}

uint8_t CompositeLedController::scale(uint8_t value) const
{
    return static_cast<uint8_t>((static_cast<uint16_t>(value) * m_brightness) / 255u);
}
