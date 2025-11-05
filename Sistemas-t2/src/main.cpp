#include <iostream>
#include <vector>
#include <thread>
#include <memory> // para std::make_shared

#include "config.h"
#include "simulacion.h"
#include "hero.h"
#include "monster.h"

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "error: debes pasar un archivo de configuracion" << std::endl;
    std::cerr << "uso: ./doom_sim <archivo_config.txt>" << std::endl;
    return 1;
  }

  std::string config_file = argv[1];

  SimulationConfig config = parsear_config(config_file);


  // usamos shared_ptr para pasar la simulacion a todos los threads de forma segura
  std::shared_ptr<Simulacion> sim = std::make_shared<Simulacion>();

  // instancias de heroes y monstruos
  for (const auto &hero_config : config.heroes)
  {
    sim->heroes.emplace_back(hero_config);
  }
  for (const auto &monster_config : config.monsters)
  {
    sim->monsters.emplace_back(monster_config);
  }


  std::vector<std::thread> threads;

  // threads de heroes (N heroes)
  for (auto &hero : sim->heroes)
  {
    // pasamos hero por referencia y sim por puntero compartido
    threads.emplace_back([&hero, sim]()
                         { hero.vivir(*sim); });
  }

  // threads de monstruos (M monstruos)
  for (auto &monster : sim->monsters)
  {
    threads.emplace_back([&monster, sim]()
                         { monster.vivir(*sim); });
  }

  std::cout << "simulacion iniciada con " << sim->heroes.size() << " heroes y "
            << sim->monsters.size() << " monstruos." << std::endl;

  for (auto &t : threads)
  {
    t.join();
  }

  std::cout << "simulacion terminada." << std::endl;

  return 0;
}