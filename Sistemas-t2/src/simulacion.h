#pragma once

#include <vector>
#include <mutex>
#include "hero.h"
#include "monster.h"


class Simulacion
{
public:
  // vectores que contienen las instancias activas
  std::vector<Hero> heroes;
  std::vector<Monster> monsters;

  // el mutex global para proteger el acceso a los vectores
  // y a los datos internos de heroes y monstruos (como el hp)
  std::mutex mtx;
};