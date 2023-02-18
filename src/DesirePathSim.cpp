#include "DesirePathSim.hpp"

#include <future>
#include <chrono>

#include <rlgl.h>

#include "WorldGen.hpp"
#include "Pathfinding.hpp"
#include "Version.hpp"

DesirePathSim::DesirePathSim(const Options& options):
   m_options{options},
   m_worldWidthTiles{m_options.screenWidthPixels * 2 / m_tileWidthPixels},
   m_worldHeightTiles{m_options.screenHeightPixels * 2 / m_tileHeightPixels},
   m_worldWidthPixels{m_worldWidthTiles * m_tileWidthPixels},
   m_worldHeightPixels{m_worldHeightTiles * m_tileHeightPixels},
   m_baseRNG{std::random_device{}()},
   m_voronoiMap{static_cast<size_t>(m_worldWidthTiles), static_cast<size_t>(m_worldHeightTiles)},
   m_worldMap{static_cast<size_t>(m_worldWidthTiles), static_cast<size_t>(m_worldHeightTiles), TileType::Grass},
   m_baseCostMap{static_cast<size_t>(m_worldWidthTiles), static_cast<size_t>(m_worldHeightTiles), 0},
   m_desirePathsMap{static_cast<size_t>(m_worldWidthTiles), static_cast<size_t>(m_worldHeightTiles), 0},
   m_shadowBitmap{static_cast<size_t>(m_worldWidthTiles), static_cast<size_t>(m_worldHeightTiles), 0},
   m_villagers{m_options.villagerCount}
{
   std::uniform_int_distribution<> rngPercent{1, 100};

   auto voronoiCentroidList = Voronoi::generateCentroids(0, 0, m_voronoiMap.width(), m_voronoiMap.height(), m_options.voronoiCentroidCountPerLevel, m_baseRNG);

   for (std::size_t centroidY = 0; centroidY < m_voronoiMap.height(); ++centroidY)
   {
      for (std::size_t centroidX = 0; centroidX < m_voronoiMap.width(); ++centroidX)
      {
         m_voronoiMap.at(centroidX, centroidY) = Voronoi::getShortestDistanceCentroidIndex(centroidX, centroidY, voronoiCentroidList, [this] (const std::size_t aX, const std::size_t aY, const std::size_t bX, const std::size_t bY)
         {
            return Voronoi::distanceMinkowski(aX, aY, bX, bY, m_options.voronoiLevel0MinkowskiP);
         });
      }
   }

   // Subdivide level 1

   for (std::size_t subdivideCentroidIndex = 0; subdivideCentroidIndex < m_options.voronoiCentroidCountPerLevel; ++subdivideCentroidIndex)
   {
      if (rngPercent(m_baseRNG) <= m_options.voronoiSubdivideProbabilityLevel1)
      {
         Voronoi::subdivide(voronoiCentroidList, subdivideCentroidIndex, m_voronoiMap, m_options.voronoiCentroidCountPerLevel, m_baseRNG, [this] (const std::size_t aX, const std::size_t aY, const std::size_t bX, const std::size_t bY)
         {
            return Voronoi::distanceMinkowski(aX, aY, bX, bY, m_options.voronoiLevel1MinkowskiP);
         });
      }
   }

   // Subdivide level 2

   for (std::size_t subdivideCentroidIndex = m_options.voronoiCentroidCountPerLevel; subdivideCentroidIndex < m_options.voronoiCentroidCountPerLevel + m_options.voronoiCentroidCountPerLevel * m_options.voronoiCentroidCountPerLevel; ++subdivideCentroidIndex)
   {
      if (rngPercent(m_baseRNG) <= m_options.voronoiSubdivideProbabilityLevel2)
      {
         Voronoi::subdivide(voronoiCentroidList, subdivideCentroidIndex, m_voronoiMap, m_options.voronoiCentroidCountPerLevel, m_baseRNG, [this] (const std::size_t aX, const std::size_t aY, const std::size_t bX, const std::size_t bY)
         {
            return Voronoi::distanceMinkowski(aX, aY, bX, bY, m_options.voronoiLevel2MinkowskiP);
         });
      }
   }

   m_voronoiColorTable.reserve(voronoiCentroidList.size());

   for (std::size_t cellColorIndex = 0; cellColorIndex < voronoiCentroidList.size(); ++cellColorIndex)
   {
      std::uniform_int_distribution<> rngCellColorChannel{50, 255};

      Color color
      {
         static_cast<unsigned char>(rngCellColorChannel(m_baseRNG)),
         static_cast<unsigned char>(rngCellColorChannel(m_baseRNG)),
         static_cast<unsigned char>(rngCellColorChannel(m_baseRNG)),
         255
      };

      m_voronoiColorTable.emplace_back(color);
   }

   WorldGen worldGen{m_worldMap, m_baseRNG};

   worldGen.placeStreetsFromVoronoiMap(m_voronoiMap);

   if (m_options.placeRoundabouts)
   {
      worldGen.placeRoundaboutsA();
      worldGen.placeRoundaboutsB();
   }

   if (m_options.placePonds)
   {
      worldGen.placePonds(4.0f, 5.0f);
   }

   worldGen.placeBuildings(100);

   if (m_options.placeFullPavedAreas)
   {
      worldGen.placeFullPavedAreas(2.0f, 0.5f);
   }

   if (m_options.placeLargeTrees)
   {
      worldGen.placeLargeTrees(5);
   }

   if (m_options.placeSmallTrees)
   {
      worldGen.placeSmallTrees(10);
   }

   if (m_options.removeStreetsAfterGeneration)
   {
      worldGen.replaceAll(TileType::Street, TileType::Grass);
   }

   updateBaseCostMap();
   updateShadowBitmap();

   for (auto& villager : m_villagers)
   {
      std::uniform_int_distribution<> rngColorChannel{8, 128};

      villager.m_color.r = static_cast<unsigned char>(rngColorChannel(m_baseRNG));
      villager.m_color.g = static_cast<unsigned char>(rngColorChannel(m_baseRNG));
      villager.m_color.b = static_cast<unsigned char>(rngColorChannel(m_baseRNG));

      villager.m_movementPixelPerSec = static_cast<float>(std::uniform_int_distribution<>{15, 20}(m_baseRNG)) * 4.0f;

      villager.setState(Villager::State::EnqueuedForPath);
      m_pathfindingQueue.push(&villager);
   }

   InitWindow(m_options.screenWidthPixels, m_options.screenHeightPixels, getAppNameWithVersion());

   if (m_options.targetFPS > 0)
   {
      SetTargetFPS(m_options.targetFPS);
   }

   m_worldMapTexture = LoadRenderTexture(m_worldWidthPixels, m_worldHeightPixels);
   updateWorldMapTexture(m_worldMapTexture);

   m_shadowMapTexture = LoadRenderTexture(m_worldWidthPixels, m_worldHeightPixels);
   updateShadowMapTexture(m_shadowMapTexture, m_shadowBitmap);

   m_desirePathsMapTexture = LoadRenderTexture(m_worldWidthPixels, m_worldHeightPixels);

   m_camera.target = {static_cast<float>(m_options.screenWidthPixels    ), static_cast<float>(m_options.screenHeightPixels    )};
   m_camera.offset = {static_cast<float>(m_options.screenWidthPixels / 2), static_cast<float>(m_options.screenHeightPixels / 2)};
   m_camera.zoom = 0.5f;
}

DesirePathSim::~DesirePathSim()
{
   UnloadRenderTexture(m_desirePathsMapTexture);
   UnloadRenderTexture(m_shadowMapTexture);
   UnloadRenderTexture(m_worldMapTexture);

   CloseWindow();
}

void DesirePathSim::run()
{
   bool drawVoronoi = false;
   bool drawShadowMap = true;
   bool drawDesirePath = true;
   bool drawDesirePathUpdateRect = false;
   bool drawKeysInfo = false;

   std::size_t currentDesirePathsMapUpdateIndex = 0;

   float desirePathDecayRatePerSec = 0.25f;
   float desirePathDecayAccu = 0.0f;

   float fpsUpdateRatePerSec = 2.0f;
   float fpsUpdateAccu = 0.0f;
   int fps = 0;

   std::vector<Pathfinder> pathfinders;
   pathfinders.reserve(m_options.pathfindingThreadCount);

   std::vector<std::mt19937_64> pathfindingRNGs;
   pathfindingRNGs.reserve(m_options.pathfindingThreadCount);

   std::vector<std::future<void>> pathfindingFutures;
   pathfindingFutures.reserve(m_options.pathfindingThreadCount);

   for (std::size_t pathfindingThreadIndex = 0; pathfindingThreadIndex < m_options.pathfindingThreadCount; ++pathfindingThreadIndex)
   {
      pathfinders.emplace_back(m_worldMap.width(), m_worldMap.height());

      pathfindingRNGs.emplace_back(m_baseRNG());

      pathfindingFutures.emplace_back(std::async(std::launch::async, [&] (const std::size_t threadIndex)
      {
         using namespace std::chrono_literals;

         while (m_stopPathfindingThread.load() == false)
         {
            if (auto villager = m_pathfindingQueue.tryPopFor(100ms).value_or(nullptr); villager != nullptr)
            {
               villager->reset(m_worldMap, pathfinders[threadIndex], m_baseCostMap, m_tileWidthPixels, m_tileHeightPixels, pathfindingRNGs[threadIndex]);
            }
         }
      }, pathfindingThreadIndex));
   }

   UpdateRect mapUpdateRect; // If this becomes non-0, the map itself has changed there and needs a visual update

   std::vector<UpdateRect> desirePathsUpdateRects; // Every frame a different subsection of the desire paths will be redrawn

   {
      constexpr std::size_t desirePathUpdateRectEdgeSize = 64;

      UpdateRect currentDesirePathUpdateRect{0, 0, desirePathUpdateRectEdgeSize, desirePathUpdateRectEdgeSize};

      currentDesirePathUpdateRect.right  = std::min(currentDesirePathUpdateRect.right , m_worldMap.width () - 1);
      currentDesirePathUpdateRect.bottom = std::min(currentDesirePathUpdateRect.bottom, m_worldMap.height() - 1);

      desirePathsUpdateRects.emplace_back(currentDesirePathUpdateRect);

      for (;;)
      {
         currentDesirePathUpdateRect.left  += desirePathUpdateRectEdgeSize;
         currentDesirePathUpdateRect.right  = std::min(currentDesirePathUpdateRect.left + desirePathUpdateRectEdgeSize, m_worldMap.width() - 1);

         if (currentDesirePathUpdateRect.left >= m_worldMap.width())
         {
            currentDesirePathUpdateRect.left = 0;
            currentDesirePathUpdateRect.right  = std::min(currentDesirePathUpdateRect.left + desirePathUpdateRectEdgeSize, m_worldMap.width() - 1);

            currentDesirePathUpdateRect.top  += desirePathUpdateRectEdgeSize;
            currentDesirePathUpdateRect.bottom  = std::min(currentDesirePathUpdateRect.top + desirePathUpdateRectEdgeSize, m_worldMap.height() - 1);

            if (currentDesirePathUpdateRect.top >= m_worldMap.height())
               break; // Reached end of map
         }

         desirePathsUpdateRects.emplace_back(currentDesirePathUpdateRect);
      }
   }

   while (WindowShouldClose() == false)
   {
      const auto delta = GetFrameTime();

      fpsUpdateAccu += delta;

      if (fpsUpdateAccu >= 1.0f / fpsUpdateRatePerSec)
      {
         fps = GetFPS();

         fpsUpdateAccu = 0.0f;
      }

      tickVillagers(mapUpdateRect, delta);

      updateDesirePathsMapTexture(m_desirePathsMapTexture, desirePathsUpdateRects[currentDesirePathsMapUpdateIndex]);
      currentDesirePathsMapUpdateIndex = currentDesirePathsMapUpdateIndex == desirePathsUpdateRects.size() - 1 ? 0 : currentDesirePathsMapUpdateIndex + 1;

      desirePathDecayAccu += delta;

      if (desirePathDecayAccu >= 1.0f / desirePathDecayRatePerSec)
      {
         if (m_options.decayDesirePaths)
         {
            decayDesirePaths(m_desirePathsMap, m_baseCostMap);
         }

         desirePathDecayAccu = 0.0f;
      }

      if (mapUpdateRect.empty() == false)
      {
         updateWorldMapTexture(m_worldMapTexture, mapUpdateRect);
      }

      if (IsKeyPressed(KEY_ESCAPE))
         break;

      if (IsKeyPressed(KEY_V))
      {
         drawVoronoi = !drawVoronoi;
      }

      if (IsKeyPressed(KEY_S))
      {
         drawShadowMap = !drawShadowMap;
      }

      if (IsKeyPressed(KEY_D))
      {
         drawDesirePath = !drawDesirePath;
      }

      if (IsKeyPressed(KEY_U))
      {
         drawDesirePathUpdateRect = !drawDesirePathUpdateRect;
      }

      if (IsKeyPressed(KEY_F1))
      {
         drawKeysInfo = !drawKeysInfo;
      }

      const auto mouseWheelMove = GetMouseWheelMove();

      if (mouseWheelMove != 0)
      {
         m_camera.zoom += static_cast<float>(mouseWheelMove) * 0.2f;
      }

      updateCamera(delta);

      BeginDrawing();

      ClearBackground({0, 0, 0, 255});

      BeginMode2D(m_camera);

      if (drawVoronoi)
      {
         drawVoronoiMap();
      }
      else
      {
         // NOTE: Render texture must be y-flipped due to default OpenGL coordinates (left-bottom)
         DrawTextureRec(
            m_worldMapTexture.texture,
            {0.0f, 0.0f, static_cast<float>(m_worldMapTexture.texture.width), -static_cast<float>(m_worldMapTexture.texture.height)},
            Vector2{0.0f, 0.0f},
            WHITE
         );

         if (drawDesirePath)
         {
            // NOTE: Render texture must be y-flipped due to default OpenGL coordinates (left-bottom)
            DrawTextureRec(
               m_desirePathsMapTexture.texture,
               {0.0f, 0.0f, static_cast<float>(m_desirePathsMapTexture.texture.width), -static_cast<float>(m_desirePathsMapTexture.texture.height)},
               Vector2{0.0f, 0.0f},
               WHITE
            );
         }

         // Draw villagers

         for (const auto& villager : m_villagers)
         {
            DrawRectangleV(villager.m_position, {static_cast<float>(m_tileWidthPixels), static_cast<float>(m_tileHeightPixels)}, villager.m_color);
         }

         if (drawShadowMap)
         {
            // NOTE: Render texture must be y-flipped due to default OpenGL coordinates (left-bottom)
            DrawTextureRec(
               m_shadowMapTexture.texture,
               {0.0f, 0.0f, static_cast<float>(m_shadowMapTexture.texture.width), -static_cast<float>(m_shadowMapTexture.texture.height)},
               Vector2{0.0f, 0.0f},
               WHITE
            );
         }

         if (drawDesirePathUpdateRect)
         {
            const auto& desirePathsMapUpdateRect = desirePathsUpdateRects[currentDesirePathsMapUpdateIndex];

            Vector2 position{static_cast<float>(desirePathsMapUpdateRect.left * m_tileWidthPixels), static_cast<float>(desirePathsMapUpdateRect.top * m_tileHeightPixels)};

            Vector2 size{static_cast<float>(desirePathsMapUpdateRect.width() * m_tileWidthPixels), static_cast<float>(desirePathsMapUpdateRect.height() * m_tileHeightPixels)};

            DrawRectangleV(position, size, {255, 0, 0, 64});
         }
      }

      EndMode2D();

      DrawText(TextFormat("FPS: %i", fps), 10, 10, 20, BLACK);

      static constexpr int s_kkiKeyInfoFontSize = 20;

      static constexpr char s_kksz8KeysInfoShort[] = "[F1] Toggle keys info";
      static constexpr char s_kksz8KeysInfoFull[] =
         "[F1] Toggle keys info\n"
         "[ESC] Exit\n"
         "[V] Toggle Voronoi\n"
         "[S] Toggle shadow map\n"
         "[D] Toggle desire path\n"
         "[U] Toggle desire path update rect\n"
         "[ARROW KEYS] Move map\n"
         "[MOUSE WHEEL] Zoom\n";

      const int keysInfoShortTextWidth = MeasureText(s_kksz8KeysInfoShort, s_kkiKeyInfoFontSize);

      if (drawKeysInfo)
      {
         const int keysInfoFullTextWidth = MeasureText(s_kksz8KeysInfoFull, s_kkiKeyInfoFontSize);
         DrawText(s_kksz8KeysInfoFull, m_options.screenWidthPixels - 10 - keysInfoFullTextWidth, 10, s_kkiKeyInfoFontSize, BLACK);
      }
      else
      {
         const int keysInfoShortTextWidth = MeasureText(s_kksz8KeysInfoShort, s_kkiKeyInfoFontSize);
         DrawText(s_kksz8KeysInfoShort, m_options.screenWidthPixels - 10 - keysInfoShortTextWidth, 10, s_kkiKeyInfoFontSize, BLACK);
      }

      EndDrawing();
   }

   m_stopPathfindingThread.store(true);

   for (auto& pathfindingFuture : pathfindingFutures)
   {
      pathfindingFuture.wait();
   }
}

void DesirePathSim::tickVillagers(
   UpdateRect& mapUpdateRect,
   const float delta
)
{
   for (auto& villager : m_villagers)
   {
      villager.tick(m_worldMap, mapUpdateRect, m_baseCostMap, m_desirePathsMap, m_options.paveDesirePaths, m_tileWidthPixels, m_tileHeightPixels, m_baseRNG, delta);

      if (villager.getState() == Villager::State::AwaitingPath)
      {
         villager.setState(Villager::State::EnqueuedForPath);
         m_pathfindingQueue.push(&villager);
      }
   }
}

void DesirePathSim::updateCamera(const float delta)
{
   auto cameraSpeed = 400.0f;

   if (IsKeyDown(KEY_RIGHT_SHIFT))
      cameraSpeed *= 2.0;

   if (IsKeyDown(KEY_LEFT))
      m_camera.target.x -= cameraSpeed * delta;
   else if (IsKeyDown(KEY_RIGHT))
      m_camera.target.x += cameraSpeed * delta;

   if (IsKeyDown(KEY_UP))
      m_camera.target.y -= cameraSpeed * delta;
   else if (IsKeyDown(KEY_DOWN))
      m_camera.target.y += cameraSpeed * delta;
}

void DesirePathSim::drawVoronoiMap()
{
   for (std::size_t voronoiY = 0; voronoiY < m_voronoiMap.height(); ++voronoiY)
   {
      for (std::size_t voronoiX = 0; voronoiX < m_voronoiMap.width(); ++voronoiX)
      {
         Vector2 tilePos{static_cast<float>(voronoiX * m_tileWidthPixels), static_cast<float>(voronoiY * m_tileHeightPixels)};

         DrawRectangleV(tilePos, {static_cast<float>(m_tileWidthPixels), static_cast<float>(m_tileHeightPixels)}, m_voronoiColorTable.at(m_voronoiMap.at(voronoiX, voronoiY)));
      }
   }
}

void DesirePathSim::updateWorldMapTexture(RenderTexture2D& texture, UpdateRect& updateRect)
{
   std::uniform_int_distribution<> rngPercent{1, 100};

   BeginTextureMode(texture);

   for (std::size_t worldTileY = updateRect.top; worldTileY <= updateRect.bottom; ++worldTileY)
   {
      for (std::size_t worldTileX = updateRect.left; worldTileX <= updateRect.right; ++worldTileX)
      {
         Vector2 tilePos{static_cast<float>(worldTileX * m_tileWidthPixels), static_cast<float>(worldTileY * m_tileHeightPixels)};

         Color color{0, 0, 0, 255};

         if (m_worldMap.at(worldTileX, worldTileY) == TileType::Water)
         {
            if (worldTileY % 3 == 1)
            {
               color = (worldTileX % 3 == 0) ? Color{60, 170, 255, 255} : Color{20, 130, 230, 255};
            }
            else if (worldTileY % 3 == 0)
            {
               color = (worldTileX % 3 > 0) ? Color{60, 170, 255, 255} : Color{20, 130, 230, 255};
            }
            else
            {
               color = Color{20, 130, 230, 255};
            }

            // Bevel
            if (worldTileX > 0 && m_worldMap.at(worldTileX - 1, worldTileY) != TileType::Water)
            {
               color.r -= 20;
               color.g -= 20;
               color.b -= 20;
            }
            else if (worldTileY > 0 && m_worldMap.at(worldTileX, worldTileY - 1) != TileType::Water)
            {
               color.r -= 20;
               color.g -= 20;
               color.b -= 20;
            }
         }
         else if (m_worldMap.at(worldTileX, worldTileY) == TileType::Tree)
         {
            color = (worldTileX + worldTileY) % 2 ? Color{90, 180, 40, 255} : Color{70, 160, 20, 255};

            // Bevel
            if (worldTileX < m_worldMap.width() - 1 && m_worldMap.at(worldTileX + 1, worldTileY) != TileType::Tree)
            {
               color.r -= 20;
               color.g -= 20;
               color.b -= 20;
            }
            else if (worldTileY < m_worldMap.height() - 1 && m_worldMap.at(worldTileX, worldTileY + 1) != TileType::Tree)
            {
               color.r -= 20;
               color.g -= 20;
               color.b -= 20;
            }
         }
         else if (m_worldMap.at(worldTileX, worldTileY) == TileType::BuildingEntrance)
         {
            if (rngPercent(m_baseRNG) <= 90)
            {
               color = Color{150, 150, 150, 255};
            }
            else
            {
               color = Color{130, 130, 130, 255};
            }
         }
         else if (m_worldMap.at(worldTileX, worldTileY) == TileType::Building)
         {
            color = worldTileX % 2 ? Color{230, 130, 80, 255} : Color{210, 110, 60, 255};

            // Bevel
            if (worldTileX > 0 && m_worldMap.at(worldTileX - 1, worldTileY) != TileType::Building)
            {
               color.r += 20;
               color.g += 20;
               color.b += 20;
            }
            else if (worldTileY > 0 && m_worldMap.at(worldTileX, worldTileY - 1) != TileType::Building)
            {
               color.r += 20;
               color.g += 20;
               color.b += 20;
            }
            else if (worldTileX < m_worldMap.width() - 1 && m_worldMap.at(worldTileX + 1, worldTileY) != TileType::Building)
            {
               color.r -= 20;
               color.g -= 20;
               color.b -= 20;
            }
            else if (worldTileY < m_worldMap.height() - 1 && m_worldMap.at(worldTileX, worldTileY + 1) != TileType::Building)
            {
               color.r -= 20;
               color.g -= 20;
               color.b -= 20;
            }
         }
         else if (m_worldMap.at(worldTileX, worldTileY) == TileType::Street)
         {
            if (rngPercent(m_baseRNG) <= 90)
            {
               color = Color{150, 150, 150, 255};
            }
            else
            {
               color = Color{130, 130, 130, 255};
            }
         }
         else if(m_worldMap.at(worldTileX, worldTileY) == TileType::Grass)
         {
            if (rngPercent(m_baseRNG) <= 20)
            {
               color = Color{110, 200, 60, 255};
            }
            else
            {
               color = Color{130, 220, 80, 255};
            }
         }

         DrawRectangleV(tilePos, {static_cast<float>(m_tileWidthPixels), static_cast<float>(m_tileHeightPixels)}, color);
      }
   }

   EndTextureMode();

   updateRect.reset();
}

void DesirePathSim::updateWorldMapTexture(RenderTexture2D& texture)
{
   UpdateRect updateRect{0, 0, m_worldMap.width() - 1, m_worldMap.height() - 1};

   updateWorldMapTexture(texture, updateRect);
}

void DesirePathSim::updateShadowMapTexture(RenderTexture2D& texture, const Bitmap& shadowBitmap)
{
   BeginTextureMode(texture);

   for (std::size_t worldTileY = 0; worldTileY < shadowBitmap.height(); ++worldTileY)
   {
      for (std::size_t worldTileX = 0; worldTileX < shadowBitmap.width(); ++worldTileX)
      {
         Vector2 tilePos{static_cast<float>(worldTileX * m_tileWidthPixels), static_cast<float>(worldTileY * m_tileHeightPixels)};

         Color color{0, 0, 0, 0};

         if (shadowBitmap.at(worldTileX, worldTileY) == 1) // Shadow
         {
            color = Color{0, 0, 0, 150};
         }

         DrawRectangleV(tilePos, {static_cast<float>(m_tileWidthPixels), static_cast<float>(m_tileHeightPixels)}, color);
      }
   }

   EndTextureMode();
}

void DesirePathSim::updateDesirePathsMapTexture(RenderTexture2D& texture, const UpdateRect& updateRect)
{
   BeginTextureMode(texture);

   const Vector2 updateRectPos  = {static_cast<float>(updateRect.left    * m_tileWidthPixels), static_cast<float>(updateRect.top      * m_tileHeightPixels)};
   const Vector2 updateRectSize = {static_cast<float>(updateRect.width() * m_tileWidthPixels), static_cast<float>(updateRect.height() * m_tileHeightPixels)};

   //                SRC_ALPHA, SRC_ALPHA,    MIN
   rlSetBlendFactors(   0x0302,    0x0302, 0x8007); // Clear
   rlSetBlendMode(BLEND_CUSTOM);
   DrawRectangleV(updateRectPos, updateRectSize, {0, 0, 0, 0});
   rlSetBlendMode(BLEND_ALPHA);

   for (std::size_t worldTileY = updateRect.top; worldTileY <= updateRect.bottom; ++worldTileY)
   {
      for (std::size_t worldTileX = updateRect.left; worldTileX <= updateRect.right; ++worldTileX)
      {
         const Vector2 tilePos{static_cast<float>(worldTileX * m_tileWidthPixels), static_cast<float>(worldTileY * m_tileHeightPixels)};

         const int selfAlpha = m_desirePathsMap.at(worldTileX, worldTileY);

         int neighborAlphaSum = 0;

         if (worldTileX > 0                      ) neighborAlphaSum += m_desirePathsMap.at(worldTileX - 1, worldTileY    );
         if (worldTileX < m_worldMap.width () - 1) neighborAlphaSum += m_desirePathsMap.at(worldTileX + 1, worldTileY    );
         if (worldTileY > 0                      ) neighborAlphaSum += m_desirePathsMap.at(worldTileX    , worldTileY - 1);
         if (worldTileY < m_worldMap.height() - 1) neighborAlphaSum += m_desirePathsMap.at(worldTileX    , worldTileY + 1);

         const auto finalAlpha = static_cast<unsigned char>(std::max(selfAlpha, neighborAlphaSum / 4)); // Could be less than 4 neighbors but let's not care too much about map edges

         const auto color = Color{90, 80, 50, finalAlpha};

         DrawRectangleV(tilePos, {static_cast<float>(m_tileWidthPixels), static_cast<float>(m_tileHeightPixels)}, color);
      }
   }

   EndTextureMode();
}

void DesirePathSim::updateDesirePathsMapTexture(RenderTexture2D& texture)
{
   UpdateRect updateRect{0, 0, m_desirePathsMap.width() - 1, m_desirePathsMap.height() - 1};

   updateDesirePathsMapTexture(texture, updateRect);
}

void DesirePathSim::updateBaseCostMap()
{
   for (std::size_t y = 0; y < m_worldMap.height(); ++y)
   {
      for (std::size_t x = 0; x < m_worldMap.width(); ++x)
      {
         m_baseCostMap.at(x, y) = getBaseCostForValue(m_worldMap.at(x, y), m_baseRNG);
      }
   }
}

void DesirePathSim::updateShadowBitmap()
{
   for (std::size_t y = 0; y < m_worldMap.height(); ++y)
   {
      for (std::size_t x = 0; x < m_worldMap.width(); ++x)
      {
         if (m_worldMap.at(x, y) == TileType::Building || m_worldMap.at(x, y) == TileType::Tree)
         {
            m_shadowBitmap.at(x, y) = 1;
         }
         else
         {
            m_shadowBitmap.at(x, y) = 0;
         }
      }
   }

   auto shadowBitmapDilatedEroded = BitmapTransform::dilate(m_shadowBitmap, false);
   shadowBitmapDilatedEroded = BitmapTransform::erode(shadowBitmapDilatedEroded, false);
   auto shadowBitmapDilatedErodedShifted = BitmapTransform::shift(shadowBitmapDilatedEroded, 1, 1, 0);
   m_shadowBitmap = BitmapTransform::mask(shadowBitmapDilatedErodedShifted, m_shadowBitmap, 0, 0);
}
