#include "common.hpp"
#include <cstdlib>
#include <sys/wait.h>

using namespace chat;

// bandera global para terminar limpio cuando llegue una señal
static volatile sig_atomic_t stop_flag = 0;
static void on_sigint(int) { stop_flag = 1; }

int main(int argc, char **argv)
{
    // instala handlers de señal para poder cortar sin dejar fifos colgando
    struct sigaction sa{};
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // identificacion del proceso y rutas de sus fifos privadas
    pid_t mypid = getpid();
    std::string c2s_path = fifo_c2s(mypid); // cliente -> server
    std::string s2c_path = fifo_s2c(mypid); // server  -> cliente

    // crea las fifos si no existen
    mkfifo_if_needed(c2s_path);
    mkfifo_if_needed(s2c_path);

    // abre ambas en rdwr|nonblock para evitar bloqueos tipicos de fifo
    int fd_c2s = open_rdwr_nonblock(c2s_path);
    int fd_s2c = open_rdwr_nonblock(s2c_path);
    if (fd_c2s == -1 || fd_s2c == -1)
    {
        std::cerr << "[client] error abriendo mis fifos\n";
        return 1;
    }

    // hace join con el server a traves de la fifo de registro global
    int fd_reg = open_write_nonblock(REG_FIFO);
    if (fd_reg == -1)
    {
        std::cerr << "[client] no puedo abrir " << REG_FIFO << "\n";
        return 1;
    }
    write_line(fd_reg, "JOIN " + std::to_string(mypid) + " " + c2s_path + " " + s2c_path);
    close(fd_reg);

    // muestra comandos basicos para el usuario
    std::cout << "[client " << mypid << "] listo. comandos:\n"
              << "  <texto>        -> envia msg\n"
              << "  reportar <pid> -> envia report <pid>\n"
              << "  clone          -> crea un nuevo proceso cliente\n"
              << "  salir          -> leave y termina\n";

    // bucle principal: multiplexa stdin y la fifo s2c usando select
    while (!stop_flag)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds); // entrada del usuario
        FD_SET(fd_s2c, &rfds);       // mensajes que vienen del server
        int maxfd = std::max(STDIN_FILENO, fd_s2c);

        // espera eventos de lectura
        int ready = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue; // señal interrumpio, reintenta
            std::perror("select");
            break;
        }

        // procesa lo que escribio el usuario por teclado
        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            std::string input;
            std::getline(std::cin, input);
            if (!std::cin)
            {
                break;
            } // eof o error en stdin

            auto toks = split_ws(input);

            // comando salir: avisa al server y termina
            if (!toks.empty() && toks[0] == "salir")
            {
                write_line(fd_c2s, "LEAVE " + std::to_string(mypid));
                break;

                // comando reportar: envia "report <pid>" al server
            }
            else if (toks.size() == 2 && toks[0] == "reportar")
            {
                write_line(fd_c2s, "REPORT " + toks[1]);

                // comando clone: crea un nuevo proceso cliente completamente independiente
            }
            else if (toks.size() == 1 && toks[0] == "clone")
            {
                pid_t child = fork();
                if (child == -1)
                {
                    std::perror("fork");
                }
                else if (child == 0)
                {
                    // hijo: ejecuta el binario del cliente
                    // asume que el binario esta en ./build/client relativo al cwd actual
                    execl("./build/client", "./build/client", (char *)nullptr);
                    std::perror("execl");
                    _exit(1);
                }
                else
                {
                    std::cout << "[client " << mypid << "] clon lanzado pid=" << child << "\n";
                }

                // caso general: todo lo demas se envia como mensaje normal
            }
            else
            {
                // formato simple: "msg <pid> <texto libre>"
                write_line(fd_c2s, "MSG " + std::to_string(mypid) + " " + input);
            }
        }

        // procesa mensajes que llegaron desde el server por la fifo s2c
        if (FD_ISSET(fd_s2c, &rfds))
        {
            std::string line;
            if (!read_line(fd_s2c, line) || line.empty())
            {
                // si el server cerro, salimos
                std::cout << "[client] server cerro\n";
                break;
            }
            else
            {
                // imprime lo que dijo el server (difusiones, acks, errores)
                std::cout << line << std::endl;
            }
        }
    }

    // cierra descriptores antes de salir
    close(fd_c2s);
    close(fd_s2c);

    // limpia las fifos propias del cliente
    unlink(c2s_path.c_str());
    unlink(s2c_path.c_str());

    std::cout << "[client " << mypid << "] bye.\n";
    return 0;
}
