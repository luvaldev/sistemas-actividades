#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <cstring>

namespace chat
{

  // rutas fijas del sistema para hablar con server y moderador
  static const std::string REG_FIFO = "/tmp/chat_srv_reg";     // donde los clientes mandan join
  static const std::string REPORTS_FIFO = "/tmp/chat_reports"; // donde el server envia reportes al moderador

  // crea una fifo si no existe, y revienta el programa si falla
  inline void mkfifo_if_needed(const std::string &path, mode_t mode = 0666)
  {
    if (mkfifo(path.c_str(), mode) == -1 && errno != EEXIST)
    {
      std::perror(("mkfifo " + path).c_str());
      _exit(1);
    }
  }

  // abre una fifo en lectura y escritura no bloqueante (truco para evitar bloqueos)
  inline int open_rdwr_nonblock(const std::string &path)
  {
    int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd == -1)
      std::perror(("open " + path).c_str());
    return fd;
  }

  // abre una fifo solo lectura no bloqueante
  inline int open_read_nonblock(const std::string &path)
  {
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1)
      std::perror(("open " + path).c_str());
    return fd;
  }

  // abre una fifo solo escritura no bloqueante
  inline int open_write_nonblock(const std::string &path)
  {
    int fd = open(path.c_str(), O_WRONLY | O_NONBLOCK);
    if (fd == -1)
      std::perror(("open " + path).c_str());
    return fd;
  }

  // escribe una linea terminada en '\n' en el fd. devuelve true si se escribio todo
  inline bool write_line(int fd, const std::string &line)
  {
    std::string data = line + "\n";
    ssize_t n = write(fd, data.c_str(), data.size());
    if (n == -1 && errno != EAGAIN)
    {
      std::perror("write");
      return false;
    }
    return n == (ssize_t)data.size();
  }

  // lee hasta encontrar '\n' y pone el resultado en out
  // retorna true si obtuvo una linea completa (o un fragmento grande), false si el fd se cerro sin datos
  inline bool read_line(int fd, std::string &out)
  {
    out.clear();
    char ch;
    ssize_t n;
    while (true)
    {
      n = read(fd, &ch, 1);
      if (n == 1)
      {
        if (ch == '\n')
          return true; // fin de linea
        out.push_back(ch);
        if (out.size() > 8192)
          return true; // corta por si llega basura infinita
      }
      else if (n == 0)
      {
        // el extremo remoto cerro la fifo
        if (out.empty())
          return false; // no alcanzo a leer nada
        return true;    // devuelve lo que alcanzo
      }
      else
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          // no hay mas datos ahora mismo
          return !out.empty(); // devuelve true si quedo algo parcial
        }
        else if (errno == EINTR)
        {
          // senal interrumpio la lectura, reintenta
          continue;
        }
        else
        {
          std::perror("read");
          return false;
        }
      }
    }
  }

  // separa por espacios en blanco en tokens simples
  inline std::vector<std::string> split_ws(const std::string &s)
  {
    std::istringstream iss(s);
    std::vector<std::string> v;
    std::string tok;
    while (iss >> tok)
      v.push_back(tok);
    return v;
  }

  // genera paths de las fifos por cliente, una para cliente->server y otra para server->cliente
  inline std::string fifo_c2s(pid_t pid) { return "/tmp/chat_" + std::to_string(pid) + "_c2s"; }
  inline std::string fifo_s2c(pid_t pid) { return "/tmp/chat_" + std::to_string(pid) + "_s2c"; }

} // namespace chat
