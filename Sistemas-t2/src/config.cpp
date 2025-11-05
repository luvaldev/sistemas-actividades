#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>


SimulationConfig parsear_config(const std::string &filename)
{
  SimulationConfig config;
  std::ifstream file(filename);
  std::string line;

  while (std::getline(file, line))
  {
    std::stringstream ss(line);
    std::string key;
    ss >> key;

    if (key == "GRID_SIZE")
    { 
      ss >> config.grid_size.x >> config.grid_size.y;
    }

  }

  std::cout << "configuracion leida de " << filename << std::endl;
  return config;
}