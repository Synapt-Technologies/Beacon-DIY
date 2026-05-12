#pragma once

#include "consumer/ILutConsumer.hpp"
#include "driver/gpio.h"

class SimpleRGBConsumer : public ILutConsumer {
public:
    SimpleRGBConsumer(gpio_num_t rPin, gpio_num_t gPin, gpio_num_t bPin, DeviceAlertTarget target)
        : _rPin(rPin), _gPin(gPin), _bPin(bPin), _target(target)
    {
        gpio_set_direction(_rPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(_gPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(_bPin, GPIO_MODE_OUTPUT);
    }

    ~SimpleRGBConsumer() {
        gpio_set_direction(_rPin, GPIO_MODE_DISABLE);
        gpio_set_direction(_gPin, GPIO_MODE_DISABLE);
        gpio_set_direction(_bPin, GPIO_MODE_DISABLE);
    }

private:
    gpio_num_t        _rPin, _gPin, _bPin;
    DeviceAlertTarget _target;

    void writeGpio(uint8_t r, uint8_t g, uint8_t b) {
        // Binary GPIO. To add PWM: replace with ledc_set_duty/ledc_update_duty per pin.
        gpio_set_level(_rPin, r > 0 ? 0 : 1);
        gpio_set_level(_gPin, g > 0 ? 0 : 1);
        gpio_set_level(_bPin, b > 0 ? 0 : 1);
    }

    void applyState(TallyState state) override {
        uint8_t r, g, b;
        stateToColor(state, r, g, b);
        writeGpio(scale_brightness(r), scale_brightness(g), scale_brightness(b));
    }

    void setAlertStep(DeviceAlertAction action, DeviceAlertTarget target, uint8_t step) override {
        if (target != _target) return;
        const AlertPattern* pattern = getAlertPattern(action);
        if (!pattern) return;
        // Use variant 1 — basic single-phase animation (variant 0 is always no-flash)
        TallyState state = pattern->patterns[1][step % pattern->patternLen];
        if (state == TallyState::NONE)
            applyState(_state);
        else
            applyState(state);
    }
};
