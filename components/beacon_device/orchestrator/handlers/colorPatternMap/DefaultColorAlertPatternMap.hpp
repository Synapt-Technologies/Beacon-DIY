#pragma once

#include "orchestrator/handlers/colorPatternMap/IColorAlertPatternMap.hpp"

class DefaultColorAlertPatternMap : public IColorAlertPatternMap {
public:
    virtual ~DefaultColorAlertPatternMap() = default;

    const ColorAlertPattern* getPattern(DeviceAlertAction action) const override;

private:
    static const TallyState IDENT[][8];
    static const TallyState INFO[][8];
    static const TallyState NORMAL[][8];
    static const TallyState PRIO[][8];
    static const ColorAlertPattern PATTERNS[];
};
