#include "monster.h"
#include "simulacion.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <limits>  
#include <memory>  

Monster::Monster(const MonsterConfig &config)
{
  id = config.id;
  hp = config.hp;
  attack_damage = config.attack_damage;
  vision_range = config.vision_range;
  attack_range = config.attack_range;
  pos = config.coords;
  vivo = true;
  alertado = false; // todos empiezan en estado pasivo
  target_hero_id = -1;
  std::cout << "monstruo " << id << " creado en (" << pos.x << "," << pos.y << ")" << std::endl;
}

// funcion interna para buscar el heroe mas cercano
static int encontrar_heroe_cercano(Monster *m, Simulacion &sim)
{
  int target_id = -1;
  int dist_min = std::numeric_limits<int>::max();

  for (const auto &hero : sim.heroes)
  {
    if (hero.vivo && !hero.escapado)
    {
      int d = distancia_manhattan(m->pos, hero.pos);
      if (d < dist_min)
      {
        dist_min = d;
        target_id = hero.id;
      }
    }
  }
  return target_id;
}

void Monster::vivir(Simulacion &sim)
{
  while (true)
  {

    {
      std::lock_guard<std::mutex> lock(sim.mtx);

      if (!sim.juego_activo || !vivo)
      {
        break;
      }

      if (hp <= 0)
      {
        vivo = false;
        std::cout << "monstruo " << id << " ha muerto" << std::endl;
        break; // sale del loop y el thread termina
      }

      // logica de vision y alerta
      if (!alertado)
      {
        int heroe_visto_id = -1;
        int dist_min = std::numeric_limits<int>::max();

        for (const auto &hero : sim.heroes)
        {
          if (hero.vivo && !hero.escapado) 
          {
            int dist = distancia_manhattan(pos, hero.pos); 
            if (dist <= vision_range && dist < dist_min)
            {
              dist_min = dist;
              heroe_visto_id = hero.id; 
            }
          }
        }

        if (heroe_visto_id != -1)
        {
          alertado = true;
          target_hero_id = heroe_visto_id; 
          std::cout << "[monstruo " << id << "] ve al heroe " << heroe_visto_id << " a distancia " << dist_min << std::endl;

          // alertar a otros monstruos
          for (auto &otro_monstruo : sim.monsters)
          {
            if (otro_monstruo.id != id && otro_monstruo.vivo && !otro_monstruo.alertado) 
            {
              if (distancia_manhattan(pos, otro_monstruo.pos) <= vision_range) 
              {
                std::cout << "[monstruo " << id << "] alerta a monstruo " << otro_monstruo.id << std::endl; 
                otro_monstruo.alertado = true; 
              }
            }
          }
        }
      }

      // logica de movimiento y ataque (si esta alertado)
      if (alertado)
      {
        // validar objetivo
        Hero *target = nullptr; 
        if (target_hero_id != -1)
        {
          for (auto &h : sim.heroes) // itera sobre objetos
          {
            if (h.id == target_hero_id && h.vivo && !h.escapado) 
            {
              target = &h; 
              break;
            }
          }
        }

        // si el objetivo no es valido, buscar uno nuevo
        if (target == nullptr)
        {
          int nuevo_target_id = encontrar_heroe_cercano(this, sim);
          if (nuevo_target_id == -1)
          {
            // no hay heroes vivos, volver a dormir
            alertado = false;
            target_hero_id = -1;
          }
          else
          {
            target_hero_id = nuevo_target_id;
            // volvemos a buscar el puntero en la proxima iteracion
          }
        }
        else
        {
          // objetivo valido, decidir accion
          int d = distancia_manhattan(pos, target->pos);

          if (d <= attack_range)
          {
            // atacar
            std::cout << "[monstruo " << id << "] ataca a heroe " << target->id << " (hp antes: " << target->hp << ")" << std::endl;
            target->hp -= attack_damage;
            if (target->hp <= 0)
            {
              target->vivo = false;
              std::cout << "[heroe " << target->id << "] muere por monstruo " << id << std::endl;
              sim.actualizar_estado_juego(); // revisar si termino
            }
            else
            {
              std::cout << "[heroe " << target->id << "] hp restante: " << target->hp << std::endl;
            }
          }
          else
          {
            // moverse (ruta manhattan)
            if (target->pos.x > pos.x)
              pos.x++;
            else if (target->pos.x < pos.x)
              pos.x--;
            else if (target->pos.y > pos.y)
              pos.y++;
            else if (target->pos.y < pos.y)
              pos.y--;

            std::cout << "[monstruo " << id << "] avanza hacia heroe " << target->id << " -> (" << pos.x << "," << pos.y << ")" << std::endl;
          }
        }
      }
    } 

    // dormir
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // log de fin de hilo
  std::lock_guard<std::mutex> lock(sim.mtx);
  if (!vivo)
  {
    std::cout << "[monstruo " << id << "] termina hilo (muerto)." << std::endl;
  }
  else
  {
    std::cout << "[monstruo " << id << "] termina hilo (juego terminado)." << std::endl;
  }
}