#pragma once

#include "config.h"
#include <vector>
#include <iostream>

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
  size_t path_index; 

  bool vivo;
  bool en_combate; 
  bool escapado;   

  // constructor
  Hero(const HeroConfig &config);

  // esta es la funcion que ejecutara el thread del heroe
  void vivir(Simulacion &sim);
};