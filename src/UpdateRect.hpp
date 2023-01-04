#ifndef UPDATERECT_HPP
#define UPDATERECT_HPP

#include <cstdlib>

struct UpdateRect final
{
   std::size_t left   = 0;
   std::size_t top    = 0;
   std::size_t right  = 0;
   std::size_t bottom = 0;

   UpdateRect() = default;

   UpdateRect(const std::size_t left, const std::size_t top, const std::size_t right, const std::size_t bottom);

   void reset();

   bool empty() const;

   void add(const std::size_t x, const std::size_t y);

   std::size_t width() const;
   std::size_t height() const;
};

inline UpdateRect::UpdateRect(const std::size_t left, const std::size_t top, const std::size_t right, const std::size_t bottom):
   left  {left  },
   top   {top   },
   right {right },
   bottom{bottom}
{
   // NOP
}

inline void UpdateRect::reset()
{
   left   = 0;
   top    = 0;
   right  = 0;
   bottom = 0;
}

inline bool UpdateRect::empty() const
{
   return ((left + top + right + bottom) == 0);
}

inline void UpdateRect::add(const std::size_t x, const std::size_t y)
{
   if (empty() == false)
   {
      left   = std::min(left  , x);
      top    = std::min(top   , y);
      right  = std::max(right , x);
      bottom = std::max(bottom, y);
   }
   else
   {
      left   = x;
      top    = y;
      right  = x;
      bottom = y;
   }
}

inline std::size_t UpdateRect::width() const
{
   return (right - left + 1);
}

inline std::size_t UpdateRect::height() const
{
   return (bottom - top + 1);
}

#endif // UPDATERECT_HPP
