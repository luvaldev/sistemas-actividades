#pragma once

#include <cmath>
#include <string> 
#include <cctype> 

// estructura simple para coordenadas
struct Coords
{
  int x = 0; 
  int y = 0; 
};

// calcula la distancia de manhattan
inline int distancia_manhattan(Coords a, Coords b)
{
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}


// helper para normalizar keys (ej. "hero_1_hp" -> "hero1hp")
inline std::string normalize_key(const std::string &s)
{
  std::string out = "";
  for (char c : s)
  {
    if (c != '_' && c != '-')
    {
      out += std::tolower(c);
    }
  }
  return out;
}