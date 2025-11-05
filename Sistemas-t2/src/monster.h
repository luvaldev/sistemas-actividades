#pragma once

#include "config.h"

// forward declaration
class Simulacion;

class Monster
{
public:
  int id;
  int hp;
  int attack_damage;
  int vision_range;
  int attack_range;
  Coords pos;

  bool vivo;
  bool alertado; // estado pasivo o activo

  Monster(const MonsterConfig &config);

  // funcion que ejecutara el thread del monstruo
  void vivir(Simulacion &sim);
};