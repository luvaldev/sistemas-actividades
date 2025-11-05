# Tarea 2 - (Héroe, Monstruo)

Este proyecto es una simulacion concurrente basada en el juego Doom, utilizando C++ y threads. La simulacion involucra un heroe y varios monstruos que interactuan en un grid 2D, siguiendo reglas especificas de movimiento, vision y combate.

> Estructura del Proyecto

```
|-- src/
|   |-- main.cpp         # punto de entrada, lee config, crea e inicia los threads
|   |-- config.h         # definiciones de estructuras para guardar la config
|   |-- config.cpp       # logica para leer y parsear el archivo .txt
|   |-- simulacion.h     # clase o struct que contiene el estado compartido (grid, entidades) y el mutex
|   |-- entidad.h        # clase base para heroe y monstruo
|   |-- hero.h           # clase para el heroe
|   |-- hero.cpp         # logica del thread del heroe (moverse, atacar)
|   |-- monster.h        # clase para el monstruo
|   |-- monster.cpp      # logica del thread del monstruo (vigilar, alertar, atacar)
|   |-- utils.h          # funciones utiles (ej. distancia manhattan)
|
|-- Makefile             # para compilar todo
|-- README.md            # explicacion de ejecucion (item 8 de la rubrica)
|-- ejemplo1.txt         # (archivo de prueba)
|-- ejemplo2.txt         # (archivo de prueba)
|-- ejemplo3.txt         # (archivo de prueba)
```


## Compilacion
Para compilar el proyecto, necesitas g++ y make.

```bash
make
```