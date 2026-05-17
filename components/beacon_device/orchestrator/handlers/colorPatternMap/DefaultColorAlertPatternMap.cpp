#include "orchestrator/handlers/colorPatternMap/DefaultColorAlertPatternMap.hpp"

const TallyState DefaultColorAlertPatternMap::IDENT[][8] = {
    { TallyState::NONE,    TallyState::NONE,    TallyState::NONE,    TallyState::NONE    },
    { TallyState::PREVIEW, TallyState::PROGRAM, TallyState::PREVIEW, TallyState::PROGRAM },
    { TallyState::PROGRAM, TallyState::PREVIEW, TallyState::PROGRAM, TallyState::PREVIEW },
    { TallyState::PROGRAM, TallyState::NONE,    TallyState::PREVIEW, TallyState::NONE    },
    { TallyState::NONE,    TallyState::PROGRAM, TallyState::NONE,    TallyState::PREVIEW },
};
const TallyState DefaultColorAlertPatternMap::INFO[][8] = {
    { TallyState::NONE, TallyState::NONE, TallyState::NONE, TallyState::NONE },
    { TallyState::INFO, TallyState::NONE, TallyState::INFO, TallyState::NONE },
    { TallyState::INFO, TallyState::NONE, TallyState::INFO, TallyState::NONE },
};
const TallyState DefaultColorAlertPatternMap::NORMAL[][8] = {
    { TallyState::NONE,    TallyState::NONE,    TallyState::NONE,    TallyState::NONE },
    { TallyState::WARNING, TallyState::NONE,    TallyState::WARNING, TallyState::NONE },
    { TallyState::WARNING, TallyState::NONE,    TallyState::WARNING, TallyState::NONE },
};
const TallyState DefaultColorAlertPatternMap::PRIO[][8] = {
    { TallyState::NONE,    TallyState::NONE,    TallyState::NONE,    TallyState::NONE    },
    { TallyState::PROGRAM, TallyState::WARNING, TallyState::PROGRAM, TallyState::WARNING },
    { TallyState::WARNING, TallyState::PROGRAM, TallyState::WARNING, TallyState::PROGRAM },
    { TallyState::PROGRAM, TallyState::NONE,    TallyState::WARNING, TallyState::NONE    },
    { TallyState::NONE,    TallyState::PROGRAM, TallyState::NONE,    TallyState::WARNING },
};
const ColorAlertPattern DefaultColorAlertPatternMap::PATTERNS[] = {
    { 400, IDENT,  5, 4 },
    { 300, INFO,   3, 4 },
    { 400, NORMAL, 3, 4 },
    { 150, PRIO,   5, 4 },
};

const ColorAlertPattern* DefaultColorAlertPatternMap::getPattern(DeviceAlertAction action) const {
    switch (action) {
        case DeviceAlertAction::IDENT:  return &PATTERNS[0];
        case DeviceAlertAction::INFO:   return &PATTERNS[1];
        case DeviceAlertAction::NORMAL: return &PATTERNS[2];
        case DeviceAlertAction::PRIO:   return &PATTERNS[3];
        default:                        return nullptr;
    }
}
