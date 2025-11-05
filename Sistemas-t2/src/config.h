#pragma once

#include <string>
#include <vector>
#include "utils.h"

struct HeroConfig
{
  int id;
  int hp;
  int attack_damage;
  int attack_range;
  Coords start;
  std::vector<Coords> path;
};

struct MonsterConfig
{
  int id;
  int hp;
  int attack_damage;
  int vision_range;
  int attack_range;
  Coords coords;
};

struct SimulationConfig
{
  Coords grid_size;
  std::vector<HeroConfig> heroes;     
  std::vector<MonsterConfig> monsters; 
};

SimulationConfig parsear_config(const std::string &filename);