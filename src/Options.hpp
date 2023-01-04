#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <cstdlib>

struct Options final
{
   int screenWidthPixels = 1920;
   int screenHeightPixels = 1080;

   int targetFPS = 60; // 0 to disable

   bool removeStreetsAfterGeneration = false;

   std::size_t villagerCount = 1000;

   std::size_t voronoiCentroidCountPerLevel = 6;
   int voronoiSubdivideProbabilityLevel1 = 90;
   int voronoiSubdivideProbabilityLevel2 = 80;
   float voronoiLevel0MinkowskiP = 3.0f; // 3.0f for more curved long streets on top level
   float voronoiLevel1MinkowskiP = 3.0f; // 3.0f for more curved streets on level 1
   float voronoiLevel2MinkowskiP = 1.0f; // 1.0f for more straight streets on level 2

   bool placeRoundabouts = false;
   bool placePonds = true;
   bool placeFullPavedAreas = true;
   bool placeLargeTrees = true;
   bool placeSmallTrees = true;

   std::size_t pathfindingThreadCount = 4;

   bool paveDesirePaths = true;
   bool decayDesirePaths = true;
};

#endif // OPTIONS_HPP
