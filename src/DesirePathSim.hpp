#ifndef DESIREPATHSIM_HPP
#define DESIREPATHSIM_HPP

#include <vector>
#include <atomic>
#include <random>

#include <raylib.h>

#include "Options.hpp"
#include "Voronoi.hpp"
#include "WorldMap.hpp"
#include "DesirePaths.hpp"
#include "Bitmap.hpp"
#include "Villager.hpp"
#include "UpdateRect.hpp"
#include "Queue.hpp"

class DesirePathSim final
{
public:
   DesirePathSim(const Options& options);

   DesirePathSim(const DesirePathSim&) = default;
   DesirePathSim(DesirePathSim&&) = default;

   ~DesirePathSim();

   DesirePathSim& operator=(const DesirePathSim&) = default;
   DesirePathSim& operator=(DesirePathSim&&) = default;

   void run();

private:
   const Options& m_options;

   // Assigns a color to each Voronoi Centroid
   using VoronoiColorTable = std::vector<Color>;

   static constexpr auto m_tileWidthPixels = 6;
   static constexpr auto m_tileHeightPixels = 6;

   int m_worldWidthTiles;
   int m_worldHeightTiles;

   int m_worldWidthPixels;
   int m_worldHeightPixels;

   std::mt19937_64 m_baseRNG;

   // For each world tile, stores the index of the closest Centroid
   VoronoiMap m_voronoiMap;

   VoronoiColorTable m_voronoiColorTable = {};

   WorldMap m_worldMap;

   CostMap m_baseCostMap;

   DesirePathsMap m_desirePathsMap;

   Bitmap m_shadowBitmap;

   std::vector<Villager> m_villagers;

   Camera2D m_camera = {0};

   RenderTexture2D m_worldMapTexture = {};
   RenderTexture2D m_shadowMapTexture = {};
   RenderTexture2D m_desirePathsMapTexture = {};

   // Villagers waiting for new path are enqueued here
   Queue<Villager*> m_pathfindingQueue;
   std::atomic<bool> m_stopPathfindingThread;

private:
   void tickVillagers(UpdateRect& mapUpdateRect, const float delta);

   void updateCamera(const float delta);

   void drawVoronoiMap();

   void updateWorldMapTexture(RenderTexture2D& texture, UpdateRect& updateRect);
   void updateWorldMapTexture(RenderTexture2D& texture);

   void updateShadowMapTexture(RenderTexture2D& texture, const Bitmap& shadowBitmap);

   void updateDesirePathsMapTexture(RenderTexture2D& texture, const UpdateRect& updateRect);
   void updateDesirePathsMapTexture(RenderTexture2D& texture);

   void updateBaseCostMap();
   void updateShadowBitmap();
};

#endif // DESIREPATHSIM_HPP
