#include "common.hpp"
#include <cstdlib>

using namespace chat;

static volatile sig_atomic_t stop_flag = 0;
static void on_sigint(int) { stop_flag = 1; }

int main()
{
  struct sigaction sa{};
  sa.sa_handler = on_sigint;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  pid_t mypid = getpid();
  std::string c2s_path = fifo_c2s(mypid);
  std::string s2c_path = fifo_s2c(mypid);

  mkfifo_if_needed(c2s_path);
  mkfifo_if_needed(s2c_path);

  // Para evitar bloqueos, abrimos ambos en RDWR|NONBLOCK
  int fd_c2s = open_rdwr_nonblock(c2s_path);
  int fd_s2c = open_rdwr_nonblock(s2c_path);
  if (fd_c2s == -1 || fd_s2c == -1)
  {
    std::cerr << "[CLIENT] error abriendo mis FIFOs\n";
    return 1;
  }

  // JOIN por FIFO de registro
  int fd_reg = open_write_nonblock(REG_FIFO);
  if (fd_reg == -1)
  {
    std::cerr << "[CLIENT] no puedo abrir " << REG_FIFO << "\n";
    return 1;
  }
  write_line(fd_reg, "JOIN " + std::to_string(mypid) + " " + c2s_path + " " + s2c_path);
  close(fd_reg);

  std::cout << "[CLIENT " << mypid << "] listo. Comandos:\n"
            << "  texto libre -> MSG\n"
            << "  reportar <pid> -> REPORT pid\n"
            << "  salir -> LEAVE y termina\n";

  while (!stop_flag)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(fd_s2c, &rfds);
    int maxfd = std::max(STDIN_FILENO, fd_s2c);

    int ready = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
    if (ready < 0)
    {
      if (errno == EINTR)
        continue;
      std::perror("select");
      break;
    }

    if (FD_ISSET(STDIN_FILENO, &rfds))
    {
      std::string input;
      std::getline(std::cin, input);
      if (!std::cin)
      {
        break;
      }

      auto toks = split_ws(input);
      if (!toks.empty() && toks[0] == "salir")
      {
        write_line(fd_c2s, "LEAVE " + std::to_string(mypid));
        break;
      }
      else if (toks.size() == 2 && toks[0] == "reportar")
      {
        write_line(fd_c2s, "REPORT " + toks[1]);
      }
      else
      {
        // envia como MSG "<pid> <texto>"
        write_line(fd_c2s, "MSG " + std::to_string(mypid) + " " + input);
      }
    }

    if (FD_ISSET(fd_s2c, &rfds))
    {
      std::string line;
      if (!read_line(fd_s2c, line) || line.empty())
      {
        std::cout << "[CLIENT] servidor cerró\n";
        break;
      }
      else
      {
        std::cout << line << std::endl;
      }
    }
  }

  close(fd_c2s);
  close(fd_s2c);
  // Limpieza opcional de mis FIFOs (si nadie más las usa)
  unlink(c2s_path.c_str());
  unlink(s2c_path.c_str());

  std::cout << "[CLIENT] bye.\n";
  return 0;
}
