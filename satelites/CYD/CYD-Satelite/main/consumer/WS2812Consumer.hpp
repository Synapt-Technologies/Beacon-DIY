#pragma once

#include "consumer/IConsumer.hpp"
#include "driver/gpio.h"
#include "led_pattern.h"


class WS2812Consumer : public IConsumer {

public:
    WS2812Consumer(led_strip_handle_t strip, uint8_t count, DeviceAlertTarget target) {

        _target = target;
        _strip = strip;
        _ledCount = count;
    }
    ~WS2812Consumer() {

    }

private:

    DeviceAlertTarget  _target;

    led_strip_handle_t _strip;
    int                _ledCount;

    void applyState(TallyState state) override {
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

    void setColor(uint8_t r, uint8_t g, uint8_t b) {

        const uint8_t sr = scale_brightness(r);
        const uint8_t sg = scale_brightness(g);
        const uint8_t sb = scale_brightness(b);

        for (int i = 0; i < _ledCount; i++)
            led_strip_set_pixel(_strip, i, sr, sg, sb);

        led_strip_refresh(_strip);
    }


    void setAlertStep(DeviceAlertAction action, DeviceAlertTarget target, uint8_t step) override {
        if (target != _target) return; // This consumer only reacts to its configured target

        const AlertPatternConfig* pattern = getAlertPattern(action);
        if (!pattern) return; // No pattern for this action (e.g. CLEAR)

        TallyState state = pattern->pattern[step % pattern->patternLen];
        this->applyState(state);
    }

    struct AlertPatternConfig {
        uint32_t speedMs;
        const TallyState* pattern;
        uint8_t patternLen;
    };

    uint32_t getAlertStepLength(DeviceAlertAction action) override {
        return this->getAlertPattern(action)->speedMs;
    }

    uint8_t getAlertStepCount(DeviceAlertAction action) override {
        return this->getAlertPattern(action)->patternLen;
    }

    // Returns nullptr for CLEAR (no pattern). TallyState::NONE = LED off.
    static const AlertPatternConfig* getAlertPattern(DeviceAlertAction action) {

        static const TallyState IDENT[]  = { TallyState::PREVIEW, TallyState::PROGRAM };
        static const TallyState INFO[]   = { TallyState::PREVIEW, TallyState::NONE, TallyState::NONE, TallyState::NONE };
        static const TallyState NORMAL[] = { TallyState::WARNING, TallyState::NONE };
        static const TallyState PRIO[]   = { TallyState::PROGRAM, TallyState::WARNING };

        static const AlertPatternConfig PATTERNS[] = {
            { 400, IDENT,  2 },
            { 300, INFO,   4 },
            { 400, NORMAL, 2 },
            { 150, PRIO,   2 },
        };

        switch (action) {
            case DeviceAlertAction::IDENT:  return &PATTERNS[0];
            case DeviceAlertAction::INFO:   return &PATTERNS[1];
            case DeviceAlertAction::NORMAL: return &PATTERNS[2];
            case DeviceAlertAction::PRIO:   return &PATTERNS[3];
            default:                        return nullptr;
        }
    }
};
