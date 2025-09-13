# Chat comunitario en C++ 

## 📌 Descripción
Este proyecto implementa un **chat comunitario en C++** utilizando **procesos y pipes (FIFOs)** en un entorno UNIX (Linux/WSL).  
La arquitectura está compuesta por:

1. **Servidor central (`server`)**  
   - Maneja la conexión de todos los clientes.  
   - Difunde los mensajes a todos los participantes.  
   - Mantiene un **log global** de la conversación en `chat.log`.  

2. **Clientes independientes (`client`)**  
   - Procesos **no emparentados** que se comunican con el servidor mediante pipes bidireccionales.  
   - Cada cliente puede:  
     - Enviar mensajes de texto al chat.  
     - Reportar a otros procesos por su `pid`.  
     - Salirse del chat en cualquier momento.  
     - **Clonarse** (`clone`): crear un nuevo proceso cliente que se conecta al servidor como un participante más.  

3. **Moderador (`moderator`)**  
   - Proceso secundario que escucha reportes enviados desde el servidor.  
   - Cuenta los reportes recibidos por cada `pid`.  
   - Si un proceso acumula **más de 10 reportes**, lo mata (`SIGTERM`, y opcionalmente `SIGKILL`).  

> Restricción: Está prohibido el uso de hilos, mutex, semáforos.  
> Solo se utilizan procesos, FIFOs y `select()` para multiplexar entrada/salida.

---

## 🛠️ Requisitos
Compilar y ejecutar en UNIX (Ubuntu o WSL).  

Instala herramientas necesarias:
```bash
sudo apt update
sudo apt install build-essential gdb
```
- `build-essential` instala `g++` y `make`.  
- `gdb` es útil para depurar con VSCode (opcional, recomendado).

---

## 📂 Estructura del proyecto
```
Sistemas-t1/
├─ src/
│  ├─ server.cpp
│  ├─ client.cpp
│  ├─ moderator.cpp
│  └─ common.hpp
├─ Makefile
├─ build/         # se genera al compilar
├─ chat.log       # generado por el servidor
└─ README.md
```

---

## ⚙️ Compilación
En la carpeta raíz del proyecto:
```bash
make
```
Esto genera:
- `./build/server`
- `./build/client`
- `./build/moderator`

---

## ▶️ Ejecución manual (múltiples terminales)
1. **Moderador** (Terminal 1):
   ```bash
   ./build/moderator
   ```

2. **Servidor central** (Terminal 2):
   ```bash
   ./build/server
   ```

3. **Cliente A** (Terminal 3):
   ```bash
   ./build/client
   ```

4. **Cliente B, C, ...** (Terminal 4, 5, ...):
   ```bash
   ./build/client
   ```

### Comandos del cliente
- `<texto>` → se difunde como `MSG`.  
- `reportar <pid>` → envía `REPORT <pid>`.  
- `clone` → crea un nuevo proceso cliente (fork + exec).  
- `salir` → envía `LEAVE` y termina.

---

## 📖 Funcionamiento interno

### Canales y protocolo
- **FIFOs globales**:  
  - Registro de clientes: `/tmp/chat_srv_reg`  
  - Reportes al moderador: `/tmp/chat_reports`  

- **FIFOs por cliente** (dos por cada proceso para bidireccionalidad):  
  - Cliente → Servidor: `/tmp/chat_<pid>_c2s`  
  - Servidor → Cliente: `/tmp/chat_<pid>_s2c`  

- **Mensajes (texto por línea)**:  
  - `JOIN <pid> <fifo_c2s> <fifo_s2c>`  
  - `MSG <pid> <texto...>`  
  - `REPORT <pid>`  
  - `LEAVE <pid>`  

### Flujo
1. El cliente crea sus dos FIFOs y envía `JOIN` por `/tmp/chat_srv_reg`.  
2. El servidor abre los FDs de ese cliente, lo agrega a su tabla y le envía `OK`.  
3. Los `MSG` que recibe el servidor se difunden a todos los clientes y se registran en `chat.log`.  
4. Los `REPORT` se encaminan al moderador, que cuenta y banea si `>10`.  
5. `LEAVE` o cierre del pipe elimina al cliente de la tabla.  
6. `clone` en el cliente hace `fork()`/`exec()` de un nuevo `client` con PID propio, que inicia su propio `JOIN`.

---

## 🧪 Prueba rápida
- En un cliente escribe “hola” → todos lo ven.  
- En otro cliente: `reportar <pid>` (11 veces) → el moderador indica kill del proceso.  
- `clone` en un cliente → aparece un nuevo participante conectado automáticamente.  
- Revisa `chat.log` para ver el historial.

---

## 🧹 Limpieza
Al finalizar, si quedan FIFOs huérfanos:
```bash
rm -f /tmp/chat_* /tmp/chat_srv_reg /tmp/chat_reports
```

---

## ✅ Requisitos cumplidos
- Proceso central con log 
- Clientes independientes + pipes bidireccionales 
- Clonación de clientes
- Moderador con baneo > 10 reportes
- Sin threads/semáforos
- Compila y corre en UNIX/WSL

---

## ℹ️ Datos
- Los códigos usan solo **POSIX**: `fork`, `mkfifo`, `open`, `read/write` no bloqueantes, `select`, `kill`.  
- Se evita bloqueo con `O_NONBLOCK` y apertura en `O_RDWR` para FIFOs.
