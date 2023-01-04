#ifndef VORONOI_HPP
#define VORONOI_HPP

#include <vector>
#include <cmath>

#include <raylib.h>

#include "Map.hpp"

using VoronoiMap = Map<std::size_t>;

struct VoronoiCentroid final
{
   int x;
   int y;
};

using VoronoiCentroidList = std::vector<VoronoiCentroid>;

namespace Voronoi
{
inline float distanceMinkowski(const int aX, const int aY, const int bX, const int bY, const float p)
{
   const auto faX = static_cast<float>(aX);
   const auto faY = static_cast<float>(aY);
   const auto fbX = static_cast<float>(bX);
   const auto fbY = static_cast<float>(bY);

   return static_cast<float>(std::pow(std::pow(std::abs(faX - fbX), p) + std::pow(std::abs(faY - fbY), p), 1.0f / p));
}

inline float distanceManhatten(const int aX, const int aY, const int bX, const int bY)
{
   return distanceMinkowski(aX, aY, bX, bY, 1.0f);
}

inline float distanceEuclidean(const int aX, const int aY, const int bX, const int bY)
{
   return distanceMinkowski(aX, aY, bX, bY, 2.0f);
}

inline float distanceChebyshev(const int aX, const int aY, const int bX, const int bY)
{
   return static_cast<float>(std::max(std::abs(aX - bX), std::abs(aY - bY)));
}

inline bool alwaysTruePredicate(const int, const int)
{
   return true;
}

template<typename PredT = decltype(alwaysTruePredicate)>
VoronoiCentroidList generateCentroids(const int areaLeft, const int areaTop, const int areaWidth, const int areaHeight, const std::size_t count, std::mt19937_64& rng, PredT predicate = alwaysTruePredicate)
{
   VoronoiCentroidList centroidList;

   centroidList.reserve(count);

   const float minDistance = static_cast<float>(areaWidth + areaHeight) / (static_cast<float>(count) * 1.5f);

   for (std::size_t counter = 1; counter <= count; ++counter)
   {
      int x = 0;
      int y = 0;

      float minDistanceToOtherCentroids = 0.0f;

      do
      {
         x = std::uniform_int_distribution<>{areaLeft, areaLeft + areaWidth  - 1}(rng);
         y = std::uniform_int_distribution<>{areaTop , areaTop  + areaHeight - 1}(rng);

         minDistanceToOtherCentroids = std::numeric_limits<float>::max();

         for (const auto& otherCentroid : centroidList)
         {
            const auto distance = distanceEuclidean(x, y, otherCentroid.x, otherCentroid.y);

            if (distance < minDistanceToOtherCentroids)
            {
               minDistanceToOtherCentroids = distance;
            }
         }
      }
      while (predicate(x, y) == false || minDistanceToOtherCentroids < minDistance);

      centroidList.emplace_back(VoronoiCentroid{x, y});
   }

   return centroidList;
}

template<typename DistanceF>
std::size_t getShortestDistanceCentroidIndex(const std::size_t voronoiX, const std::size_t voronoiY, const VoronoiCentroidList& centroidList, DistanceF getDistance)
{
   float shortestDistance = std::numeric_limits<float>::max();
   std::size_t shortestDistanceIndex = 0;

   for (std::size_t index = 0; index < centroidList.size(); ++index)
   {
      const auto& centroid = centroidList[index];

      const auto distance = getDistance(voronoiX, voronoiY, centroid.x, centroid.y);

      if (distance < shortestDistance)
      {
         shortestDistance = distance;
         shortestDistanceIndex = index;
      }
   }

   return shortestDistanceIndex;
}

template<typename DistanceF>
void subdivide(
   VoronoiCentroidList& voronoiCentroidList,
   const std::size_t subdivideCentroidIndex,
   VoronoiMap& voronoiMap,
   const std::size_t centroidCountToAdd,
   std::mt19937_64& rng,
   DistanceF getDistance
)
{
   const auto newCentroidStartIndex = voronoiCentroidList.size();

   // Determine bounding box of Voronoi area

   std::size_t topLeftX = std::numeric_limits<std::size_t>::max();
   std::size_t topLeftY = std::numeric_limits<std::size_t>::max();
   std::size_t bottomRightX = 0;
   std::size_t bottomRightY = 0;

   for (std::size_t voronoiY = 0; voronoiY < voronoiMap.height(); ++voronoiY)
   {
      for (std::size_t voronoiX = 0; voronoiX < voronoiMap.width(); ++voronoiX)
      {
         if (voronoiMap.at(voronoiX, voronoiY) == subdivideCentroidIndex)
         {
            if (voronoiX < topLeftX)
            {
               topLeftX = voronoiX;
            }

            if (voronoiX > bottomRightX)
            {
               bottomRightX = voronoiX;
            }

            if (voronoiY < topLeftY)
            {
               topLeftY = voronoiY;
            }

            if (voronoiY > bottomRightY)
            {
               bottomRightY = voronoiY;
            }
         }
      }
   }

   const auto innerVoronoiMapWidth = bottomRightX - topLeftX + 1;
   const auto innerVoronoiMapHeight = bottomRightY - topLeftY + 1;

   VoronoiMap newVoronoiMap{innerVoronoiMapWidth, innerVoronoiMapHeight};

   // Create new set of Centroids within the chosen area
   const auto newCentroids = Voronoi::generateCentroids(topLeftX, topLeftY, newVoronoiMap.width(), newVoronoiMap.height(), centroidCountToAdd, rng, [&] (const int kiX, const int kiY)
   {
      return (voronoiMap.at(kiX, kiY) == subdivideCentroidIndex);
   });

   for (std::size_t innerVoronoiY = 0; innerVoronoiY < innerVoronoiMapHeight; ++innerVoronoiY)
   {
      for (std::size_t innerVoronoiX = 0; innerVoronoiX < innerVoronoiMapWidth; ++innerVoronoiX)
      {
         newVoronoiMap.at(innerVoronoiX, innerVoronoiY) = getShortestDistanceCentroidIndex(topLeftX + innerVoronoiX, topLeftY + innerVoronoiY, newCentroids, getDistance);
      }
   }
   // Combine inner with outer

   for (std::size_t innerVoronoiY = 0; innerVoronoiY < innerVoronoiMapHeight; ++innerVoronoiY)
   {
      for (std::size_t innerVoronoiX = 0; innerVoronoiX < innerVoronoiMapWidth; ++innerVoronoiX)
      {
         const auto outerVoronoiX = topLeftX + innerVoronoiX;
         const auto outerVoronoiY = topLeftY + innerVoronoiY;

         if (voronoiMap.at(outerVoronoiX, outerVoronoiY) == subdivideCentroidIndex)
         {
            voronoiMap.at(outerVoronoiX, outerVoronoiY) = newVoronoiMap.at(innerVoronoiX, innerVoronoiY) + newCentroidStartIndex;
         }
      }
   }

   // Extend Centroid list

   voronoiCentroidList.reserve(voronoiCentroidList.size() + centroidCountToAdd);
   voronoiCentroidList.insert(voronoiCentroidList.end(), newCentroids.begin(), newCentroids.end());
}
} // namespace Voronoi

#endif // VORONOI_HPP
