#include "DesirePaths.hpp"

#include <algorithm>

std::uint8_t adjustDesirePathStress(const std::size_t tileX, const std::size_t tileY, DesirePathsMap& desirePathsMap, CostMap& baseCostMap, const int adjustment)
{
   const auto valueBefore = desirePathsMap.at(tileX, tileY);
   const auto valueAfter = static_cast<std::uint8_t>(std::clamp(valueBefore + adjustment, 0, 255));

   if (valueBefore != valueAfter)
   {
      desirePathsMap.at(tileX, tileY) = valueAfter;

      const auto baseCostAdjustmentBefore = -(valueBefore / 64);
      const auto baseCostAdjustmentAfter  = -(valueAfter  / 64);

      baseCostMap.at(tileX, tileY) = baseCostMap.at(tileX, tileY) - baseCostAdjustmentBefore + baseCostAdjustmentAfter;
   }

   return valueAfter;
}

void decayDesirePaths(DesirePathsMap& desirePathsMap, CostMap& baseCostMap)
{
   for (std::size_t worldTileY = 0; worldTileY < desirePathsMap.height(); ++worldTileY)
   {
      for (std::size_t worldTileX = 0; worldTileX < desirePathsMap.width(); ++worldTileX)
      {
         adjustDesirePathStress(worldTileX, worldTileY, desirePathsMap, baseCostMap, -2);
      }
   }
}
