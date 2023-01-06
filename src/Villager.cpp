#include "Villager.hpp"

#include <raymath.h>

#include "Voronoi.hpp"
#include "Util.hpp"

void Villager::tick(
   WorldMap& worldMap,
   UpdateRect& mapUpdateRect,
   CostMap& baseCostMap,
   DesirePathsMap& desirePathsMap,
   const bool pavePaths,
   const int tileWidthPixels,
   const int tileHeightPixels,
   std::mt19937_64& rng,
   const float delta
)
{
   const auto state = m_state.load();

   if (state == State::AwaitingPath)
   {
      // NOP
   }
   else if (state == State::EnqueuedForPath)
   {
      // NOP
   }
   else if (state == State::PathProvided)
   {
      assert(m_path.size() >= 2);

      // "Spawn" at first point in path
      m_currentPathIndex = 0;

      const auto startTileX = m_path[m_currentPathIndex].first;
      const auto startTileY = m_path[m_currentPathIndex].second;

      moveOntoTile(startTileX, startTileY, worldMap, mapUpdateRect, baseCostMap, desirePathsMap, pavePaths, tileWidthPixels, tileHeightPixels, rng);

      m_position.x = static_cast<float>(static_cast<int>(startTileX) * tileWidthPixels);
      m_position.y = static_cast<float>(static_cast<int>(startTileY) * tileHeightPixels);

      m_tileToTileMovementStart = m_position;

      const auto nextTileX = m_path[m_currentPathIndex + 1].first;
      const auto nextTileY = m_path[m_currentPathIndex + 1].second;

      m_tileToTileMovementTarget.x = static_cast<float>(static_cast<int>(nextTileX) * tileWidthPixels);
      m_tileToTileMovementTarget.y = static_cast<float>(static_cast<int>(nextTileY) * tileHeightPixels);

      m_tileToTileMovementDirection = Vector2Normalize(Vector2Subtract(m_tileToTileMovementTarget, m_position));

      m_tileToTileMovementDistance = Vector2Distance(m_tileToTileMovementTarget, m_position);

      m_state.store(State::Moving);
   }
   else if (state == State::Moving)
   {
      m_position = Vector2Add(m_position, Vector2Scale(m_tileToTileMovementDirection, m_movementPixelPerSec * delta));

      if (Vector2Distance(m_position, m_tileToTileMovementStart) >= m_tileToTileMovementDistance) // Next tile reached
      {
         m_position = m_tileToTileMovementTarget;

         m_currentPathIndex += 1;

         const auto reachedTileX = m_path[m_currentPathIndex].first;
         const auto reachedTileY = m_path[m_currentPathIndex].second;

         moveOntoTile(reachedTileX, reachedTileY, worldMap, mapUpdateRect, baseCostMap, desirePathsMap, pavePaths, tileWidthPixels, tileHeightPixels, rng);

         if (m_path.size() > m_currentPathIndex + 1)
         {
            m_tileToTileMovementStart = m_position;

            const auto nextTileX = m_path[m_currentPathIndex + 1].first;
            const auto nextTileY = m_path[m_currentPathIndex + 1].second;

            m_tileToTileMovementTarget.x = static_cast<float>(static_cast<int>(nextTileX) * tileWidthPixels);
            m_tileToTileMovementTarget.y = static_cast<float>(static_cast<int>(nextTileY) * tileHeightPixels);

            m_tileToTileMovementDirection = Vector2Normalize(Vector2Subtract(m_tileToTileMovementTarget, m_position));

            m_tileToTileMovementDistance = Vector2Distance(m_tileToTileMovementTarget, m_position);
         }
         else
         {
            m_path.clear();
            m_currentPathIndex = 0;

            m_state.store(State::AwaitingPath);
         }
      }
   }
}

void Villager::moveOntoTile(
   const std::size_t tileX,
   const std::size_t tileY,
   WorldMap& worldMap,
   UpdateRect& mapUpdateRect,
   CostMap& baseCostMap,
   DesirePathsMap& desirePathsMap,
   const bool pavePaths,
   const int tileWidthPixels,
   const int tileHeightPixels,
   std::mt19937_64& rng
)
{
   if (worldMap.at(tileX, tileY) == TileType::Grass)
   {
      const auto currentDesirePathTileStress = adjustDesirePathStress(tileX, tileY, desirePathsMap, baseCostMap, std::uniform_int_distribution<>{2, 6}(rng));

      const auto shouldBePaved = pavePaths && (currentDesirePathTileStress == 255);

      std::vector<std::pair<std::size_t, std::size_t>> neighbors;

      neighbors.reserve(4);

      if (tileY > 0)
      {
         neighbors.emplace_back(tileX, tileY - 1);

         if (tileX > 0)
         {
            neighbors.emplace_back(tileX - 1, tileY - 1);
         }

         if (tileX < worldMap.width() - 1)
         {
            neighbors.emplace_back(tileX + 1, tileY - 1);
         }
      }

      if (tileX > 0)
      {
         neighbors.emplace_back(tileX - 1, tileY);
      }

      if (tileX < worldMap.width() - 1)
      {
         neighbors.emplace_back(tileX + 1, tileY);
      }

      if (tileY < worldMap.height() - 1)
      {
         neighbors.emplace_back(tileX, tileY + 1);

         if (tileX > 0)
         {
            neighbors.emplace_back(tileX - 1, tileY + 1);
         }

         if (tileX < worldMap.width() - 1)
         {
            neighbors.emplace_back(tileX + 1, tileY + 1);
         }
      }

      bool paved = false;

      if (shouldBePaved)
      {
         // Only pave if an adjacent tile is paved
         for (const auto& neighbor : neighbors)
         {
            const auto neighborValue = worldMap.at(neighbor.first, neighbor.second);

            if (neighborValue == TileType::Street || neighborValue == TileType::BuildingEntrance)
            {
               paveTile(tileX, tileY, worldMap, mapUpdateRect, baseCostMap, desirePathsMap, tileWidthPixels, tileHeightPixels, rng);
               paved = true;
               break;
            }
         }
      }

      // Pave neighboring grass
      if (paved)
      {
         for (const auto& neighbor : neighbors)
         {
            if (worldMap.at(neighbor.first, neighbor.second) == TileType::Grass)
            {
               paveTile(neighbor.first, neighbor.second, worldMap, mapUpdateRect, baseCostMap, desirePathsMap, tileWidthPixels, tileHeightPixels, rng);
            }
         }
      }
   }
}

void Villager::reset(
   const WorldMap& worldMap,
   Pathfinder& pathfinder,
   const CostMap& baseCostMap,
   const int tileWidthPixels,
   const int tileHeightPixels,
   std::mt19937_64& rng
)
{
   std::uniform_int_distribution<std::size_t> rngWidth {0, worldMap.width () - 1};
   std::uniform_int_distribution<std::size_t> rngHeight{0, worldMap.height() - 1};

   auto findRandomBuildingEntrance = [&] () -> std::pair<std::size_t, std::size_t>
   {
      return worldMap.find(TileType::BuildingEntrance, rngWidth(rng), rngHeight(rng), true).value_or(std::make_pair(0, 0)); // Fallback, entrances should always exist
   };

   const auto& [spawnX, spawnY] = findRandomBuildingEntrance();

   std::size_t destinationX = 0;
   std::size_t destinationY = 0;

   do
   {
      std::tie(destinationX, destinationY) = findRandomBuildingEntrance();
   }
   while (spawnX == destinationX && spawnY == destinationY); // Spawn point and destination must differ

   m_position.x = static_cast<float>(spawnX * tileWidthPixels );
   m_position.y = static_cast<float>(spawnY * tileHeightPixels);

   m_currentPathIndex = 0;

   auto isBlocked = [] (const TileType tileType)
   {
      switch (tileType)
      {
      case TileType::Street:
      case TileType::BuildingEntrance:
      case TileType::Grass:
      //case TileType::Water:
         return false;
      default:
         return true;
      }
   };

   auto canTraverse = [&] (const std::size_t fromX, const std::size_t fromY, const std::size_t toX, const std::size_t toY)
   {
      if (isBlocked(worldMap.at(toX, toY)))
         return false;

      if (fromX == toX || fromY == toY)
         return true;

      // Diagonal movement is allowed if both "corners" are free
      return ((isBlocked(worldMap.at(fromX, toY)) == false) && (isBlocked(worldMap.at(toX, fromY)) == false));
   };

   auto getTraversalCost = [&] (const std::size_t fromX, const std::size_t fromY, const std::size_t toX, const std::size_t toY)
   {
      const bool isDiagonal = (fromX != toX && fromY != toY);

      const std::uint8_t factor = isDiagonal ? 14 : 10; // (sqrt(2) * 10) or (1 * 10)

      return static_cast<std::int32_t>(factor * baseCostMap.at(toX, toY));
   };

   // See http://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html
   auto heuristic = [&] (const std::size_t fromX, const std::size_t fromY, const std::size_t toX, const std::size_t toY)
   {
      // Diagonal distance (times 10)
      const auto diffX = absDiff(fromX, toX);
      const auto diffY = absDiff(fromY, toY);

      static constexpr std::size_t d = 10; // 1 * 10
      static constexpr std::size_t d2 = 14; // sqrt(2) * 10

      return static_cast<std::int32_t>(d * (diffX + diffY) + (d2 - 2 * d) * std::min(diffX, diffY));
   };

   m_path = pathfinder.getPath(spawnX, spawnY, destinationX, destinationY, canTraverse, getTraversalCost, heuristic);

   m_state.store(State::PathProvided);
}

void Villager::paveTile(
   const std::size_t tileX,
   const std::size_t tileY,
   WorldMap& worldMap,
   UpdateRect& mapUpdateRect,
   CostMap& baseCostMap,
   DesirePathsMap& desirePathsMap,
   const int tileWidthPixels,
   const int tileHeightPixels,
   std::mt19937_64& rng
)
{
   if (worldMap.at(tileX, tileY) != TileType::Street)
   {
      worldMap.at(tileX, tileY) = TileType::Street;

      baseCostMap.at(tileX, tileY) = getBaseCostForValue(TileType::Street, rng);

      desirePathsMap.at(tileX, tileY) = 0;

      mapUpdateRect.add(tileX, tileY);
   }
}
