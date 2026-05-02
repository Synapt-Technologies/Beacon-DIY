#pragma once

#include "types/TallyTypes.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

        uint32_t step_size = this->getAlertStepSize(state->action); // ms

        while (xTaskGetTickCount() < start_time + parsed_timeout || infinite_timeout) {
            this->setAlertStep(state->action, state->target, step++);

            TickType_t parsed_delay = 
                (xTaskGetTickCount() + step_size > start_time + parsed_timeout) && !infinite_timeout ?
                (start_time + parsed_timeout - xTaskGetTickCount()) :
                step_size;
            vTaskDelay(parsed_delay);
        }
    }

    virtual uint32_t getAlertStepSize(DeviceAlertAction action) = 0;
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
    };



};

class ISmartConsumer : public IConsumer {
public:
    virtual ~ISmartConsumer() = default;

    virtual void setText(const char* text, const uint8_t index, const uint32_t timeout = 0) = 0;
};