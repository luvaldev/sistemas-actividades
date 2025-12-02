#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>

// --- definición de colores para consola ---
#define COLOR_RESET "\x1b[0m"
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"

using namespace std;

// --- estructuras de datos ---

// estructura para representar un marco de pagina
struct MemoryFrame
{
    int pid = -1;      // id del proceso dueno
    int page_num = -1; // numero de pagina logica
    bool occupied = false;
};

// estructura para controlar procesos
struct Process
{
    int pid;
    int total_pages;
    bool active;
};

// --- variables globales ---

vector<MemoryFrame> ram;
vector<MemoryFrame> swap_memory; 
vector<Process> processes;

int ram_size_mb, page_size_kb, swap_size_mb;
int num_frames_ram, num_frames_swap;
int min_proc_size, max_proc_size;
int fifo_pointer = 0;

// --- funciones auxiliares ---

// inicializa los vectores de memoria
void init_memory()
{
    ram.resize(num_frames_ram);
    swap_memory.resize(num_frames_swap);
}

// busca un frame libre en ram
int find_free_ram()
{
    for (int i = 0; i < num_frames_ram; i++)
    {
        if (!ram[i].occupied)
            return i;
    }
    return -1;
}

// busca un frame libre en swap
int find_free_swap()
{
    for (int i = 0; i < num_frames_swap; i++)
    {
        if (!swap_memory[i].occupied)
            return i;
    }
    return -1;
}

// crea un proceso nuevo y asigna memoria
void create_process(int id)
{
    int size_kb = min_proc_size + rand() % (max_proc_size - min_proc_size + 1);

    // calculo de paginas necesarias
    int pages_needed = (size_kb + page_size_kb - 1) / page_size_kb;

    cout << COLOR_GREEN "[crear] proceso " << id << " (tamano: " << size_kb
         << " kb, paginas: " << pages_needed << ")" << COLOR_RESET << endl;

    processes.push_back({id, pages_needed, true});

    // asignar paginas
    for (int i = 0; i < pages_needed; i++)
    {
        int ram_idx = find_free_ram();

        if (ram_idx != -1)
        {
            // asignar a ram
            ram[ram_idx].occupied = true;
            ram[ram_idx].pid = id;
            ram[ram_idx].page_num = i;
        }
        else
        {
            // ram llena, intentar en swap
            int swap_idx = find_free_swap();
            if (swap_idx != -1)
            {
                swap_memory[swap_idx].occupied = true;
                swap_memory[swap_idx].pid = id;
                swap_memory[swap_idx].page_num = i;
                cout << COLOR_YELLOW "  -> pagina " << i << " (proc " << id
                     << ") asignada a swap (ram llena)" << COLOR_RESET << endl;
            }
            else
            {
                // memoria agotada
                cout << COLOR_RED "[fatal] memoria agotada (ram y swap llenas). fin de simulacion."
                     << COLOR_RESET << endl;
                exit(0);
            }
        }
    }
}

// finaliza un proceso aleatorio y libera memoria
void kill_process_random()
{
    // filtrar procesos activos
    vector<int> active_indices;
    for (size_t i = 0; i < processes.size(); i++)
    {
        if (processes[i].active)
            active_indices.push_back(i);
    }

    if (active_indices.empty())
        return;

    // elegir victima
    int victim_idx = active_indices[rand() % active_indices.size()];
    int pid = processes[victim_idx].pid;
    processes[victim_idx].active = false;

    cout << COLOR_MAGENTA "[kill] finalizando proceso " << pid << COLOR_RESET << endl;

    // liberar frames en ram
    for (auto &frame : ram)
    {
        if (frame.occupied && frame.pid == pid)
        {
            frame.occupied = false;
            frame.pid = -1;
        }
    }
    // liberar frames en swap
    for (auto &frame : swap_memory)
    {
        if (frame.occupied && frame.pid == pid)
        {
            frame.occupied = false;
            frame.pid = -1;
        }
    }
}

// simula acceso a memoria y gestiona page faults
void access_random_address()
{
    // buscar proceso activo
    vector<int> active_indices;
    for (size_t i = 0; i < processes.size(); i++)
    {
        if (processes[i].active)
            active_indices.push_back(i);
    }

    if (active_indices.empty())
        return;

    int proc_idx = active_indices[rand() % active_indices.size()];
    int pid = processes[proc_idx].pid;
    // pagina aleatoria dentro del proceso
    int target_page = rand() % processes[proc_idx].total_pages;

    cout << COLOR_CYAN "[acceso] proc " << pid << " solicita pagina virtual "
         << target_page << COLOR_RESET << endl;

    // verificar si esta en ram
    bool in_ram = false;
    for (int i = 0; i < num_frames_ram; i++)
    {
        if (ram[i].occupied && ram[i].pid == pid && ram[i].page_num == target_page)
        {
            in_ram = true;
            cout << "  -> pagina encontrada en ram (frame " << i << "). exito." << endl;
            break;
        }
    }

    if (!in_ram)
    {
        // page fault
        cout << COLOR_RED "  -> page fault! pagina no esta en ram. buscando en swap..."
             << COLOR_RESET << endl;

        int swap_loc = -1;
        // buscar en swap
        for (int i = 0; i < num_frames_swap; i++)
        {
            if (swap_memory[i].occupied && swap_memory[i].pid == pid && swap_memory[i].page_num == target_page)
            {
                swap_loc = i;
                break;
            }
        }

        if (swap_loc != -1)
        {
            // politica fifo para reemplazo
            int victim_frame = fifo_pointer;
            fifo_pointer = (fifo_pointer + 1) % num_frames_ram;

            cout << COLOR_YELLOW "  -> swap in/out: moviendo frame " << victim_frame
                 << " (pid " << ram[victim_frame].pid << ") a swap slot " << swap_loc
                 << COLOR_RESET << endl;

            // intercambio directo
            MemoryFrame temp = ram[victim_frame];

            ram[victim_frame] = swap_memory[swap_loc];
            ram[victim_frame].occupied = true;

            swap_memory[swap_loc] = temp;
        }
        else
        {
            cout << "  -> error: pagina no encontrada en swap (o proceso finalizado)." << endl;
        }
    }
}

// visualizacion del estado de la memoria
void print_status()
{
    int ram_used = 0;
    for (const auto &f : ram)
        if (f.occupied)
            ram_used++;

    int swap_used = 0;
    for (const auto &f : swap_memory)
        if (f.occupied)
            swap_used++;

    cout << "--- estado: ram " << ram_used << "/" << num_frames_ram
         << " | swap " << swap_used << "/" << num_frames_swap << " ---" << endl;
}

// --- funcion principal ---

int main()
{
    srand(time(NULL));

    cout << "ingrese memoria fisica (mb): ";
    if (!(cin >> ram_size_mb))
        return 1;

    cout << "ingrese tamano pagina (kb): ";
    if (!(cin >> page_size_kb))
        return 1;

    cout << "ingrese rango procesos (min_kb max_kb): ";
    if (!(cin >> min_proc_size >> max_proc_size))
        return 1;

    // calculo aleatorio de swap (1.5x a 4.5x ram)
    float multiplier = 1.5 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 3.0));
    swap_size_mb = static_cast<int>(ram_size_mb * multiplier);

    cout << "memoria virtual (swap) asignada: " << swap_size_mb
         << " mb (factor " << multiplier << "x)" << endl;

    // conversion a cantidad de frames
    num_frames_ram = (ram_size_mb * 1024) / page_size_kb;
    num_frames_swap = (swap_size_mb * 1024) / page_size_kb;

    cout << "frames totales -> ram: " << num_frames_ram
         << " | swap: " << num_frames_swap << endl;

    init_memory();

    int timer = 0;
    int pid_counter = 1;

    // ciclo principal de la simulacion
    while (true)
    {
        cout << endl
             << "[t=" << timer << " s]" << endl;

        // crear proceso cada 2 segundos
        if (timer % 2 == 0)
        {
            create_process(pid_counter++);
        }

        // eventos despues de 30 segundos
        if (timer >= 30)
        {
            // matar proceso cada 5 segundos
            if (timer % 5 == 0)
            {
                kill_process_random();
            }
            // acceder memoria cada 5 segundos
            if (timer % 5 == 0)
            {
                access_random_address();
            }
        }

        print_status();

        // esperar 1 segundo
        this_thread::sleep_for(chrono::seconds(1));
        timer++;
    }

    return 0;
}