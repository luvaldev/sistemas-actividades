#pragma once

#include <vector>
#include <mutex>
#include <iostream>
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

  bool juego_activo;

  Simulacion() : juego_activo(true) {}

  // funcion para revisar si el juego debe terminar
  void actualizar_estado_juego()
  {
    // esta funcion debe ser llamada con el mutex tomado
    int heroes_vivos = 0;
    int heroes_escapados = 0;
    for (const auto &h : heroes)
    {
      if (h.vivo)
      {
        heroes_vivos++;
      }
      if (h.escapado)
      {
        heroes_escapados++;
      }
    }

    if (heroes_vivos == 0)
    {
      std::cout << "=== todos los heroes han muerto. ===" << std::endl;
      juego_activo = false;
    }
    else if (heroes_vivos > 0 && heroes_vivos == heroes_escapados)
    {
      std::cout << "=== todos los heroes activos escaparon.===" << std::endl;
      juego_activo = false;
    }
  }
};