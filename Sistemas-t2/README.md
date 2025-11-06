# Tarea 2 — Héroe y Monstruos 

**Universidad Diego Portales**   
**Curso:** Sistemas Operativos 
**Profesor:** Víctor Reyes 


## Integrantes
- **Nombre:** Luis Valdenegro — **Correo:** luis.valdenegro@mail.udp.cl


## Descripción del programa

El proyecto implementa una **simulación concurrente** basada en *Doom (1993)*, utilizando **C++11** y **threads**.

En un **grid 2D**, un **héroe** se mueve siguiendo una ruta predefinida, mientras varios **monstruos** permanecen pasivos hasta detectar o ser alertados por el héroe.  
Cada entidad (héroe o monstruo) se ejecuta en un **hilo independiente**, compartiendo un estado global protegido por **mutex** para evitar condiciones de carrera.

**Características principales:**
- Un hilo para el héroe y M hilos para los monstruos.  
- Lectura dinámica de parámetros desde archivo `.txt`.  
- Monstruos con rango de visión y ataque; héroe con ruta, daño y vida.  
- Sincronización segura mediante `std::mutex`.  
- Extensión (Parte 2): soporte para múltiples héroes concurrentes con rutas independientes.

---

## Estructura del proyecto

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

---

## Compilación 

En un entorno **WLS/Linux** con `make` y `g++` instalados:

```bash
make
```

Esto generará el ejecutable.

---

## Ejecución

Ejecuta la simulación indicando el archivo de configuración:

```bash
make run
```

Tambien se puede utilizar 

```bash
./doom_sim ejemplo1.txt
```


---

## Funcionamiento resumido

1. Se lee la configuración y se crea el grid.  
2. Se generan los threads del héroe y los monstruos.  
3. El héroe se mueve siguiendo su ruta.  
4. Los monstruos detectan o son alertados, y atacan.  
5. La simulación termina cuando el héroe muere o llega a su meta.

---

## Requisitos del entorno

- Sistema operativo **WLS/Linux**  
- **g++ 11+**  
- Compatible con **POSIX threads (std::thread)**  

---

Universidad Diego Portales, Escuela de Informática y Telecomunicaciones
