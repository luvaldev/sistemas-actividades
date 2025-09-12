#include "common.hpp"
#include <cstdlib>
#include <sys/wait.h>

using namespace chat;

static volatile sig_atomic_t stop_flag = 0;
static void on_sigint(int){ stop_flag = 1; }

int main(int argc, char** argv) {
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

    // Abrimos en RDWR|NONBLOCK para evitar bloqueos típicos de FIFO
    int fd_c2s = open_rdwr_nonblock(c2s_path);
    int fd_s2c = open_rdwr_nonblock(s2c_path);
    if (fd_c2s == -1 || fd_s2c == -1) {
        std::cerr << "[CLIENT] error abriendo mis FIFOs\n";
        return 1;
    }

    // JOIN por FIFO de registro
    int fd_reg = open_write_nonblock(REG_FIFO);
    if (fd_reg == -1) {
        std::cerr << "[CLIENT] no puedo abrir " << REG_FIFO << "\n";
        return 1;
    }
    write_line(fd_reg, "JOIN " + std::to_string(mypid) + " " + c2s_path + " " + s2c_path);
    close(fd_reg);

    std::cout << "[CLIENT " << mypid << "] listo. Comandos:\n"
              << "  <texto>              -> envía MSG\n"
              << "  reportar <pid>       -> envía REPORT <pid>\n"
              << "  clone                -> crea un nuevo proceso cliente\n"
              << "  salir                -> LEAVE y termina\n";

    while (!stop_flag) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(fd_s2c, &rfds);
        int maxfd = std::max(STDIN_FILENO, fd_s2c);

        int ready = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) continue;
            std::perror("select");
            break;
        }

        // Entrada del usuario
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            std::string input;
            std::getline(std::cin, input);
            if (!std::cin) { break; }

            auto toks = split_ws(input);

            if (!toks.empty() && toks[0] == "salir") {
                write_line(fd_c2s, "LEAVE " + std::to_string(mypid));
                break;

            } else if (toks.size() == 2 && toks[0] == "reportar") {
                // REPORT pid
                write_line(fd_c2s, "REPORT " + toks[1]);

            } else if (toks.size() == 1 && toks[0] == "clone") {
                // Crea un nuevo proceso cliente completamente independiente
                pid_t child = fork();
                if (child == -1) {
                    std::perror("fork");
                } else if (child == 0) {
                    // Hijo: ejecuta el binario del cliente. Supone que estás en la carpeta del proyecto.
                    // Si prefieres robustez extra, usa /proc/self/exe, pero esto basta para el flujo del curso.
                    execl("./build/client", "./build/client", (char*)nullptr);
                    std::perror("execl");
                    _exit(1);
                } else {
                    std::cout << "[CLIENT " << mypid << "] clon lanzado pid=" << child << "\n";
                }

            } else {
                // Enviar como MSG "<pid> <texto libre...>"
                write_line(fd_c2s, "MSG " + std::to_string(mypid) + " " + input);
            }
        }

        // Mensajes del servidor
        if (FD_ISSET(fd_s2c, &rfds)) {
            std::string line;
            if (!read_line(fd_s2c, line) || line.empty()) {
                std::cout << "[CLIENT] servidor cerró\n";
                break;
            } else {
                std::cout << line << std::endl;
            }
        }
    }

    close(fd_c2s);
    close(fd_s2c);

    // Limpia tus FIFOs (cada cliente gestiona las suyas)
    unlink(c2s_path.c_str());
    unlink(s2c_path.c_str());

    std::cout << "[CLIENT " << mypid << "] bye.\n";
    return 0;
}
