#pragma once

#include "types/TallyTypes.hpp"
#include "types/ColorTypes.hpp"

class ITallyColorMapper {
public:
    virtual ~ITallyColorMapper() = default;

    virtual RGBColor stateToColor(TallyState state) = 0;
};