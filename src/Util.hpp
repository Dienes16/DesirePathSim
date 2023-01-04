#ifndef UTIL_HPP
#define UTIL_HPP

template<typename T>
inline T absDiff(const T a, const T b)
{
   return (a >= b) ? (a - b) : (b - a);
}

#endif // UTIL_HPP
