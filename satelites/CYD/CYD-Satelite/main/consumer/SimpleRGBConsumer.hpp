#pragma once

#include "consumer/IConsumer.h"
#include "driver/gpio.h"
#include "led_pattern.h"


class SimpleRGBConsumer : public IConsumer {

    SimpleRGBConsumer(gpio_num_t rPin, gpio_num_t gPin, gpio_num_t bPin, TallyAlertTarget target) {
        _rPin = rPin;
        _gPin = gPin;
        _bPin = bPin;

        _target = target;

        gpio_set_direction(_rPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(_gPin, GPIO_MODE_OUTPUT);
        gpio_set_direction(_bPin, GPIO_MODE_OUTPUT);
    }
    ~SimpleRGBConsumer() {

        gpio_set_direction(_rPin, GPIO_MODE_DEF_DISABLE);
        gpio_set_direction(_gPin, GPIO_MODE_DEF_DISABLE);
        gpio_set_direction(_bPin, GPIO_MODE_DEF_DISABLE);
    }

    void setBrightness(uint8_t brightness) {
        this->_brightness = brightness;
        // TODO PWM
    }

private:

    gpio_num_t _rPin, _gPin, _bPin;
    TallyAlertTarget _target;

    void applyState(TallyState state) {
                switch (state)
        {
            case TallyState::NONE:
                this->setColor(0, 0, 0);
                break;
            case TallyState::DANGER:
            case TallyState::WARNING:
                this->setColor(255, 255, 0);
                break;
            case TallyState::INFO:
                this->setColor(0, 0, 255);
                break;
            case TallyState::PREVIEW:
                this->setColor(0, 255, 0);
                break;
            case TallyState::PROGRAM:
                this->setColor(255, 0, 0);
                break;
            default:
                break;
        }
    }

    void setColor(uint8_t r, uint8_t g, uint8_t b) { // TODO PWM
        gpio_set_level(_rPin, r > 0 ? 0 : 1);
        gpio_set_level(_gPin, g > 0 ? 0 : 1);
        gpio_set_level(_bPin, b > 0 ? 0 : 1);
    }

    uint32_t getAlertStepSize(DeviceAlertAction action) {
        return 200;
    }
    void setAlertStep(DeviceAlertAction action, DeviceAlertTarget target, uint8_t step) {
        ;
    }
}
