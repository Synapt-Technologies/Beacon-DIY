#pragma once

#include "consumer/IConsumer.hpp"
#include "driver/gpio.h"
#include "led_pattern.h"


struct StripSection { // Todo: Brightness section? -> Then the way they are iterated would change..
    uint8_t             startLed;
    uint8_t             alertPattern;
    DeviceAlertTarget   target;
};

class WS2812Consumer : public IConsumer {

public:
    WS2812Consumer(led_strip_handle_t strip, uint8_t ledCount, StripSection sections[], uint8_t sectionCount, DeviceAlertTarget target) {

        _target = target;
        _strip = strip;
        _ledCount = ledCount;
        _sections = sections;
        _sectionCount = sectionCount;
    }
    ~WS2812Consumer() {

    }

private:

    DeviceAlertTarget  _target;

    led_strip_handle_t _strip;
    int                _ledCount;

    StripSection* _sections;
    uint8_t _sectionCount;


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
        // const TallyState (*pattern)[5];
        const TallyState *pattern;
        uint8_t patternLen;
    };

    uint32_t getAlertStepLength(DeviceAlertAction action) override {
        return this->getAlertPattern(action)->speedMs;
    }

    uint8_t getAlertStepCount(DeviceAlertAction action) override {
        return this->getAlertPattern(action)->patternLen;
    }

    // Returns nullptr for CLEAR (no pattern). TallyState::NONE = LED off.
    static const AlertPatternConfig* getAlertPattern(DeviceAlertAction action, uint8_t index = 0) {

        static const TallyState IDENT[][5]  = {
            { TallyState::NONE,     TallyState::NONE,       TallyState::NONE,       TallyState::NONE },
            { TallyState::PREVIEW,  TallyState::PROGRAM,    TallyState::PREVIEW,    TallyState::PROGRAM },
            { TallyState::PROGRAM,  TallyState::PREVIEW,    TallyState::PROGRAM,    TallyState::PREVIEW },
            { TallyState::PROGRAM,  TallyState::NONE,       TallyState::PREVIEW,    TallyState::NONE },
            { TallyState::NONE,     TallyState::PROGRAM,       TallyState::NONE,    TallyState::PREVIEW },
        };
        static const TallyState INFO[][5]   = { 
            { TallyState::NONE,     TallyState::NONE,       TallyState::NONE,       TallyState::NONE },
            { TallyState::INFO,     TallyState::NONE,       TallyState::INFO,       TallyState::NONE },
            { TallyState::NONE,     TallyState::INFO,       TallyState::NONE,       TallyState::INFO },
            { TallyState::INFO,     TallyState::NONE,       TallyState::INFO,       TallyState::NONE },
            { TallyState::NONE,     TallyState::INFO,       TallyState::NONE,       TallyState::INFO },
        };
        static const TallyState NORMAL[][5] = { 
            { TallyState::NONE,     TallyState::NONE,       TallyState::NONE,       TallyState::NONE },
            { TallyState::WARNING,  TallyState::NONE,       TallyState::WARNING,    TallyState::NONE },
            { TallyState::NONE,     TallyState::WARNING,    TallyState::NONE,       TallyState::WARNING },

        };
        static const TallyState PRIO[][5]   = { 
            { TallyState::NONE,     TallyState::NONE,       TallyState::NONE,       TallyState::NONE },
            { TallyState::PROGRAM,  TallyState::WARNING,    TallyState::PROGRAM,    TallyState::WARNING },
            { TallyState::WARNING,  TallyState::PROGRAM,    TallyState::WARNING,    TallyState::PROGRAM },
            { TallyState::PROGRAM,  TallyState::NONE,       TallyState::WARNING,    TallyState::NONE },
            { TallyState::NONE,     TallyState::PROGRAM,    TallyState::NONE,       TallyState::WARNING },
        };

        static const AlertPatternConfig PATTERNS[] = {
            { 400, IDENT[index],  2 },
            { 300, INFO[index],   4 },
            { 400, NORMAL[index], 2 },
            { 150, PRIO[index],   2 },
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
