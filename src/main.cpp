#include <string>
#include <iostream>

#include "Options.hpp"
#include "DesirePathSim.hpp"

std::optional<int> tryReadArgInt(const std::string& arg, const std::string& name) try
{
   std::string argStart = '-' + name + '=';

   if (arg.find(argStart) == 0)
      return std::stoi(arg.substr(argStart.length()));

   return std::nullopt;
}
catch (...)
{
   throw std::runtime_error{"Error parsing argument \"" + arg + "\""};
}

std::optional<bool> tryReadArgBool(const std::string& arg, const std::string& name)
{
   const auto v = tryReadArgInt(arg, name);

   if (v.has_value() == false)
      return std::nullopt;

   return (v.value() != 0);
}

int main(int argc, char** argv)
{
   Options options;

   try
   {
      for (int argIndex = 1; argIndex < argc; ++argIndex)
      {
         std::string arg = argv[argIndex];

              if (auto v = tryReadArgInt (arg, "width"                 ); v.has_value()) options.screenWidthPixels                 = v.value();
         else if (auto v = tryReadArgInt (arg, "height"                ); v.has_value()) options.screenHeightPixels                = v.value();
         else if (auto v = tryReadArgInt (arg, "target_fps"            ); v.has_value()) options.targetFPS                         = v.value();
         else if (auto v = tryReadArgBool(arg, "remove_streets"        ); v.has_value()) options.removeStreetsAfterGeneration      = v.value();
         else if (auto v = tryReadArgInt (arg, "villager_count"        ); v.has_value()) options.villagerCount                     = v.value();
         else if (auto v = tryReadArgInt (arg, "centroid_count"        ); v.has_value()) options.voronoiCentroidCountPerLevel      = v.value();
         else if (auto v = tryReadArgInt (arg, "subdiv_prob_1"         ); v.has_value()) options.voronoiSubdivideProbabilityLevel1 = v.value();
         else if (auto v = tryReadArgInt (arg, "subdiv_prob_2"         ); v.has_value()) options.voronoiSubdivideProbabilityLevel2 = v.value();
         else if (auto v = tryReadArgInt (arg, "minkowski_0"           ); v.has_value()) options.voronoiLevel0MinkowskiP           = static_cast<float>(v.value());
         else if (auto v = tryReadArgInt (arg, "minkowski_1"           ); v.has_value()) options.voronoiLevel1MinkowskiP           = static_cast<float>(v.value());
         else if (auto v = tryReadArgInt (arg, "minkowski_2"           ); v.has_value()) options.voronoiLevel2MinkowskiP           = static_cast<float>(v.value());
         else if (auto v = tryReadArgBool(arg, "place_roundabouts"     ); v.has_value()) options.placeRoundabouts                  = v.value();
         else if (auto v = tryReadArgBool(arg, "place_ponds"           ); v.has_value()) options.placePonds                        = v.value();
         else if (auto v = tryReadArgBool(arg, "place_full_paved_areas"); v.has_value()) options.placeFullPavedAreas               = v.value();
         else if (auto v = tryReadArgBool(arg, "place_large_trees"     ); v.has_value()) options.placeLargeTrees                   = v.value();
         else if (auto v = tryReadArgBool(arg, "place_small_trees"     ); v.has_value()) options.placeSmallTrees                   = v.value();
         else if (auto v = tryReadArgInt (arg, "pathfinding_threads"   ); v.has_value()) options.pathfindingThreadCount            = v.value();
         else if (auto v = tryReadArgBool(arg, "pave_desire_paths"     ); v.has_value()) options.paveDesirePaths                   = v.value();
         else if (auto v = tryReadArgBool(arg, "decay_desire_paths"    ); v.has_value()) options.decayDesirePaths                  = v.value();
      }
   }
   catch (const std::exception& ex)
   {
      std::cerr << ex.what();
      return EXIT_FAILURE;
   }

   DesirePathSim desirePathSim{options};

   desirePathSim.run();

   return EXIT_SUCCESS;
}
