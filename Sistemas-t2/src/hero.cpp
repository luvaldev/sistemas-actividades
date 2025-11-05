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
  en_combate = false; 
  escapado = false;   
  std::cout << "heroe " << id << " creado en (" << pos.x << "," << pos.y << ")" << std::endl;
}

// esta es la funcion principal del thread
void Hero::vivir(Simulacion &sim)
{
  while (true) 
  {
    { // --- inicio de la seccion critica ---
      std::lock_guard<std::mutex> lock(sim.mtx);

      // revisar estado
      if (!sim.juego_activo || !vivo || escapado)
      {
        break; // sale del loop y el thread termina
      }

      if (hp <= 0)
      {
        vivo = false;
        std::cout << "heroe " << id << " ha muerto" << std::endl;
        sim.actualizar_estado_juego(); // revisar si el juego termino
        break;
      }

      // logica de combate
      en_combate = false;
      Monster *target_monster = nullptr; // puntero simple al monstruo

      for (auto &monster : sim.monsters) // itera sobre objetos
      {
        if (monster.vivo)
        {
          int dist = distancia_manhattan(pos, monster.pos);
          if (dist <= attack_range)
          {
            en_combate = true;
            target_monster = &monster; // guarda la direccion del monstruo
            break;
          }
        }
      }

      if (en_combate && target_monster)
      {
        std::cout << "heroe " << id << " ataca a monstruo " << target_monster->id
                  << " (hp antes=" << target_monster->hp << ")" << std::endl;

        target_monster->hp -= attack_damage; // ataca usando el puntero

        if (target_monster->hp <= 0)
        {
          target_monster->vivo = false;
          std::cout << "[monstruo " << target_monster->id << "] muere" << std::endl;
        }
        else
        {
          std::cout << "[monstruo " << target_monster->id << "] hp restante: " << target_monster->hp << std::endl;
        }
      }

      // logica de movimiento
      if (!en_combate)
      {
        if (path_index < path.size())
        {
          pos = path[path_index];
          path_index++;
          std::cout << "heroe " << id << " se mueve a (" << pos.x << "," << pos.y << ")" << std::endl;
        }
        else if (!escapado)
        {
          escapado = true;
          std::cout << "heroe " << id << " llego a la meta y escapa" << std::endl;
          sim.actualizar_estado_juego(); // revisar si el juego termino
          break;
        }
      }
    } // --- fin de la seccion critica ---

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  // log de fin de hilo
  std::lock_guard<std::mutex> lock(sim.mtx);
  if (!vivo)
  {
    std::cout << "[heroe " << id << "] termina hilo (muerto)." << std::endl;
  }
  else if (escapado)
  {
    std::cout << "[heroe " << id << "] termina hilo (escapado)." << std::endl;
  }
  else
  {
    std::cout << "[heroe " << id << "] termina hilo (juego terminado)." << std::endl;
  }
}