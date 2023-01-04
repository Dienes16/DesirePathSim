#include "Bitmap.hpp"

namespace BitmapTransform
{
Bitmap erode(const Bitmap& bitmap, const bool includeDiagonals)
{
   Bitmap outMap{bitmap.width(), bitmap.height()};

   for (std::size_t centerY = 0; centerY < bitmap.height(); ++centerY)
   {
      for (std::size_t centerX = 0; centerX < bitmap.width(); ++centerX)
      {
         if (bitmap.at(centerX, centerY) == 1)
         {
            std::uint8_t sum = 0;

            if (centerX > 0)
            {
               if (includeDiagonals)
               {
                  if (centerY > 0)
                  {
                     sum += bitmap.at(centerX - 1, centerY - 1);
                  }

                  if (centerY < bitmap.height() - 1)
                  {
                     sum += bitmap.at(centerX - 1, centerY + 1);
                  }
               }

               sum += bitmap.at(centerX - 1, centerY);
            }

            if (centerX < bitmap.width() - 1)
            {
               if (includeDiagonals)
               {
                  if (centerY > 0)
                  {
                     sum += bitmap.at(centerX + 1, centerY - 1);
                  }

                  if (centerY < bitmap.height() - 1)
                  {
                     sum += bitmap.at(centerX + 1, centerY + 1);
                  }
               }

               sum += bitmap.at(centerX + 1, centerY);
            }

            if (centerY > 0)
            {
               sum += bitmap.at(centerX, centerY - 1);
            }

            if (centerY < bitmap.height() - 1)
            {
               sum += bitmap.at(centerX, centerY + 1);
            }

            if (sum == (includeDiagonals ? 8 : 4))
            {
               // All neighbors were 1
               outMap.at(centerX, centerY) = 1;
            }
            else
            {
               // Some neighbor was 0
               outMap.at(centerX, centerY) = 0;
            }
         }
         else
         {
            outMap.at(centerX, centerY) = 0;
         }
      }
   }

   return outMap;
}

Bitmap dilate(const Bitmap& bitmap, const bool includeDiagonals)
{
   Bitmap outMap{bitmap.width(), bitmap.height()};

   for (std::size_t centerY = 0; centerY < bitmap.height(); ++centerY)
   {
      for (std::size_t centerX = 0; centerX < bitmap.width(); ++centerX)
      {
         if (bitmap.at(centerX, centerY) == 0)
         {
            std::uint8_t sum = 0;

            if (centerX > 0)
            {
               if (includeDiagonals)
               {
                  if (centerY > 0)
                  {
                     sum += bitmap.at(centerX - 1, centerY - 1);
                  }

                  if (centerY < bitmap.height() - 1)
                  {
                     sum += bitmap.at(centerX - 1, centerY + 1);
                  }
               }

               sum += bitmap.at(centerX - 1, centerY);
            }

            if (centerX < bitmap.width() - 1)
            {
               if (includeDiagonals)
               {
                  if (centerY > 0)
                  {
                     sum += bitmap.at(centerX + 1, centerY - 1);
                  }

                  if (centerY < bitmap.height() - 1)
                  {
                     sum += bitmap.at(centerX + 1, centerY + 1);
                  }
               }

               sum += bitmap.at(centerX + 1, centerY);
            }

            if (centerY > 0)
            {
               sum += bitmap.at(centerX, centerY - 1);
            }

            if (centerY < bitmap.height() - 1)
            {
               sum += bitmap.at(centerX, centerY + 1);
            }

            if (sum == 0)
            {
               // All neighbors were 0
               outMap.at(centerX, centerY) = 0;
            }
            else
            {
               // Some neighbor was 1
               outMap.at(centerX, centerY) = 1;
            }
         }
         else
         {
            outMap.at(centerX, centerY) = 1;
         }
      }
   }

   return outMap;
}

Bitmap border(const Bitmap& bitmap, const bool includeDiagonals)
{
   Bitmap outMap{bitmap.width(), bitmap.height()};

   for (std::size_t centerY = 0; centerY < bitmap.height(); ++centerY)
   {
      for (std::size_t centerX = 0; centerX < bitmap.width(); ++centerX)
      {
         if (bitmap.at(centerX, centerY) == 0)
         {
            std::uint8_t sum = 0;

            if (centerX > 0)
            {
               if (includeDiagonals)
               {
                  if (centerY > 0)
                  {
                     sum += bitmap.at(centerX - 1, centerY - 1);
                  }

                  if (centerY < bitmap.height() - 1)
                  {
                     sum += bitmap.at(centerX - 1, centerY + 1);
                  }
               }

               sum += bitmap.at(centerX - 1, centerY);
            }

            if (centerX < bitmap.width() - 1)
            {
               if (includeDiagonals)
               {
                  if (centerY > 0)
                  {
                     sum += bitmap.at(centerX + 1, centerY - 1);
                  }

                  if (centerY < bitmap.height() - 1)
                  {
                     sum += bitmap.at(centerX + 1, centerY + 1);
                  }
               }

               sum += bitmap.at(centerX + 1, centerY);
            }

            if (centerY > 0)
            {
               sum += bitmap.at(centerX, centerY - 1);
            }

            if (centerY < bitmap.height() - 1)
            {
               sum += bitmap.at(centerX, centerY + 1);
            }

            if (sum == 0)
            {
               // All neighbors were 0
               outMap.at(centerX, centerY) = 0;
            }
            else
            {
               // Some neighbor was 1
               outMap.at(centerX, centerY) = 1;
            }
         }
         else
         {
            outMap.at(centerX, centerY) = 0;
         }
      }
   }

   return outMap;
}

Bitmap shift(const Bitmap& bitmap, const int deltaX, const int deltaY, const uint8_t fill)
{
   Bitmap outMap{bitmap.width(), bitmap.height(), fill};

   for (std::size_t y = 0; y < bitmap.height(); ++y)
   {
      for (std::size_t x = 0; x < bitmap.width(); ++x)
      {
         const auto outX = x + deltaX;
         const auto outY = y + deltaY;

         if (outX < 0 || outX >= outMap.width())
            continue;

         if (outY < 0 || outY >= outMap.height())
            continue;

         outMap.at(outX, outY) = bitmap.at(x, y);
      }
   }

   return outMap;
}

Bitmap invert(const Bitmap& bitmap)
{
   Bitmap outMap{bitmap.width(), bitmap.height()};

   for (std::size_t y = 0; y < bitmap.height(); ++y)
   {
      for (std::size_t x = 0; x < bitmap.width(); ++x)
      {
         outMap.at(x, y) = (bitmap.at(x, y) == 0 ? 1 : 0);
      }
   }

   return outMap;
}

Bitmap mask(const Bitmap& bitmap, const Bitmap& mask, const int bitmapBOffsetX, const int bitmapBOffsetY)
{
   const auto overlapRegionX1 = static_cast<std::size_t>(std::max(0, bitmapBOffsetX));
   const auto overlapRegionY1 = static_cast<std::size_t>(std::max(0, bitmapBOffsetY));
   const auto overlapRegionX2 = std::min(bitmap.width(), bitmapBOffsetX + mask.width());
   const auto overlapRegionY2 = std::min(bitmap.height(), bitmapBOffsetY + mask.height());

   Bitmap outMap{bitmap.width(), bitmap.height()};

   for (std::size_t y = 0; y < bitmap.height(); ++y)
   {
      for (std::size_t x = 0; x < bitmap.width(); ++x)
      {
         if (x >= overlapRegionX1 && x <= overlapRegionX2 && y >= overlapRegionY1 && y <= overlapRegionY2)
         {
            if (mask.at(x - bitmapBOffsetX, y - bitmapBOffsetY) == 1)
            {
               outMap.at(x, y) = 0;
            }
            else
            {
               outMap.at(x, y) = bitmap.at(x, y);
            }
         }
         else
         {
            outMap.at(x, y) = bitmap.at(x, y);
         }
      }
   }

   return outMap;
}
} // namespace BitmapTransform
