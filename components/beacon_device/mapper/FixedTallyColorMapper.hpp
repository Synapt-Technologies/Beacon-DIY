#pragma once

// TODO: Rename to default, add fixed version that allows color setting in the constructor, and a version that uses the config.
class FixedTallyColorMapper : public ITallyColorMapper {
public:
    RGBColor stateToColor(TallyState state) override {
        switch (state) {
            case TallyState::DANGER:  return {120, 50,   50};
            case TallyState::WARNING: return {255, 255,   0};
            case TallyState::INFO:    return {  0,   0, 255};
            case TallyState::PREVIEW: return {  0, 255,   0};
            case TallyState::PROGRAM: return {255,   0,   0};
            default:                  return {  0,   0,   0};
        }
    }
};