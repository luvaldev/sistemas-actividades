#include "common.hpp"
#include <unordered_map>
#include <algorithm>
#include <fstream>

using namespace chat;

// cliente: guarda pid y los fds para hablar con el server
struct Client
{
  pid_t pid{};
  int fd_c2s{-1}; // lectura desde el cliente (cliente -> server)
  int fd_s2c{-1}; // escritura hacia el cliente (server -> cliente)
};

// tabla de clientes conectados
static std::unordered_map<pid_t, Client> clients;

// fds globales: registro de joins y canal de reportes hacia el moderador
static int fd_reg = -1;     // fifo donde llegan los join
static int fd_reports = -1; // fifo donde el server manda "report <pid>" al moderador

// log en archivo
static std::ofstream logf;

// bandera para terminar ordenado con ctrl+c
static volatile sig_atomic_t stop_flag = 0;
static void on_sigint(int) { stop_flag = 1; }

// envia una linea a todos los clientes conectados (difusion)
static void broadcast(const std::string &line)
{
  for (auto &[pid, c] : clients)
  {
    if (c.fd_s2c != -1)
    {
      write_line(c.fd_s2c, line);
    }
  }
}

// saca un cliente: cierra fds y lo borra del mapa
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
  // instala handlers para poder cerrar ordenado con señales
  struct sigaction sa{};
  sa.sa_handler = on_sigint;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  // crea fifos globales (si no existen) para registro y reportes
  mkfifo_if_needed(REG_FIFO);
  mkfifo_if_needed(REPORTS_FIFO);

  // abre fifo de registro en solo lectura no bloqueante
  fd_reg = open_read_nonblock(REG_FIFO);
  if (fd_reg == -1)
    return 1;

  // abre fifo de reportes en rdwr no bloqueante para no quedar colgado si moderador no esta
  fd_reports = open_rdwr_nonblock(REPORTS_FIFO);
  if (fd_reports == -1)
    return 1;

  // abre log para guardar el historial simple
  logf.open("chat.log", std::ios::app);

  std::cout << "[server] listo. leyendo join en " << REG_FIFO << std::endl;

  // bucle principal: multiplexa con select sobre fifo de registro + c2s de cada cliente
  while (!stop_flag)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd_reg, &rfds);
    int maxfd = fd_reg;

    // agrega todos los c2s de clientes al set
    for (auto &[pid, c] : clients)
    {
      if (c.fd_c2s != -1)
      {
        FD_SET(c.fd_c2s, &rfds);
        maxfd = std::max(maxfd, c.fd_c2s);
      }
    }

    // espera eventos de lectura
    int ready = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
    if (ready < 0)
    {
      if (errno == EINTR)
        continue; // señal: reintenta
      std::perror("select");
      break;
    }

    // procesa joins nuevos que llegaron al fifo de registro
    if (FD_ISSET(fd_reg, &rfds))
    {
      std::string line;
      // lee varias lineas si hay
      while (read_line(fd_reg, line))
      {
        auto toks = split_ws(line);
        // formato esperado: "join <pid> <fifo_c2s> <fifo_s2c>"
        if (toks.size() >= 4 && toks[0] == "JOIN")
        {
          pid_t pid = (pid_t)std::stol(toks[1]);
          std::string c2s = toks[2];
          std::string s2c = toks[3];

          // abre los extremos del cliente en rdwr|nonblock para evitar bloqueos
          int fd_c2s = open_rdwr_nonblock(c2s);
          int fd_s2c = open_rdwr_nonblock(s2c);
          if (fd_c2s != -1 && fd_s2c != -1)
          {
            clients[pid] = Client{pid, fd_c2s, fd_s2c};
            std::cout << "[server] join pid=" << pid << std::endl;
            write_line(fd_s2c, "ok bienvenido pid=" + std::to_string(pid));
            // avisa a todos que entro este pid
            broadcast("MSG server " + std::to_string(pid) + " se ha unido");
          }
          else
          {
            // si fallo abrir alguno, cierra lo que abriste
            if (fd_c2s != -1)
              close(fd_c2s);
            if (fd_s2c != -1)
              close(fd_s2c);
          }
        }
        else
        {
          // si la linea no calza o es parcial, corta el while para no ciclar
          if (line.empty())
            break;
        }
      }
    }

    // procesa trafico que llega desde cada cliente (por su c2s)
    std::vector<pid_t> to_remove;
    for (auto &[pid, c] : clients)
    {
      if (c.fd_c2s != -1 && FD_ISSET(c.fd_c2s, &rfds))
      {
        std::string line;
        // si read_line da vacio o error: el cliente se fue
        if (!read_line(c.fd_c2s, line) || line.empty())
        {
          to_remove.push_back(pid);
        }
        else
        {
          auto toks = split_ws(line);

          // leave: cliente pide salir
          if (!toks.empty() && toks[0] == "LEAVE")
          {
            to_remove.push_back(pid);
            std::cout << "[server] leave pid=" << pid << std::endl;
            broadcast("MSG server " + std::to_string(pid) + " salio");
          }
          // report: cliente reporta a otro pid; reenviamos al moderador
          else if (!toks.empty() && toks[0] == "REPORT")
          {
            if (toks.size() >= 2)
            {
              std::cout << "[server] report de " << pid << " -> " << toks[1] << std::endl;
              // mandamos la linea tal cual al moderador por la fifo de reportes
              write_line(fd_reports, line);
              // respondemos ack al emisor
              write_line(c.fd_s2c, "ok reporte enviado");
            }
            else
            {
              write_line(c.fd_s2c, "err uso: report <pid>");
            }
          }
          // msg: mensaje normal; mostramos pid real, logueamos y difundimos
          else if (!toks.empty() && toks[0] == "MSG")
          {
            // tomamos solo el texto real y descartamos cualquier pid que haya escrito el cliente
            std::string msgText;
            if (line.size() > 4)
            {
              msgText = line.substr(4); // quita "msg "
              auto firstSpace = msgText.find(' ');
              if (firstSpace != std::string::npos)
              {
                // descarta primer token (suele ser el pid autodeclarado del cliente)
                msgText = msgText.substr(firstSpace + 1);
              }
            }

            // imprime en la consola del server con el pid real
            std::cout << "[server][" << pid << "] " << msgText << std::endl;

            // guarda en el log
            if (logf.is_open())
            {
              logf << "[pid " << pid << "] " << msgText << "\n";
              logf.flush();
            }

            // difunde a todos con el pid real (anti suplantacion)
            broadcast("MSG " + std::to_string(pid) + " " + msgText);
          }
          // formato desconocido: responde error al cliente
          else
          {
            write_line(c.fd_s2c, "err formato");
          }
        }
      }
    }

    // limpia clientes que se fueron
    for (pid_t p : to_remove)
      remove_client(p);
  }

  // salida ordenada: cierra todos los fds y deja constancia
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

  // si quieres borrar las fifos globales, descomenta (a veces conviene dejarlas)
  // unlink(REG_FIFO.c_str());
  // unlink(REPORTS_FIFO.c_str());

  std::cout << "[server] adios.\n";
  return 0;
}
