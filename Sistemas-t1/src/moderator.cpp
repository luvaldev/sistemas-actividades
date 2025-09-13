#include "common.hpp"
#include <unordered_map>
#include <cstdlib>

using namespace chat;

int main()
{
  // crea fifo global de reportes si no existe
  mkfifo_if_needed(REPORTS_FIFO);

  // abre fifo de reportes en lectura no bloqueante
  int fd = open_read_nonblock(REPORTS_FIFO);
  if (fd == -1)
    return 1;

  // tabla para contar reportes por pid
  std::unordered_map<pid_t, int> cnt;

  std::cout << "[mod] escuchando en " << REPORTS_FIFO << std::endl;

  // bucle infinito: espera lineas de reportes y actua
  while (true)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    // espera hasta que haya datos en la fifo
    int ready = select(fd + 1, &rfds, nullptr, nullptr, nullptr);
    if (ready < 0)
    {
      if (errno == EINTR)
        continue; // señal interrumpio select, reintenta
      std::perror("select");
      break;
    }

    if (FD_ISSET(fd, &rfds))
    {
      std::string line;
      // intenta leer una linea completa
      if (!chat::read_line(fd, line) || line.empty())
      {
        // si fifo se cerro, reabre para seguir escuchando
        close(fd);
        fd = open_read_nonblock(REPORTS_FIFO);
        continue;
      }

      // divide en tokens
      auto t = split_ws(line);
      // formato esperado: "report <pid>"
      if (t.size() == 2 && t[0] == "REPORT")
      {
        pid_t p = (pid_t)std::stol(t[1]);
        int c = ++cnt[p]; // aumenta contador de este pid

        std::cout << "[mod] report pid=" << p << " total=" << c << std::endl;

        // si supera 10 reportes, intenta matar proceso
        if (c > 10)
        {
          std::cout << "[mod] baneando pid=" << p << std::endl;
          if (kill(p, SIGTERM) == -1)
            std::perror("kill sigterm");
          // opcional: esperar y mandar sigkill si sigue vivo
          // usleep(200*1000);
          // if (kill(p, 0) == 0) kill(p, SIGKILL);
        }
      }
    }
  }

  // cierre ordenado de fd
  close(fd);
  return 0;
}
