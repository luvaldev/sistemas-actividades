#pragma once

#include <cmath>

// estructura simple para coordenadas
struct Coords
{
  int x;
  int y;
};

// calcula la distancia de manhattan
inline int distancia_manhattan(Coords a, Coords b)
{
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}