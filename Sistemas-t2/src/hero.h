#pragma once

#include "config.h"
#include <vector>

// forward declaration para evitar inclusion circular
class Simulacion;

class Hero
{
public:
  int id;
  int hp;
  int attack_damage;
  int attack_range;
  Coords pos; // posicion actual
  std::vector<Coords> path;
  int path_index; // para saber en que parte del camino va

  bool vivo;

  // constructor
  Hero(const HeroConfig &config);

  // esta es la funcion que ejecutara el thread del heroe
  void vivir(Simulacion &sim);
};