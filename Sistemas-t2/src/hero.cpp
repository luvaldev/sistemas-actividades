#include "hero.h"
#include "simulacion.h" // incluir la simulacion para el mutex
#include "utils.h"
#include <iostream>
#include <thread> // para std::this_thread::sleep_for
#include <chrono> // para std::chrono::milliseconds

Hero::Hero(const HeroConfig &config)
{
  id = config.id;
  hp = config.hp;
  attack_damage = config.attack_damage;
  attack_range = config.attack_range;
  pos = config.start;
  path = config.path;
  path_index = 0;
  vivo = true;
  std::cout << "heroe " << id << " creado en (" << pos.x << "," << pos.y << ")" << std::endl;
}

// esta es la funcion principal del thread
void Hero::vivir(Simulacion &sim)
{
  while (vivo)
  {
    // --- inicio de la seccion critica ---
    std::lock_guard<std::mutex> lock(sim.mtx);

    if (hp <= 0)
    {
      vivo = false;
      std::cout << "heroe " << id << " ha muerto" << std::endl;
      break; // sale del loop y el thread termina
    }

    // logica de combate
    bool monstruo_cerca = false;
    for (auto &monster : sim.monsters)
    {
      if (monster.vivo)
      {
        int dist = distancia_manhattan(pos, monster.pos);
        if (dist <= attack_range)
        {
          monstruo_cerca = true;
          std::cout << "heroe " << id << " ataca a monstruo " << monster.id << std::endl;
          monster.hp -= attack_damage;
        }
      }
    }

    // logica de movimiento
    if (!monstruo_cerca && path_index < path.size())
    {
      pos = path[path_index];
      path_index++;
      std::cout << "heroe " << id << " se mueve a (" << pos.x << "," << pos.y << ")" << std::endl;
    }
    else if (!monstruo_cerca && path_index >= path.size())
    {
      std::cout << "heroe " << id << " llego a la meta" << std::endl;
      vivo = false; // termino su camino
      break;
    }

    // --- fin de la seccion critica ---

    // liberamos el mutex y dormimos un rato
    // para que otros threads puedan ejecutarse
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
}