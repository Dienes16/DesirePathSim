#ifndef DESIREPATHS_HPP
#define DESIREPATHS_HPP

#include <cstdint>

#include "Map.hpp"
#include "CostMap.hpp"

using DesirePathsMap = Map<std::uint8_t>;

std::uint8_t adjustDesirePathStress(const std::size_t tileX, const std::size_t tileY, DesirePathsMap& desirePathsMap, CostMap& baseCostMap, const int adjustment);

void decayDesirePaths(DesirePathsMap& desirePathsMap, CostMap& baseCostMap);

#endif // DESIREPATHS_HPP
