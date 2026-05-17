#pragma once

#include "consumer/ILutConsumer.hpp"
#include "driver/gpio.h"

class SimpleRGBConsumer : public ILutConsumer {
public:
    // TODO add variant config.
    SimpleRGBConsumer(ITallyColorMapper& colorMapper, gpio_num_t rPin, gpio_num_t gPin, gpio_num_t bPin, DeviceAlertTarget target, uint8_t alertVariant = 1)
        : ILutConsumer(colorMapper), _rPin(rPin), _gPin(gPin), _bPin(bPin), _target(target), _alertVariant(alertVariant)
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
    uint8_t           _alertVariant;

    void writeGpio(uint8_t r, uint8_t g, uint8_t b) {
        // Binary GPIO. To add PWM: replace with ledc_set_duty/ledc_update_duty per pin.
        gpio_set_level(_rPin, r > 0 ? 0 : 1);
        gpio_set_level(_gPin, g > 0 ? 0 : 1);
        gpio_set_level(_bPin, b > 0 ? 0 : 1);
    }

    void applyState(TallyState state) override {
        RGBColor color = _colorMapper.stateToColor(state);
        writeGpio(scale_brightness(color.r), scale_brightness(color.g), scale_brightness(color.b));
    }

    void setAlertStep(DeviceAlertTarget target, const TallyState* step_variants, uint8_t variantCount) override {
        if (!doesTargetMatch(_target, target)) return;
        TallyState state = step_variants[_alertVariant % variantCount];
        if (state == TallyState::NONE)
            applyState(_state);
        else
            applyState(state);
    }
};
