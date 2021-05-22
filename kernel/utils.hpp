#pragma once
#include <algorithm>

template <typename T, typename U>
void Erase(T& c, const U& value) {
  auto it = std::remove(c.begin(), c.end(), value);
  c.erase(it, c.end());
}
