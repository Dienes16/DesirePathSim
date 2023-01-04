#ifndef WORLDMAP_HPP
#define WORLDMAP_HPP

#include <random>

#include "Map.hpp"
#include "TileType.hpp"

using WorldMap = Map<TileType>;

inline std::uint8_t getBaseCostForValue(const TileType tileType, std::mt19937_64& rng)
{
   switch (tileType)
   {
   case TileType::Grass           : return std::uniform_int_distribution<>{14, 16}(rng); break;
   case TileType::Street          : return  10;                                          break;
   case TileType::Building        : return   0;                                          break;
   case TileType::BuildingEntrance: return  12;                                          break;
   case TileType::Tree            : return   0;                                          break;
   case TileType::Water           : return 100;                                          break;
   default                        : return   0;
   }
}

#endif // WORLDMAP_HPP
