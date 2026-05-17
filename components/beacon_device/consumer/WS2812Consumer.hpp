#pragma once

#include "led_strip.h"
#include "consumer/ILutConsumer.hpp"
#include "freertos/semphr.h"

struct StripSection {
    uint8_t           startLed;
    uint8_t           alertVariant;
    DeviceAlertTarget target;
    TallyState        minState     = TallyState::NONE;  // section off when _state < minState
    bool              stateColored = true;              // false = section dark during normal tally
};

class WS2812Consumer : public ILutConsumer {
public:
    WS2812Consumer(ITallyColorMapper& colorMapper, led_strip_handle_t strip, uint8_t ledCount, StripSection sections[], uint8_t sectionCount) : ILutConsumer(colorMapper) {
        _strip        = strip;
        _ledCount     = ledCount;
        _sections     = sections;
        _sectionCount = sectionCount;
        _mutex        = xSemaphoreCreateMutex();
    }
    ~WS2812Consumer() {
        if (_mutex) vSemaphoreDelete(_mutex);
    }

private:
    led_strip_handle_t _strip;
    int                _ledCount;
    SemaphoreHandle_t  _mutex;
    StripSection*      _sections;
    uint8_t            _sectionCount;

    uint8_t sectionCount(uint8_t s) const {
        return (s + 1 < _sectionCount)
            ? _sections[s + 1].startLed - _sections[s].startLed
            : static_cast<uint8_t>(_ledCount) - _sections[s].startLed;
    }

    void setColorRange(uint8_t start, uint8_t count, uint8_t r, uint8_t g, uint8_t b) {
        const uint8_t sr = scale_brightness(r);
        const uint8_t sg = scale_brightness(g);
        const uint8_t sb = scale_brightness(b);
        for (uint8_t i = start; i < start + count; i++)
            led_strip_set_pixel(_strip, i, sr, sg, sb);
    }

    //TODO Implement better zones. Probably setColorRange -> setZoneColor?
    void applyState(TallyState state) override {
        RGBColor color = _colorMapper.stateToColor(state);

        xSemaphoreTake(_mutex, portMAX_DELAY);
        for (uint8_t s = 0; s < _sectionCount; s++) {
            const StripSection& sec = _sections[s];
            const bool show = sec.stateColored && (state >= sec.minState);
            
            setColorRange(sec.startLed, sectionCount(s),
                      show ? color.r : 0,
                      show ? color.g : 0,
                      show ? color.b : 0);
        }
        led_strip_refresh(_strip);
        xSemaphoreGive(_mutex);
    }

    void setAlertStep(DeviceAlertTarget target, const TallyState* step_variants, uint8_t variantCount) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        for (uint8_t s = 0; s < _sectionCount; s++) {
            const StripSection& sec = _sections[s];
            if (!doesTargetMatch(sec.target, target)) continue;

            const uint8_t start      = sec.startLed;
            const uint8_t count      = sectionCount(s);
            const uint8_t variantIdx = sec.alertVariant % variantCount;
            const TallyState state   = step_variants[variantIdx];

            if (state == TallyState::NONE) {
                if (sec.stateColored && _state >= sec.minState) {
                    RGBColor color = _colorMapper.stateToColor(_state);
                    setColorRange(start, count, color.r, color.g, color.b);
                } else {
                    setColorRange(start, count, 0, 0, 0);
                }
            } else {
                RGBColor color = _colorMapper.stateToColor(state);
                setColorRange(start, count, color.r, color.g, color.b);
            }
        }
        led_strip_refresh(_strip);
        xSemaphoreGive(_mutex);
    }
};
