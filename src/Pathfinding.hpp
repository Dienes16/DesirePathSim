#ifndef PATHFINDING_HPP
#define PATHFINDING_HPP

#include <vector>
#include <cassert>
#include <algorithm>
#include <memory>

#include "Util.hpp"

class Pathfinder final
{
private:
   struct PathNode final
   {
      PathNode* m_parent = nullptr;

      std::size_t m_x = 0;
      std::size_t m_y = 0;

      bool m_inOpenList = false;
      bool m_closed = false;

      std::size_t m_openListIndex = 0;

      std::int32_t m_f = 0;
      std::int32_t m_g = 0;
      std::int32_t m_h = 0;

      PathNode() = default;

      PathNode(const std::size_t x, const std::size_t y):
         m_x{x},
         m_y{y}
      {
         // NOP
      }
   };

public:
   static inline bool alwaysTraversable(const std::size_t /*fromX*/, const std::size_t /*fromY*/, const std::size_t /*toX*/, const std::size_t /*toY*/)
   {
      return true;
   }

   static inline std::uint32_t zeroCost(const std::size_t /*fromX*/, const std::size_t /*fromY*/, const std::size_t /*toX*/, const std::size_t /*toY*/)
   {
      return 0;
   }

   static inline std::uint32_t defaultHeuristic(const std::size_t fromX, const std::size_t fromY, const std::size_t toX, const std::size_t toY)
   {
      // Chebyshev distance (diagonals are counted as same distance as orthogonals)

      const auto diffX = absDiff(fromX, toX);
      const auto diffY = absDiff(fromY, toY);

      return static_cast<std::uint32_t>(std::max(diffX, diffY));
   }

private:
   std::size_t m_width;
   std::size_t m_height;

   std::vector<PathNode> m_pathNodeMap;

   std::vector<PathNode*> m_openList;

public:
   Pathfinder(const std::size_t width, const std::size_t height):
      m_width{width},
      m_height{height}
   {
      m_pathNodeMap.resize(m_width * m_height);

      for (std::size_t y = 0; y < m_height; ++y)
      {
         for (std::size_t x = 0; x < m_width; ++x)
         {
            m_pathNodeMap[y * m_width + x].m_x = x;
            m_pathNodeMap[y * m_width + x].m_y = y;
         }
      }
   }

   template<typename CanTraverseCallable = decltype(alwaysTraversable), typename TraversalCostCallable = decltype(zeroCost), typename HeuristicCallable = decltype(defaultHeuristic)>
   std::vector<std::pair<std::size_t, std::size_t>> getPath(
      const std::size_t startX, const std::size_t startY,
      const std::size_t endX, const std::size_t endY,
      CanTraverseCallable canTraverse = alwaysTraversable,
      TraversalCostCallable getTraversalCost = zeroCost,
      HeuristicCallable heuristic = defaultHeuristic
   )
   {
      bool success = false;

      auto& startPathNode = getPathNodeAt(startX, startY);
      auto& endPathNode   = getPathNodeAt(endX  , endY  );

      // Make some room for the nodes using heuristic distance
      m_openList.reserve(std::max(absDiff(startPathNode.m_x, endPathNode.m_x), absDiff(startPathNode.m_y, endPathNode.m_y)));

      m_openList.emplace_back(&startPathNode);

      startPathNode.m_inOpenList = true;
      startPathNode.m_openListIndex = 0;

      // As we work through the open, we do not pop anything from the front, instead we increase the starting position.
      // This ensures consistent indices and no overhead caused by memory shifts when popping. It comes with the cost of
      // having to store more data in the vector.

      std::size_t openListStartingPosition = 0;

      while (openListStartingPosition < m_openList.size())
      {
         PathNode& smallestFPathNode = *m_openList[openListStartingPosition];

         openListStartingPosition += 1;

         smallestFPathNode.m_inOpenList = false;
         smallestFPathNode.m_closed = true;

         if (smallestFPathNode.m_x == endPathNode.m_x && smallestFPathNode.m_y == endPathNode.m_y)
         {
            success = true;
            break;
         }

         auto handleAdjacentPathNode = [&] (PathNode& adjacentPathNode)
         {
            if (adjacentPathNode.m_closed || canTraverse(smallestFPathNode.m_x, smallestFPathNode.m_y, adjacentPathNode.m_x, adjacentPathNode.m_y) == false)
               return;

            // G is the distance from the start node to this node plus any costs for traversing this node
            const std::int32_t tempG = smallestFPathNode.m_g + getTraversalCost(smallestFPathNode.m_x, smallestFPathNode.m_y, adjacentPathNode.m_x, adjacentPathNode.m_y);

            if (adjacentPathNode.m_inOpenList == false)
            {
               adjacentPathNode.m_parent = &smallestFPathNode;

               // Set G
               adjacentPathNode.m_g = tempG;

               // H is the heuristic distance to the end node, it has to be less than or equal to the actual cost neccessary
               adjacentPathNode.m_h = heuristic(adjacentPathNode.m_x, adjacentPathNode.m_y, endX, endY);

               // F is the sum of G and H
               adjacentPathNode.m_f = adjacentPathNode.m_g + adjacentPathNode.m_h;

               adjacentPathNode.m_inOpenList = true;

               // Find out where to insert
               const auto insertionPoint =
                  std::upper_bound(
                     std::begin(m_openList) + openListStartingPosition,
                     std::end(m_openList),
                     &adjacentPathNode,
                     [] (const auto pathNodeA, const auto pathNodeB) { return (pathNodeA->m_f < pathNodeB->m_f); }
               );

               if (insertionPoint == std::end(m_openList))
               {
                  // We can just push it back at the end, its value is the largest so far

                  // Store the index in the PathNode
                  adjacentPathNode.m_openListIndex = m_openList.size();

                  m_openList.emplace_back(&adjacentPathNode);
               }
               else
               {
                  const std::size_t insertionIndex = std::distance(std::begin(m_openList) + openListStartingPosition, insertionPoint) + openListStartingPosition;

                  // Store the new index in the PathNode
                  adjacentPathNode.m_openListIndex = insertionIndex;

                  // Inserting into the middle
                  m_openList.insert(insertionPoint, &adjacentPathNode);
               }
            }
            else
            {
               // Better G?
               if (tempG < adjacentPathNode.m_g)
               {
                  adjacentPathNode.m_parent = &smallestFPathNode;

                  adjacentPathNode.m_g = tempG;
                  adjacentPathNode.m_f = adjacentPathNode.m_g + adjacentPathNode.m_h;

                  // The entry might have to move up in the list to keep the list sorted

                  auto insertionIndex = adjacentPathNode.m_openListIndex;

                  // Counting downwards until we wrap around
                  while ((--insertionIndex < (adjacentPathNode.m_openListIndex + 1)) && (insertionIndex >= openListStartingPosition))
                  {
                     if (m_openList[insertionIndex]->m_f <= adjacentPathNode.m_f)
                        break; // We reached the insertion point
                  }

                  if (insertionIndex < adjacentPathNode.m_openListIndex)
                  {
                     // Store the new index in the PathNode
                     adjacentPathNode.m_openListIndex = insertionIndex;

                     m_openList.insert(std::begin(m_openList) + insertionIndex, &adjacentPathNode);
                  }
               }
            }
         };

         if (smallestFPathNode.m_y > 0)
         {
            handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x, smallestFPathNode.m_y - 1));

            if (smallestFPathNode.m_x > 0)
            {
               handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x - 1, smallestFPathNode.m_y - 1));
            }

            if (smallestFPathNode.m_x < m_width - 1)
            {
               handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x + 1, smallestFPathNode.m_y - 1));
            }
         }

         if (smallestFPathNode.m_y < m_height - 1)
         {
            handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x, smallestFPathNode.m_y + 1));

            if (smallestFPathNode.m_x > 0)
            {
               handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x - 1, smallestFPathNode.m_y + 1));
            }

            if (smallestFPathNode.m_x < m_width - 1)
            {
               handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x + 1, smallestFPathNode.m_y + 1));
            }
         }

         if (smallestFPathNode.m_x > 0)
         {
            handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x - 1, smallestFPathNode.m_y));
         }

         if (smallestFPathNode.m_x < m_width - 1)
         {
            handleAdjacentPathNode(getPathNodeAt(smallestFPathNode.m_x + 1, smallestFPathNode.m_y));
         }
      }

      if (success)
      {
         std::vector<std::pair<std::size_t, std::size_t>> mapNodes;

         mapNodes.reserve(m_width + m_height);

         PathNode* node = &endPathNode;

         mapNodes.emplace_back(node->m_x, node->m_y);

         while (node->m_parent != nullptr)
         {
            mapNodes.emplace_back(node->m_parent->m_x, node->m_parent->m_y);
            node = node->m_parent;
         }

         std::reverse(std::begin(mapNodes), std::end(mapNodes));

         clearOpenList();

         return mapNodes;
      }

      clearOpenList();

      return {};
   }

private:
   PathNode& getPathNodeAt(const std::size_t x, const std::size_t y)
   {
      return m_pathNodeMap[y * m_width + x];
   }

   void clearOpenList()
   {
      for (auto* const pathNode : m_openList)
      {
         pathNode->m_parent = nullptr;

         pathNode->m_inOpenList = false;
         pathNode->m_closed = false;
      }

      m_openList.clear();
   }
};

#endif // PATHFINDING_HPP
