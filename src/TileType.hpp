#ifndef TILETYPE_HPP
#define TILETYPE_HPP

#include <cstdint>

enum class TileType : std::uint8_t
{
   Grass            =   0,
   Street           =   1,
   Building         =   2,
   BuildingEntrance =   3,
   Tree             =   4,
   Water            =   5,
   Temporary        = 253,

   PatternAny       = 254,

   PatchKeep        = 255
};

#endif // TILETYPE_HPP
