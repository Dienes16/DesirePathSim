#ifndef WORLDGEN_HPP
#define WORLDGEN_HPP

#include <random>

#include "WorldMap.hpp"
#include "Voronoi.hpp"

class WorldGen final
{
public:
   using Pattern  = Map<TileType>;
   using Patch    = Map<TileType>;

public:
   WorldGen(WorldMap& worldMap, std::mt19937_64& rng);

   WorldGen(const WorldGen&) = default;
   WorldGen(WorldGen&&) noexcept = default;

   ~WorldGen() = default;

   WorldGen& operator=(const WorldGen&) = default;
   WorldGen& operator=(WorldGen&&) noexcept = default;

   void placeStreetsFromVoronoiMap(const VoronoiMap& voronoiMap);

   void placePonds(const float fillLimitInPercent, const float areaSizeLimitInPercent);

   void placeFullPavedAreas(const float fillLimitInPercent, const float areaSizeLimitInPercent);

   void replaceAll(const TileType replaceWhat, const TileType replaceWith);

   void placeBuildings(const int fillRate = 100);

   void placeLargeTrees(const int fillRate = 100);
   void placeSmallTrees(const int fillRate = 100);

   void placeRoundaboutsA();
   void placeRoundaboutsB();

private:
   static constexpr int m_rngScaledPercentFactor = 1'000;

   WorldMap& m_worldMap;

   std::mt19937_64& m_rng;
   std::uniform_int_distribution<> m_rngScaledPercent; // Scaled by m_rngScaledPercentFactor
   std::uniform_int_distribution<std::size_t> m_rngWorldMapWidth;
   std::uniform_int_distribution<std::size_t> m_rngWorldMapHeight;

   bool findPattern(const Pattern& pattern, std::size_t& x, std::size_t& y, const std::size_t startX = 0, const std::size_t startY = 0) const;

   void applyPatch(const Patch& patch, const std::size_t x, const std::size_t y);

   std::size_t floodFill(const std::size_t x, const std::size_t y, const TileType fillTileType);
};

#endif // WORLDGEN_HPP
