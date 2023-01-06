#include "WorldGen.hpp"

#include <stack>

WorldGen::WorldGen(WorldMap& worldMap, std::mt19937_64& rng):
   m_worldMap{worldMap},
   m_rng{rng},
   m_rngScaledPercent{1, 100 * m_rngScaledPercentFactor},
   m_rngWorldMapWidth {0, m_worldMap.width () - 1},
   m_rngWorldMapHeight{0, m_worldMap.height() - 1}
{
   // NOP
}

bool WorldGen::findPattern(const Pattern& pattern, std::size_t& x, std::size_t& y, const std::size_t startX, const std::size_t startY) const
{
   for (std::size_t mapY = startY; mapY < m_worldMap.height() - pattern.height() + 1; ++mapY)
   {
      for (std::size_t mapX = (mapY == startY ? startX : 0); mapX < m_worldMap.width() - pattern.width() + 1; ++mapX)
      {
         bool breakPattern = false;

         for (std::size_t patternY = 0; patternY < pattern.height(); ++patternY)
         {
            for (std::size_t patternX = 0; patternX < pattern.width(); ++patternX)
            {
               if (pattern.at(patternX, patternY) == TileType::PatternAny)
                  continue;

               if (m_worldMap.at(mapX + patternX, mapY + patternY) != pattern.at(patternX, patternY))
               {
                  breakPattern = true;
                  break;
               }
            }

            if (breakPattern)
               break;
         }

         if (breakPattern == false)
         {
            // Pattern found
            x = mapX;
            y = mapY;

            return true;
         }
      }
   }

   return false;
}

void WorldGen::applyPatch(const Patch& patch, const std::size_t x, const std::size_t y)
{
   for (std::size_t patchY = 0; patchY < patch.height(); ++patchY)
   {
      for (std::size_t patchX = 0; patchX < patch.width(); ++patchX)
      {
         if (patch.at(patchX, patchY) == TileType::PatchKeep)
            continue;

         m_worldMap.at(x + patchX, y + patchY) = patch.at(patchX, patchY);
      }
   }
}

// Returns number of replacements performed
std::size_t WorldGen::floodFill(const std::size_t x, const std::size_t y, const TileType fillTileType)
{
   const auto tileTypeToReplace = m_worldMap.at(x, y);

   std::size_t count = 0;

   std::stack<std::pair<std::size_t, std::size_t>> coordStack;

   coordStack.emplace(x, y);

   while (coordStack.empty() == false)
   {
      const auto [currentX, currentY] = coordStack.top();
      coordStack.pop();

      if (m_worldMap.at(currentX, currentY) == tileTypeToReplace)
      {
         m_worldMap.at(currentX, currentY) = fillTileType;

         if (currentY < m_worldMap.height() - 1) coordStack.emplace(currentX    , currentY + 1);
         if (currentY > 0                      ) coordStack.emplace(currentX    , currentY - 1);
         if (currentX < m_worldMap.width () - 1) coordStack.emplace(currentX + 1, currentY    );
         if (currentX > 0                      ) coordStack.emplace(currentX - 1, currentY    );

         count += 1;
      }
   }

   return count;
}

void WorldGen::placeStreetsFromVoronoiMap(const VoronoiMap& voronoiMap)
{
   for (std::size_t centerY = 0; centerY < voronoiMap.height(); ++centerY)
   {
      for (std::size_t centerX = 0; centerX < voronoiMap.width(); ++centerX)
      {
         const auto self = voronoiMap.at(centerX, centerY);

         if (centerX > 0)
         {
            if (centerY > 0)
            {
               if (voronoiMap.at(centerX - 1, centerY - 1) != self)
               {
                  m_worldMap.at(centerX, centerY) = TileType::Street;
                  continue;
               }
            }

            if (centerY < voronoiMap.height() - 1)
            {
               if (voronoiMap.at(centerX - 1, centerY + 1) != self)
               {
                  m_worldMap.at(centerX, centerY) = TileType::Street;
                  continue;
               }
            }

            if (voronoiMap.at(centerX - 1, centerY) != self)
            {
               m_worldMap.at(centerX, centerY) = TileType::Street;
               continue;
            }
         }

         if (centerX < voronoiMap.width() - 1)
         {
            if (centerY > 0)
            {
               if (voronoiMap.at(centerX + 1, centerY - 1) != self)
               {
                  m_worldMap.at(centerX, centerY) = TileType::Street;
                  continue;
               }
            }

            if (centerY < voronoiMap.height() - 1)
            {
               if (voronoiMap.at(centerX + 1, centerY + 1) != self)
               {
                  m_worldMap.at(centerX, centerY) = TileType::Street;
                  continue;
               }
            }

            if (voronoiMap.at(centerX + 1, centerY) != self)
            {
               m_worldMap.at(centerX, centerY) = TileType::Street;
               continue;
            }
         }

         if (centerY > 0)
         {
            if (voronoiMap.at(centerX, centerY - 1) != self)
            {
               m_worldMap.at(centerX, centerY) = TileType::Street;
               continue;
            }
         }

         if (centerY < voronoiMap.height() - 1)
         {
            if (voronoiMap.at(centerX, centerY + 1) != self)
            {
               m_worldMap.at(centerX, centerY) = TileType::Street;
               continue;
            }
         }
      }
   }
}

void WorldGen::placePonds(const float fillLimitInPercent, const float areaSizeLimitInPercent)
{
   const std::size_t totalTileCount = m_worldMap.width() * m_worldMap.height();

   std::size_t waterTileCount = 0;

   for (std::size_t count = 1; count <= 100; ++count)
   {
      const auto startX = m_rngWorldMapWidth (m_rng);
      const auto startY = m_rngWorldMapHeight(m_rng);

      const auto randomGrass = m_worldMap.find(TileType::Grass, startX, startY, true);

      if (randomGrass.has_value() == false)
         break;

      const auto& [x, y] = randomGrass.value();

      const auto filledCount = floodFill(x, y, TileType::Water);

      if ((filledCount * 100.0f / static_cast<float>(totalTileCount)) <= areaSizeLimitInPercent)
      {
         if (((waterTileCount + filledCount) * 100.0f / static_cast<float>(totalTileCount)) <= fillLimitInPercent)
         {
            waterTileCount += filledCount;
            continue;
         }
      }

      // Revert
      floodFill(x, y, TileType::Grass);
   }
}

void WorldGen::placeFullPavedAreas(const float fillLimitInPercent, const float areaSizeLimitInPercent)
{
   const std::size_t totalTileCount = m_worldMap.width() * m_worldMap.height();

   std::size_t filledTileCount = 0;

   for (std::size_t count = 1; count <= 100; ++count)
   {
      const auto startX = m_rngWorldMapWidth (m_rng);
      const auto startY = m_rngWorldMapHeight(m_rng);

      const auto randomGrass = m_worldMap.find(TileType::Grass, startX, startY, true);

      if (randomGrass.has_value() == false)
         break;

      const auto& [x, y] = randomGrass.value();

      const auto filledCount = floodFill(x, y, TileType::Temporary);

      if ((filledCount * 100.0f / static_cast<float>(totalTileCount)) <= areaSizeLimitInPercent)
      {
         if (((filledTileCount + filledCount) * 100.0f / static_cast<float>(totalTileCount)) <= fillLimitInPercent)
         {
            floodFill(x, y, TileType::Street);
            filledTileCount += filledCount;

            continue;
         }
      }

      // Revert
      floodFill(x, y, TileType::Grass);
   }
}

void WorldGen::replaceAll(const TileType replaceWhat, const TileType replaceWith)
{
   for (std::size_t y = 0; y < m_worldMap.height(); ++y)
   {
      for (std::size_t x = 0; x < m_worldMap.width(); ++x)
      {
         if (m_worldMap.at(x, y) == replaceWhat)
         {
            m_worldMap.at(x, y) = replaceWith;
         }
      }
   }
}

void WorldGen::placeBuildings(const int fillRate)
{
   /*
   000000  ->  ......  ->  000000
   000000  ->  .2222.  ->  022220
   000000  ->  .2222.  ->  022220
   000000  ->  .2222.  ->  022220
   000000  ->  .2222.  ->  022220
   ..00..  ->  ..3...  ->  ..30..
   ..11..  ->  ......  ->  ..11..
   ..11..  ->  ......  ->  ..11..
   ..00..  ->  ......  ->  ..00..
   */

   // Variations in size, corner blocks, L shapes and width of entrance paths

   const std::size_t minBuildingWidth = 6;
   const std::size_t maxBuildingWidth = 10;
   const std::size_t minBuildingHeight = 4;
   const std::size_t maxBuildingHeight = 12;

   const std::size_t minEstateWidth = minBuildingWidth + 2;
   const std::size_t maxEstateWidth = maxBuildingWidth + 2;
   const std::size_t minEstateHeight = minBuildingHeight + 5;
   const std::size_t maxEstateHeight = maxBuildingHeight + 5;

   std::vector<Pattern> patterns;
   std::vector<std::vector<Patch>> patches; // Inner array is for variations

   patterns.reserve((maxBuildingWidth - minBuildingWidth) * (maxBuildingHeight - minBuildingHeight));
   patches.reserve(patterns.capacity());

   for (std::size_t estateHeight = minEstateHeight; estateHeight <= maxEstateHeight; ++estateHeight)
   {
      for (std::size_t estateWidth = minEstateWidth; estateWidth <= maxEstateWidth; ++estateWidth)
      {
         Patch pattern{estateWidth, estateHeight, TileType::Grass};

         // Street short
         for (std::size_t x = 0; x < pattern.width(); ++x)
         {
            if (x < 2 || x > pattern.width() - 3)
            {
               pattern.at(x, estateHeight - 4) = TileType::PatternAny;
               pattern.at(x, estateHeight - 3) = TileType::PatternAny;
               pattern.at(x, estateHeight - 2) = TileType::PatternAny;
               pattern.at(x, estateHeight - 1) = TileType::PatternAny;
            }
            else
            {
               pattern.at(x, estateHeight - 3) = TileType::Street;
               pattern.at(x, estateHeight - 2) = TileType::Street;
            }
         }

         patterns.emplace_back(std::move(pattern));

         std::vector<Patch> patchVariations;

         for (std::size_t buildingHeight = minBuildingHeight; buildingHeight <= estateHeight - 5; ++buildingHeight)
         {
            const auto buildingWidth = estateWidth - 2;

            Patch patch{estateWidth, estateHeight, TileType::PatchKeep};

            // Building
            for (std::size_t y = 1; y <= buildingHeight; ++y)
            {
               for (std::size_t x = 1; x <= buildingWidth; ++x)
               {
                  patch.at(x, y) = TileType::Building;
               }
            }

            // Entrance
#if 1
            const bool wideEntrance = (m_rngScaledPercent(m_rng) <= 50 * m_rngScaledPercentFactor);
            const bool fullWidthEntrance = (wideEntrance == true) && (m_rngScaledPercent(m_rng) <= 50 * m_rngScaledPercentFactor);
            const bool fullWidthEntranceSpansEstate = false;
#else
            const bool wideEntrance = false;
            const bool fullWidthEntrance = true;
            const bool fullWidthEntranceSpansEstate = true;
#endif

            for (std::size_t y = 1 + buildingHeight; y < estateHeight - 3; ++y)
            {
               for (std::size_t x = 0; x <= buildingWidth + 1; ++x)
               {
                  if (x == 0 || x == buildingWidth + 1)
                  {
                     if (fullWidthEntrance && fullWidthEntranceSpansEstate)
                     {
                        patch.at(x, y) = TileType::BuildingEntrance;
                     }
                  }
                  else if (x == buildingWidth / 2)
                  {
                     patch.at(x, y) = TileType::BuildingEntrance;
                  }
                  else if (x == buildingWidth / 2 + 1)
                  {
                     if (wideEntrance)
                     {
                        patch.at(x, y) = TileType::BuildingEntrance;
                     }
                  }
                  else
                  {
                     if (fullWidthEntrance)
                     {
                        patch.at(x, y) = TileType::BuildingEntrance;
                     }
                  }
               }
            }

            patchVariations.emplace_back(patch);

            if (buildingWidth >= 5 && buildingHeight >= 5)
            {
               // Create variation without corner blocks
               {
                  auto patchVariation{patch};

                  patchVariation.at(1, 1) = TileType::PatchKeep;
                  patchVariation.at(1, buildingHeight) = TileType::PatchKeep;
                  patchVariation.at(buildingWidth, 1) = TileType::PatchKeep;
                  patchVariation.at(buildingWidth, buildingHeight) = TileType::PatchKeep;

                  patchVariations.emplace_back(std::move(patchVariation));
               }

               // Create L shape variation (top left cut out)
               {
                  auto patchVariation{patch};

                  for (std::size_t y = 1; y <= buildingHeight / 2; ++y)
                  {
                     for (std::size_t x = 1; x <= buildingWidth / 2; ++x)
                     {
                        patchVariation.at(x, y) = TileType::PatchKeep;
                     }
                  }

                  patchVariations.emplace_back(std::move(patchVariation));
               }

               // Create L shape variation (top right cut out)
               {
                  auto patchVariation{patch};

                  for (std::size_t y = 1; y <= buildingHeight / 2; ++y)
                  {
                     for (std::size_t x = buildingWidth / 2; x <= buildingWidth; ++x)
                     {
                        patchVariation.at(x, y) = TileType::PatchKeep;
                     }
                  }

                  patchVariations.emplace_back(std::move(patchVariation));
               }

               // Create L shape variation (bottom left cut out)
               {
                  auto patchVariation{patch};

                  for (std::size_t y = buildingHeight; y > buildingHeight / 2; --y)
                  {
                     for (std::size_t x = 1; x <= buildingWidth / 2; ++x)
                     {
                        if (patchVariation.at(x, y + 1) == TileType::BuildingEntrance)
                        {
                           patchVariation.at(x, y) = TileType::BuildingEntrance;
                        }
                        else
                        {
                           patchVariation.at(x, y) = TileType::PatchKeep;
                        }
                     }
                  }

                  patchVariations.emplace_back(std::move(patchVariation));
               }

               // Create L shape variation (bottom right cut out)
               {
                  auto patchVariation{patch};

                  for (std::size_t y = buildingHeight; y > buildingHeight / 2; --y)
                  {
                     for (std::size_t x = buildingWidth / 2; x <= buildingWidth; ++x)
                     {
                        if (patchVariation.at(x, y + 1) == TileType::BuildingEntrance)
                        {
                           patchVariation.at(x, y) = TileType::BuildingEntrance;
                        }
                        else
                        {
                           patchVariation.at(x, y) = TileType::PatchKeep;
                        }
                     }
                  }

                  patchVariations.emplace_back(std::move(patchVariation));
               }
            }
         }

         patches.emplace_back(std::move(patchVariations));
      }
   }

   auto applyPatches = [this] (const Pattern& pattern, const std::vector<Patch>& patchVariations, const int scaledFillRate)
   {
      std::size_t patternPosX = 0;
      std::size_t patternPosY = 0;

      std::size_t findStartPosX = 0;
      std::size_t findStartPosY = 0;

      for (;;)
      {
         if (findPattern(pattern, patternPosX, patternPosY, findStartPosX, findStartPosY) == false)
            break;

         if (m_rngScaledPercent(m_rng) <= scaledFillRate)
         {
            const auto& patch = patchVariations[std::uniform_int_distribution<std::size_t>{0, patchVariations.size() - 1}(m_rng)];

            applyPatch(patch, patternPosX, patternPosY);
         }

         findStartPosX = patternPosX + pattern.width();
         findStartPosY = patternPosY;
      }
   };

   for (std::size_t index = patterns.size(); index --> 0;)
   {
      const auto scaledFillRate = fillRate * m_rngScaledPercentFactor / static_cast<int>(patterns.size()) * m_rngScaledPercentFactor;

      applyPatches(patterns[index], patches[index], scaledFillRate);
   }

   // Rotate

   for (auto& pattern : patterns)
   {
      pattern = pattern.rotated90CW();
   }

   for (auto& patchVariations : patches)
   {
      for (auto& patchVariation : patchVariations)
      {
         patchVariation = patchVariation.rotated90CW();
      }
   }

   for (std::size_t index = patterns.size(); index --> 0;)
   {
      const auto scaledFillRate = fillRate * m_rngScaledPercentFactor / static_cast<int>(patterns.size()) * m_rngScaledPercentFactor;

      applyPatches(patterns[index], patches[index], scaledFillRate);
   }

   // Rotate

   for (auto& pattern : patterns)
   {
      pattern = pattern.rotated90CW();
   }

   for (auto& patchVariations : patches)
   {
      for (auto& patchVariation : patchVariations)
      {
         patchVariation = patchVariation.rotated90CW();
      }
   }

   for (std::size_t index = patterns.size(); index --> 0;)
   {
      const auto scaledFillRate = fillRate * m_rngScaledPercentFactor / static_cast<int>(patterns.size()) * m_rngScaledPercentFactor;

      applyPatches(patterns[index], patches[index], scaledFillRate);
   }

   // Rotate

   for (auto& pattern : patterns)
   {
      pattern = pattern.rotated90CW();
   }

   for (auto& patchVariations : patches)
   {
      for (auto& patchVariation : patchVariations)
      {
         patchVariation = patchVariation.rotated90CW();
      }
   }

   for (std::size_t index = patterns.size(); index --> 0;)
   {
      const auto scaledFillRate = fillRate * m_rngScaledPercentFactor / static_cast<int>(patterns.size()) * m_rngScaledPercentFactor;

      applyPatches(patterns[index], patches[index], scaledFillRate);
   }
}

void WorldGen::placeLargeTrees(const int fillRate)
{
   /*
   000000000  ->  .........  ->  000000000
   000000000  ->  ...444...  ->  000444000
   000000000  ->  ..44444..  ->  004444400
   000000000  ->  .4444444.  ->  044444440
   000000000  ->  .4444444.  ->  044444440
   000000000  ->  .4444444.  ->  044444440
   000000000  ->  ..44444..  ->  004444400
   000000000  ->  ...444...  ->  000444000
   000000000  ->  .........  ->  000000000
   */

   static constexpr auto GRASS = TileType::Grass;
   static constexpr auto TREE  = TileType::Tree;
   static constexpr auto KEEP  = TileType::PatchKeep;

   Map oPattern{
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS,GRASS}
   };

   Map oPatch{
      {KEEP,KEEP,KEEP,KEEP,KEEP,KEEP,KEEP,KEEP,KEEP},
      {KEEP,KEEP,KEEP,TREE,TREE,TREE,KEEP,KEEP,KEEP},
      {KEEP,KEEP,TREE,TREE,TREE,TREE,TREE,KEEP,KEEP},
      {KEEP,TREE,TREE,TREE,TREE,TREE,TREE,TREE,KEEP},
      {KEEP,TREE,TREE,TREE,TREE,TREE,TREE,TREE,KEEP},
      {KEEP,TREE,TREE,TREE,TREE,TREE,TREE,TREE,KEEP},
      {KEEP,KEEP,TREE,TREE,TREE,TREE,TREE,KEEP,KEEP},
      {KEEP,KEEP,KEEP,TREE,TREE,TREE,KEEP,KEEP,KEEP},
      {KEEP,KEEP,KEEP,KEEP,KEEP,KEEP,KEEP,KEEP,KEEP}
   };

   std::size_t patternPosX = 0;
   std::size_t patternPosY = 0;

   std::size_t findStartPosX = 0;
   std::size_t findStartPosY = 0;

   for (;;)
   {
      if (findPattern(oPattern, patternPosX, patternPosY, findStartPosX, findStartPosY) == false)
         break;

      if (m_rngScaledPercent(m_rng) <= fillRate * m_rngScaledPercentFactor)
      {
         applyPatch(oPatch, patternPosX, patternPosY);
      }

      findStartPosX = patternPosX + oPattern.width();
      findStartPosY = patternPosY;
   }
}

void WorldGen::placeSmallTrees(const int fillRate)
{
   /*
   000000  ->  ......  ->  000000
   000000  ->  ..44..  ->  004400
   000000  ->  .4444.  ->  044440
   000000  ->  .4444.  ->  044440
   000000  ->  ..44..  ->  004400
   000000  ->  ......  ->  000000
   */

   static constexpr auto GRASS = TileType::Grass;
   static constexpr auto TREE  = TileType::Tree;
   static constexpr auto KEEP  = TileType::PatchKeep;

   Map oPattern{
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS},
      {GRASS,GRASS,GRASS,GRASS,GRASS,GRASS}
   };

   Map oPatch{
      {KEEP,KEEP,KEEP,KEEP,KEEP,KEEP},
      {KEEP,KEEP,TREE,TREE,KEEP,KEEP},
      {KEEP,TREE,TREE,TREE,TREE,KEEP},
      {KEEP,TREE,TREE,TREE,TREE,KEEP},
      {KEEP,KEEP,TREE,TREE,KEEP,KEEP},
      {KEEP,KEEP,KEEP,KEEP,KEEP,KEEP}
   };

   std::size_t patternPosX = 0;
   std::size_t patternPosY = 0;

   std::size_t findStartPosX = 0;
   std::size_t findStartPosY = 0;

   for (;;)
   {
      if (findPattern(oPattern, patternPosX, patternPosY, findStartPosX, findStartPosY) == false)
         break;

      if (m_rngScaledPercent(m_rng) <= fillRate * m_rngScaledPercentFactor)
      {
         applyPatch(oPatch, patternPosX, patternPosY);
      }

      findStartPosX = patternPosX + oPattern.width();
      findStartPosY = patternPosY;
   }
}

void WorldGen::placeRoundaboutsA()
{
   /*
   00011000  ->  00011000
   00011000  ->  00111100
   00011000  ->  01111110
   11111111  ->  11100111
   11111111  ->  11100111
   00011000  ->  01111110
   00011000  ->  00111100
   00011000  ->  00011000
   */

   static constexpr auto GRASS  = TileType::Grass;
   static constexpr auto STREET = TileType::Street;

   Map oCrossingStreetPattern{
      { GRASS, GRASS, GRASS,STREET,STREET, GRASS, GRASS, GRASS},
      { GRASS, GRASS, GRASS,STREET,STREET, GRASS, GRASS, GRASS},
      { GRASS, GRASS, GRASS,STREET,STREET, GRASS, GRASS, GRASS},
      {STREET,STREET,STREET,STREET,STREET,STREET,STREET,STREET},
      {STREET,STREET,STREET,STREET,STREET,STREET,STREET,STREET},
      { GRASS, GRASS, GRASS,STREET,STREET, GRASS, GRASS, GRASS},
      { GRASS, GRASS, GRASS,STREET,STREET, GRASS, GRASS, GRASS},
      { GRASS, GRASS, GRASS,STREET,STREET, GRASS, GRASS, GRASS}
   };

   auto placeRoundabout = [&] (const std::size_t x, const std::size_t y)
   {
      // Top left corner
      m_worldMap.at(x + 2, y + 1) = TileType::Street;
      m_worldMap.at(x + 1, y + 2) = TileType::Street;
      m_worldMap.at(x + 2, y + 2) = TileType::Street;

      // Top right corner
      m_worldMap.at(x + 5, y + 1) = TileType::Street;
      m_worldMap.at(x + 6, y + 2) = TileType::Street;
      m_worldMap.at(x + 5, y + 2) = TileType::Street;

      // Bottom left corner
      m_worldMap.at(x + 1, y + 5) = TileType::Street;
      m_worldMap.at(x + 2, y + 6) = TileType::Street;
      m_worldMap.at(x + 2, y + 5) = TileType::Street;

      // Bottom right corner
      m_worldMap.at(x + 5, y + 5) = TileType::Street;
      m_worldMap.at(x + 6, y + 5) = TileType::Street;
      m_worldMap.at(x + 5, y + 6) = TileType::Street;

      // Center
      m_worldMap.at(x + 3, y + 3) = TileType::Grass;
      m_worldMap.at(x + 4, y + 3) = TileType::Grass;
      m_worldMap.at(x + 3, y + 4) = TileType::Grass;
      m_worldMap.at(x + 4, y + 4) = TileType::Grass;
   };

   std::size_t patternPosX = 0;
   std::size_t patternPosY = 0;

   std::size_t findStartPosX = 0;
   std::size_t findStartPosY = 0;

   while (findPattern(oCrossingStreetPattern, patternPosX, patternPosY, findStartPosX, findStartPosY))
   {
      placeRoundabout(patternPosX, patternPosY);

      findStartPosX = patternPosX + oCrossingStreetPattern.width();
      findStartPosY = patternPosY;
   }
}

void WorldGen::placeRoundaboutsB()
{
   /*
         ->  .1111.
   1111  ->  111111
   1111  ->  110011
   1111  ->  110011
   1111  ->  111111
         ->  .1111.
   */

   Pattern crossingStreetPatternA{4, 5, TileType::Street};
   Pattern crossingStreetPatternB{5, 4, TileType::Street};

   auto placeRoundabout = [&] (const std::size_t x, const std::size_t y)
   {
      // Top edge
      m_worldMap.at(x    , y - 1) = TileType::Street;
      m_worldMap.at(x + 1, y - 1) = TileType::Street;
      m_worldMap.at(x + 2, y - 1) = TileType::Street;
      m_worldMap.at(x + 3, y - 1) = TileType::Street;

      // Bottom edge
      m_worldMap.at(x    , y + 4) = TileType::Street;
      m_worldMap.at(x + 1, y + 4) = TileType::Street;
      m_worldMap.at(x + 2, y + 4) = TileType::Street;
      m_worldMap.at(x + 3, y + 4) = TileType::Street;

      // Left edge
      m_worldMap.at(x - 1, y    ) = TileType::Street;
      m_worldMap.at(x - 1, y + 1) = TileType::Street;
      m_worldMap.at(x - 1, y + 2) = TileType::Street;
      m_worldMap.at(x - 1, y + 3) = TileType::Street;

      // Right edge
      m_worldMap.at(x + 4, y    ) = TileType::Street;
      m_worldMap.at(x + 4, y + 1) = TileType::Street;
      m_worldMap.at(x + 4, y + 2) = TileType::Street;
      m_worldMap.at(x + 4, y + 3) = TileType::Street;

      // Center
      m_worldMap.at(x + 1, y + 1) = TileType::Grass;
      m_worldMap.at(x + 1, y + 2) = TileType::Grass;
      m_worldMap.at(x + 2, y + 1) = TileType::Grass;
      m_worldMap.at(x + 2, y + 2) = TileType::Grass;
   };

   std::size_t patternPosX = 0;
   std::size_t patternPosY = 0;

   std::size_t findStartPosX = 0;
   std::size_t findStartPosY = 0;

   while (findPattern(crossingStreetPatternA, patternPosX, patternPosY, findStartPosX, findStartPosY))
   {
      placeRoundabout(patternPosX, patternPosY);

      findStartPosX = patternPosX + 8;
      findStartPosY = patternPosY;
   }

   findStartPosX = 0;
   findStartPosY = 0;

   while (findPattern(crossingStreetPatternB, patternPosX, patternPosY, findStartPosX, findStartPosY))
   {
      placeRoundabout(patternPosX, patternPosY);

      findStartPosX = patternPosX + 8;
      findStartPosY = patternPosY;
   }
}
