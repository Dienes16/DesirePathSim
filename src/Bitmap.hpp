#ifndef BITMAP_HPP
#define BITMAP_HPP

#include "Map.hpp"
#include <cstdint>
using Bitmap = Map<std::uint8_t>;

namespace BitmapTransform
{
Bitmap erode(const Bitmap& bitmap, const bool includeDiagonals);

Bitmap dilate(const Bitmap& bitmap, const bool includeDiagonals);

Bitmap border(const Bitmap& bitmap, const bool includeDiagonals);

Bitmap shift(const Bitmap& bitmap, const int deltaX, const int deltaY, const uint8_t fill);

Bitmap invert(const Bitmap& bitmap);

// Sets bitmap to 0 where mask is 1
Bitmap mask(const Bitmap& bitmap, const Bitmap& mask, const int bitmapBOffsetX, const int bitmapBOffsetY);
} // namespace BitmapTransform

#endif // BITMAP_HPP
