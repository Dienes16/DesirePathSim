#ifndef MAP_HPP
#define MAP_HPP

#include <vector>
#include <cassert>
#include <optional>

template<typename T>
class Map
{
public:
   Map(const std::size_t width, const std::size_t height, const T& fill = T{});
   Map(std::initializer_list<std::initializer_list<T>> values);

   Map(const Map&) = default;
   Map(Map&&) noexcept = default;

   virtual ~Map() = default;

   Map& operator=(const Map&) = default;
   Map& operator=(Map&&) noexcept = default;

   const T& at(const std::size_t x, const std::size_t y) const;
   T& at(const std::size_t x, const std::size_t y);

   std::size_t width() const;
   std::size_t height() const;

   Map<T> rotated90CW() const;
   Map<T> rotated90CCW() const;
   Map<T> rotated180() const;

   std::optional<std::pair<std::size_t /*foundAtX*/, std::size_t /*foundAtY*/>>
   find(const T value, const std::size_t startX = 0, const std::size_t startY = 0, const bool wrap = false) const;

protected:
   std::size_t m_width;
   std::size_t m_height;

   std::vector<T> m_values;
};

template<typename T>
Map<T>::Map(const std::size_t width, const std::size_t height, const T& fill) :
   m_width{width},
   m_height{height},
   m_values(m_width * m_height, fill)
{
   // NOP
}

template<typename T>
Map<T>::Map(std::initializer_list<std::initializer_list<T>> values) :
   m_width{0},
   m_height{values.size()}
{
   auto minWidth = std::numeric_limits<std::size_t>::max();

   for (const auto& row : values)
   {
      if (row.size() > m_width)
      {
         m_width = row.size();
      }

      if (row.size() < minWidth)
      {
         minWidth = row.size();
      }
   }

   assert(minWidth == m_width);

   m_values.reserve(m_width * m_height);

   for (const auto& row : values)
   {
      for (const auto& column : row)
      {
         m_values.emplace_back(column);
      }
   }
}

template<typename T>
const T& Map<T>::at(const std::size_t x, const std::size_t y) const
{
   return m_values[y * m_width + x];
}

template<typename T>
T& Map<T>::at(const std::size_t x, const std::size_t y)
{
   return m_values[y * m_width + x];
}

template<typename T>
std::size_t Map<T>::width() const
{
   return m_width;
}

template<typename T>
std::size_t Map<T>::height() const
{
   return m_height;
}

template<typename T>
Map<T> Map<T>::rotated90CW() const
{
   Map<T> newMap{m_height, m_width};

   for (std::size_t y = 0; y < m_height; ++y)
   {
      for (std::size_t x = 0; x < m_width; ++x)
      {
         newMap.at(newMap.width() - y - 1, x) = at(x, y);
      }
   }

   return newMap;
}

template<typename T>
Map<T> Map<T>::rotated90CCW() const
{
   Map<T> newMap{m_height, m_width};

   for (std::size_t y = 0; y < m_height; ++y)
   {
      for (std::size_t x = 0; x < m_width; ++x)
      {
         newMap.at(y, newMap.height() - x - 1) = at(x, y);
      }
   }

   return newMap;
}

template<typename T>
Map<T> Map<T>::rotated180() const
{
   Map<T> newMap{m_height, m_width};

   for (std::size_t y = 0; y < m_height; ++y)
   {
      for (std::size_t x = 0; x < m_width; ++x)
      {
         newMap.at(newMap.width() - x - 1, newMap.height() - y - 1) = at(x, y);
      }
   }

   return newMap;
}

template<typename T>
std::optional<std::pair<std::size_t /*foundAtX*/, std::size_t /*foundAtY*/>>
Map<T>::find(const T value, const std::size_t startX, const std::size_t startY, const bool wrap) const
{
   // First row
   if (startY >= m_height || startX >= m_width)
      return std::nullopt;

   for (std::size_t x = startX; x < m_width; ++x)
   {
      if (at(x, startY) == value)
         return std::make_pair(x, startY);
   }

   for (std::size_t y = startY + 1; y < m_height; ++y)
   {
      for (std::size_t x = 0; x < m_width; ++x)
      {
         if (at(x, y) == value)
            return std::make_pair(x, y);
      }
   }

   if (wrap)
   {
      for (std::size_t y = 0; y <= startY; ++y)
      {
         for (std::size_t x = 0; x < (y == startY ? startX : m_width); ++x)
         {
            if (at(x, y) == value)
               return std::make_pair(x, y);
         }
      }
   }

   return std::nullopt;
}

#endif // MAP_HPP
