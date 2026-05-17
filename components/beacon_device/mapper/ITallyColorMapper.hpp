#pragma once

#include "types/TallyTypes.hpp"
#include "types/ColorTypes.hpp"

class ITallyColorMapper {
public:
    //TODO add getInstance and make this a singleton.

    virtual ~ITallyColorMapper() = default;

    virtual RGBColor stateToColor(TallyState state) = 0;
};