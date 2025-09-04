#include "common.hpp"
#include <unordered_map>
#include <algorithm>
#include <fstream>

using namespace chat;

struct Client
{
  pid_t pid{};
  int fd_c2s{-1}; // lectura desde el cliente
  int fd_s2c{-1}; // escritura hacia el cliente
};

static std::unordered_map<pid_t, Client> clients;
static int fd_reg = -1;     // registro (JOIN)
static int fd_reports = -1; // escribir REPORT para moderador
static std::ofstream logf;

static volatile sig_atomic_t stop_flag = 0;
static void on_sigint(int) { stop_flag = 1; }

static void broadcast(const std::string &line)
{
  for (auto &[pid, c] : clients)
  {
    if (c.fd_s2c != -1)
      write_line(c.fd_s2c, line);
  }
}

static void remove_client(pid_t pid)
{
  auto it = clients.find(pid);
  if (it == clients.end())
    return;
  if (it->second.fd_c2s != -1)
    close(it->second.fd_c2s);
  if (it->second.fd_s2c != -1)
    close(it->second.fd_s2c);
  clients.erase(it);
}

int main()
{
  // señales para limpieza ordenada
  struct sigaction sa{};
  sa.sa_handler = on_sigint;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  mkfifo_if_needed(REG_FIFO);
  mkfifo_if_needed(REPORTS_FIFO);

  fd_reg = open_read_nonblock(REG_FIFO);
  if (fd_reg == -1)
    return 1;

  // Abrimos reports en RDWR para que no bloquee si moderador aún no está
  fd_reports = open_rdwr_nonblock(REPORTS_FIFO);
  if (fd_reports == -1)
    return 1;

  logf.open("chat.log", std::ios::app);

  std::cout << "[SERVER] listo. Leyendo JOIN en " << REG_FIFO << std::endl;

  while (!stop_flag)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd_reg, &rfds);
    int maxfd = fd_reg;

    for (auto &[pid, c] : clients)
    {
      if (c.fd_c2s != -1)
      {
        FD_SET(c.fd_c2s, &rfds);
        maxfd = std::max(maxfd, c.fd_c2s);
      }
    }

    int ready = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
    if (ready < 0)
    {
      if (errno == EINTR)
        continue;
      std::perror("select");
      break;
    }

    // JOINs
    if (FD_ISSET(fd_reg, &rfds))
    {
      std::string line;
      while (read_line(fd_reg, line))
      {
        auto toks = split_ws(line);
        if (toks.size() >= 4 && toks[0] == "JOIN")
        {
          pid_t pid = (pid_t)std::stol(toks[1]);
          std::string c2s = toks[2];
          std::string s2c = toks[3];

          // abrir ambos extremos en RDWR|NONBLOCK para evitar bloqueos
          int fd_c2s = open_rdwr_nonblock(c2s);
          int fd_s2c = open_rdwr_nonblock(s2c);
          if (fd_c2s != -1 && fd_s2c != -1)
          {
            clients[pid] = Client{pid, fd_c2s, fd_s2c};
            std::cout << "[SERVER] JOIN pid=" << pid << std::endl;
            write_line(fd_s2c, "OK bienvenido pid=" + std::to_string(pid));
            broadcast("MSG server pid=" + std::to_string(pid) + " se ha unido");
          }
          else
          {
            if (fd_c2s != -1)
              close(fd_c2s);
            if (fd_s2c != -1)
              close(fd_s2c);
          }
        }
        else
        {
          // línea parcial o basura; salir del while si no completa
          if (line.empty())
            break;
        }
      }
    }

    // Mensajes desde clientes
    std::vector<pid_t> to_remove;
    for (auto &[pid, c] : clients)
    {
      if (c.fd_c2s != -1 && FD_ISSET(c.fd_c2s, &rfds))
      {
        std::string line;
        if (!read_line(c.fd_c2s, line) || line.empty())
        {
          to_remove.push_back(pid);
        }
        else
        {
          auto toks = split_ws(line);
          if (toks.size() >= 2 && toks[0] == "LEAVE")
          {
            to_remove.push_back(pid);
            broadcast("MSG server pid=" + std::to_string(pid) + " salió");
          }
          else if (toks.size() >= 2 && toks[0] == "REPORT")
          {
            // REPORT <pid_reportado>
            if (toks.size() >= 2)
            {
              write_line(fd_reports, line); // reenvía al moderador
              write_line(c.fd_s2c, "OK reporte enviado");
            }
          }
          else if (toks.size() >= 2 && toks[0] == "MSG")
          {
            // MSG <pid> <texto...>
            std::string texto = line.substr(4); // salta "MSG "
            if (logf.is_open())
              logf << texto << "\n";
            broadcast("MSG " + texto);
          }
          else
          {
            write_line(c.fd_s2c, "ERR formato");
          }
        }
      }
    }
    for (pid_t p : to_remove)
      remove_client(p);
  }

  // limpieza
  for (auto &[pid, c] : clients)
  {
    if (c.fd_c2s != -1)
      close(c.fd_c2s);
    if (c.fd_s2c != -1)
      close(c.fd_s2c);
  }
  if (fd_reg != -1)
    close(fd_reg);
  if (fd_reports != -1)
    close(fd_reports);

  // opcional: no unlinkeamos los FIFOs globales para que moderador pueda seguir, pero puedes hacerlo
  // unlink(REG_FIFO.c_str());
  // unlink(REPORTS_FIFO.c_str());

  std::cout << "[SERVER] adiós.\n";
  return 0;
}
