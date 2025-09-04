#include "common.hpp"
#include <unordered_map>
#include <cstdlib>

using namespace chat;

int main()
{
  mkfifo_if_needed(REPORTS_FIFO);
  int fd = open_read_nonblock(REPORTS_FIFO);
  if (fd == -1)
    return 1;

  std::unordered_map<pid_t, int> cnt;
  std::cout << "[MOD] escuchando en " << REPORTS_FIFO << std::endl;

  while (true)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    int ready = select(fd + 1, &rfds, nullptr, nullptr, nullptr);
    if (ready < 0)
    {
      if (errno == EINTR)
        continue;
      std::perror("select");
      break;
    }
    if (FD_ISSET(fd, &rfds))
    {
      std::string line;
      if (!chat::read_line(fd, line) || line.empty())
      {
        // fifo cerrada; reabrir
        close(fd);
        fd = open_read_nonblock(REPORTS_FIFO);
        continue;
      }
      auto t = split_ws(line);
      if (t.size() == 2 && t[0] == "REPORT")
      {
        pid_t p = (pid_t)std::stol(t[1]);
        int c = ++cnt[p];
        std::cout << "[MOD] REPORT pid=" << p << " total=" << c << std::endl;
        if (c > 10)
        {
          std::cout << "[MOD] baneando pid=" << p << std::endl;
          if (kill(p, SIGTERM) == -1)
            std::perror("kill SIGTERM");
          // opcional: esperar y SIGKILL si sigue vivo
          // usleep(200*1000);
          // if (kill(p, 0) == 0) kill(p, SIGKILL);
        }
      }
    }
  }
  close(fd);
  return 0;
}
