#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype> // para isdigit
#include <map>    // para manejar ids
#include "utils.h"

SimulationConfig parsear_config(const std::string &filename)
{
  SimulationConfig config;
  std::ifstream file(filename);
  if (!file.is_open())
  {
    std::cerr << "error: no se pudo abrir el archivo: " << filename << std::endl;
    exit(1);
  }

  // mapas temporales para guardar por id
  std::map<int, HeroConfig> hero_map;
  std::map<int, MonsterConfig> monster_map;

  std::string line;
  std::string current_key;
  std::stringstream ss_line;

  // funcion interna para leer el proximo valor
  auto get_next_value = [&]() -> std::string
  {
    std::string val;
    ss_line >> val;
    // si esta vacio, lee la siguiente linea
    while (ss_line.fail() || val.empty() || (val.rfind("#", 0) == 0))
    {
      if (!std::getline(file, line))
        return ""; // fin de archivo
      ss_line.clear();
      ss_line.str(line);
      ss_line >> val;
    }
    return val;
  };

  // funcion para leer paths
  auto parse_path = [&](std::vector<Coords> &path)
  {
    while (true)
    {
      // revisa la proxima palabra sin consumirla
      std::string next_val;

      std::streampos old_pos = ss_line.tellg(); // guardar posicion
      ss_line >> next_val;
      if (ss_line.fail() || next_val.empty() || (next_val.rfind("#", 0) == 0))
      {
        if (!std::getline(file, line))
          break;
        ss_line.clear();
        ss_line.str(line);
        old_pos = ss_line.tellg(); // guardar nueva pos
        ss_line >> next_val;
        if (ss_line.fail() || next_val.empty() || (next_val.rfind("#", 0) == 0))
          break;
      }

      if (next_val[0] == '(')
      {
        int x, y;
        sscanf(next_val.c_str(), "(%d,%d)", &x, &y);
        path.push_back(Coords{x, y});
      }
      else
      {
        // la "devolvemos" al stream
        ss_line.seekg(old_pos);
        break;
      }
    }
  };

  while (true)
  {
    current_key = get_next_value();
    if (current_key.empty())
      break; // fin del archivo

    std::string norm_key = normalize_key(current_key);

    if (norm_key == "gridsize")
    {
      config.grid_size.x = std::stoi(get_next_value());
      config.grid_size.y = std::stoi(get_next_value());
    }
    else if (norm_key == "monstercount")
    {
      // contaremos los monstruos por sus ids
      (void)std::stoi(get_next_value());
    }
    else if (norm_key == "herohp")
    {
      hero_map[1].id = 1;
      hero_map[1].hp = std::stoi(get_next_value());
    }
    else if (norm_key == "heroattackdamage")
    {
      hero_map[1].id = 1;
      hero_map[1].attack_damage = std::stoi(get_next_value());
    }
    else if (norm_key == "heroattackrange")
    {
      hero_map[1].id = 1;
      hero_map[1].attack_range = std::stoi(get_next_value());
    }
    else if (norm_key == "herostart")
    {
      hero_map[1].id = 1;
      hero_map[1].start.x = std::stoi(get_next_value());
      hero_map[1].start.y = std::stoi(get_next_value());
    }
    else if (norm_key == "heropath")
    {
      hero_map[1].id = 1;
      parse_path(hero_map[1].path);
    }
    else
    {
      int id = 0;
      std::string prop;
      const char *p = norm_key.c_str();

      if (norm_key.rfind("hero", 0) == 0)
      {
        p += 4; // saltar "hero"
        while (*p && isdigit(*p))
        {
          id = id * 10 + (*p++ - '0');
        }
        prop = p;
        if (id > 0)
        {
          hero_map[id].id = id; // asegura que el id este
          if (prop == "hp")
            hero_map[id].hp = std::stoi(get_next_value());
          else if (prop == "attackdamage")
            hero_map[id].attack_damage = std::stoi(get_next_value());
          else if (prop == "attackrange")
            hero_map[id].attack_range = std::stoi(get_next_value());
          else if (prop == "start")
          {
            hero_map[id].start.x = std::stoi(get_next_value());
            hero_map[id].start.y = std::stoi(get_next_value());
          }
          else if (prop == "path")
          {
            parse_path(hero_map[id].path);
          }
        }
      }
      else if (norm_key.rfind("monster", 0) == 0)
      {
        p += 7; // saltar "monster"
        while (*p && isdigit(*p))
        {
          id = id * 10 + (*p++ - '0');
        }
        prop = p;
        if (id > 0)
        {
          monster_map[id].id = id; // asegura que el id este
          if (prop == "hp")
            monster_map[id].hp = std::stoi(get_next_value());
          else if (prop == "attackdamage")
            monster_map[id].attack_damage = std::stoi(get_next_value());
          else if (prop == "visionrange")
            monster_map[id].vision_range = std::stoi(get_next_value());
          else if (prop == "attackrange")
            monster_map[id].attack_range = std::stoi(get_next_value());
          else if (prop == "coords")
          {
            monster_map[id].coords.x = std::stoi(get_next_value());
            monster_map[id].coords.y = std::stoi(get_next_value());
          }
        }
      }
    }
  }

  // copiar de los mapas a los vectores
  for (auto const &[id, hero_conf] : hero_map)
  {
    config.heroes.push_back(hero_conf);
  }
  for (auto const &[id, mon_conf] : monster_map)
  {
    config.monsters.push_back(mon_conf);
  }

  std::cout << "configuracion leida: " << config.heroes.size() << " heroes, "
            << config.monsters.size() << " monstruos." << std::endl;
  return config;
}