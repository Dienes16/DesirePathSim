#ifndef VILLAGER_HPP
#define VILLAGER_HPP

#include <vector>
#include <utility>
#include <atomic>
#include <random>

#include <raylib.h>

#include "WorldMap.hpp"
#include "UpdateRect.hpp"
#include "CostMap.hpp"
#include "DesirePaths.hpp"
#include "Pathfinding.hpp"

class Villager final
{
public:
   enum class State
   {
      AwaitingPath,    // Villager has no path and is not yet enqueued for pathfinding
      EnqueuedForPath, // Villager has been put into pathfinding queue
      PathProvided,    // Villager has been assigned a path but is not yet moving
      Moving           // Villager is moving along its current path
   };

public:
   Villager() = default;

   Villager(const Villager&) = default;
   Villager(Villager&&) noexcept = default;

   ~Villager() = default;

   Villager& operator=(const Villager&) = default;
   Villager& operator=(Villager&&) noexcept = default;

   Vector2 m_position = {0.0f, 0.0f};

   float m_movementPixelPerSec = 50.0f;

   Color m_color = {0, 0, 0, 255};

   State getState() const
   {
      return m_state.load();
   }

   void setState(const State state)
   {
      m_state.store(state);
   }

   void tick(
      WorldMap& worldMap,
      UpdateRect& mapUpdateRect,
      CostMap& baseCostMap,
      DesirePathsMap& desirePathsMap,
      const bool pavePaths,
      const int tileWidthPixels,
      const int tileHeightPixels,
      std::mt19937_64& rng,
      const float delta
   );

   void reset(
      const WorldMap& worldMap,
      Pathfinder& pathfinder,
      const CostMap& baseCostMap,
      const int tileWidthPixels,
      const int tileHeightPixels,
      std::mt19937_64& rng
   );

private:
   std::vector<std::pair<std::size_t, std::size_t>> m_path;

   Vector2 m_tileToTileMovementStart = {0.0f, 0.0f};
   Vector2 m_tileToTileMovementTarget = {0.0f, 0.0f};
   Vector2 m_tileToTileMovementDirection = {0.0f, 0.0f};
   float m_tileToTileMovementDistance = 0.0f;

   std::size_t m_currentPathIndex = 0;

   std::atomic<State> m_state = State::AwaitingPath;

   void moveOntoTile(
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
   );

   void paveTile(
      const std::size_t tileX,
      const std::size_t tileY,
      WorldMap& worldMap,
      UpdateRect& mapUpdateRect,
      CostMap& baseCostMap,
      DesirePathsMap& desirePathsMap,
      const int tileWidthPixels,
      const int tileHeightPixels,
      std::mt19937_64& rng
   );
};

#endif // VILLAGER_HPP
