#pragma once

#include "types/TallyTypes.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

class IConsumer {
public:
    virtual ~IConsumer() = default;

    // Sets all LEDs regardless of target
    virtual void setState(const TallyState state) {
        this->_state = state;
        this->applyState(state);
    }

    virtual void setAlert(DeviceAlertAction action, DeviceAlertTarget target, uint32_t timeout) {
        if (action == DeviceAlertAction::CLEAR) {
            this->stopAlertTask(target);
        } else {
            this->startAlertTask(action, target, timeout);
        }
    };

    virtual void setBrightness(uint8_t brightness) {
        this->_brightness = brightness;
        this->applyState(this->_state);
    }

protected:
    uint8_t _brightness = 255;
    TallyState _state = TallyState::NONE;

    TaskHandle_t _alertTask = {}; 

    uint8_t scale_brightness(uint8_t value) const
    {
        return static_cast<uint8_t>((static_cast<uint16_t>(value) * _brightness) / 255u);
    }


    virtual void applyState(TallyState state) = 0; // Sets the state without saving it to _state (used for patterns that override the state color)


    struct AlertTaskArg {
        DeviceAlertAction action;
        DeviceAlertTarget target;
        uint32_t timeout;
    };

    void alertTask(void* arg) {
        auto* state = static_cast<AlertTaskArg*>(arg);
        
        const bool infinite_timeout = (state->timeout == 0);
        TickType_t parsed_timeout = pdMS_TO_TICKS(state->timeout);
        TickType_t start_time = xTaskGetTickCount();

        uint8_t step = 0;

        uint32_t step_length = this->getAlertStepLength(state->action); // ms
        uint8_t step_count = this->getAlertStepCount(state->action); 

        while (xTaskGetTickCount() < start_time + parsed_timeout || infinite_timeout) {
            this->setAlertStep(state->action, state->target, step++);

            if (step >= step_count) step = 0;

            TickType_t parsed_delay = 
                (xTaskGetTickCount() + step_length > start_time + parsed_timeout) && !infinite_timeout ?
                (start_time + parsed_timeout - xTaskGetTickCount()) :
                step_length;
            vTaskDelay(parsed_delay);
        }

        this->applyState(this->_state);
    }

    virtual uint32_t getAlertStepLength(DeviceAlertAction action) = 0;
    virtual uint8_t getAlertStepCount(DeviceAlertAction action) = 0;
    virtual void setAlertStep(DeviceAlertAction action, DeviceAlertTarget target, uint8_t step) = 0;

    void startAlertTask(DeviceAlertAction action, DeviceAlertTarget target, uint32_t timeout) {
        
        auto* arg       = new AlertTaskArg;
        arg->action     = action;
        arg->target     = target;
        arg->timeout    = timeout;

        xTaskCreate(alertTask, "led_pat", 2048, arg, 18, &h);
    };

    void stopAlertTask(DeviceAlertTarget target) {

        if (!_alertTask) return;

        xTaskNotifyGive(_alertTask);
        vTaskDelay(pdMS_TO_TICKS(20));
        _alertTask = nullptr;
        this->applyState(this->_state);

    };



};

class ISmartConsumer : public IConsumer {
public:
    virtual ~ISmartConsumer() = default;

    virtual void setText(const char* text, const uint8_t index, const uint32_t timeout = 0) = 0;
};