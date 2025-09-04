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

  // Rutas conocidas
  static const std::string REG_FIFO = "/tmp/chat_srv_reg";
  static const std::string REPORTS_FIFO = "/tmp/chat_reports";

  // Helpers para FIFOs
  inline void mkfifo_if_needed(const std::string &path, mode_t mode = 0666)
  {
    if (mkfifo(path.c_str(), mode) == -1 && errno != EEXIST)
    {
      std::perror(("mkfifo " + path).c_str());
      _exit(1);
    }
  }

  inline int open_rdwr_nonblock(const std::string &path)
  {
    int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd == -1)
    {
      std::perror(("open " + path).c_str());
    }
    return fd;
  }

  inline int open_read_nonblock(const std::string &path)
  {
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1)
      std::perror(("open " + path).c_str());
    return fd;
  }

  inline int open_write_nonblock(const std::string &path)
  {
    int fd = open(path.c_str(), O_WRONLY | O_NONBLOCK);
    if (fd == -1)
      std::perror(("open " + path).c_str());
    return fd;
  }

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

  // Lee hasta '\n'. Devuelve true si obtuvo línea completa.
  // Si fd se cerró, devuelve false y deja out vacío.
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
          return true;
        out.push_back(ch);
        // Evita líneas absurdamente largas
        if (out.size() > 8192)
          return true;
      }
      else if (n == 0)
      {
        // pipe cerrado
        if (out.empty())
          return false;
        return true;
      }
      else
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          // no hay más por ahora
          return !out.empty(); // puede devolver línea parcial
        }
        else if (errno == EINTR)
        {
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

  inline std::vector<std::string> split_ws(const std::string &s)
  {
    std::istringstream iss(s);
    std::vector<std::string> v;
    std::string tok;
    while (iss >> tok)
      v.push_back(tok);
    return v;
  }

  inline std::string fifo_c2s(pid_t pid) { return "/tmp/chat_" + std::to_string(pid) + "_c2s"; }
  inline std::string fifo_s2c(pid_t pid) { return "/tmp/chat_" + std::to_string(pid) + "_s2c"; }

} // namespace chat
