#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include "simulator/Simulator.hpp"
#include "ui/GridDisplay.hpp"

using namespace std::chrono_literals;

// Terminal raw mode

#ifdef _WIN32
static DWORD g_orig_console_mode = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
#else
static struct termios g_orig_termios; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
#endif

static void restoreTerminal() {
#ifdef _WIN32
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), g_orig_console_mode);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
#endif
    std::cout << "\033[?25h\033[999;1H\n" << std::flush;
}

static void enableRawMode() {
#ifdef _WIN32
    // [변경] Windows builds use Console API instead of POSIX termios.
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(input, &g_orig_console_mode);
    std::atexit(restoreTerminal);
    DWORD raw = g_orig_console_mode;
    raw &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(input, raw);

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD output_mode = 0;
    if (GetConsoleMode(output, &output_mode) != 0) {
        SetConsoleMode(output, output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    std::atexit(restoreTerminal);

    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON);  // no echo, char-by-char
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
}

static bool readInputChar(char& c) {
#ifdef _WIN32
    c = static_cast<char>(_getch());
    return true;
#else
    return read(STDIN_FILENO, &c, 1) > 0;
#endif
}

// Helpers

static int inputRow(int grid_h) { return grid_h + 4; }

static void printHelp(int grid_h) {
    const int row = inputRow(grid_h) + 1;
    std::cout << "\033[" << row << ";1H\033[J"
              << "=== RVC System Operations ===\n"
              << "  start          startCleaning()\n"
              << "  stop           stopCleaning()\n"
              << "  speed <ms>     set tick interval (default 300)\n"
              << "  dust x y       placeDust(x, y) - shown as cyan *\n"
              << "  obstacle x y   placeObstacle(x, y)\n"
              << "  help           show this menu\n"
              << "  quit           exit\n"
              << std::flush;
}

static void renderInputLine(int grid_h, const std::string& buf) {
    int row = inputRow(grid_h);
    std::cout << "\033[" << row << ";1H\033[2K"
              << "> " << buf << std::flush;
}

static void processCommand(const std::string& line, Simulator& sim,
                           std::atomic<int>& tick_ms,
                           int grid_h, bool& quit) {
    std::istringstream ss(line);
    std::string cmd;
    ss >> cmd;

    if (cmd == "start") {
        sim.start();
    } else if (cmd == "stop") {
        sim.stop();
    } else if (cmd == "speed") {
        int ms = 300;
        ss >> ms;
        if (ms < 50) { ms = 50; }
        tick_ms = ms;
    } else if (cmd == "dust") {
        int x = 0;
        int y = 0;
        if (ss >> x >> y) { sim.placeDust(x, y); }
    } else if (cmd == "obstacle") {
        int x = 0;
        int y = 0;
        if (ss >> x >> y) { sim.placeObstacle(x, y); }
    } else if (cmd == "help") {
        printHelp(grid_h);
    } else if (cmd == "quit" || cmd == "exit") {
        quit = true;
    }
}

// Main

static void setupObstacles(Simulator& sim) {
    for (int x = 5; x <= 9;  ++x) { sim.placeObstacle(x, 2); }
    for (int y = 3; y <= 4;  ++y) { sim.placeObstacle(9, y); }
    for (int y = 7; y <= 8;  ++y) { sim.placeObstacle(9, y); }
    for (int x = 5; x <= 14; ++x) { sim.placeObstacle(x, 8); }
    for (int y = 3; y <= 7;  ++y) { sim.placeObstacle(16, y); }
}

int main() {
    const int grid_w = 22;
    const int grid_h = 12;

    Simulator   sim(grid_w, grid_h, {1, 5}, Heading::EAST);
    GridDisplay display(grid_w, grid_h);

    setupObstacles(sim);

    std::mutex       sim_mutex;
    std::atomic<int>  tick_ms{300};
    std::atomic<bool> running{true};
    int tick = 0;

    enableRawMode();
    std::cout << "\033[2J\033[?25l";

    std::thread tick_thread([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(tick_ms.load()));
            std::lock_guard<std::mutex> lock(sim_mutex);
            // [변경] The display clock advances only while cleaning is active.
            if (sim.isRunning()) {
                ++tick;
                sim.tick();
            }

            std::cout << "\033[H";
            display.render(sim.pos(), sim.heading(), sim.obstacles(), sim.dustCells(),
                           sim.lastDirection(), sim.lastPower(), tick);
        }
    });

    {
        std::lock_guard<std::mutex> lock(sim_mutex);
        std::cout << "\033[H";
        display.render(sim.pos(), sim.heading(), sim.obstacles(), sim.dustCells(),
                       sim.lastDirection(), sim.lastPower(), tick);
    }
    printHelp(grid_h);
    renderInputLine(grid_h, "");

    std::string buf;
    bool quit = false;

    while (!quit) {
        char c = '\0';
        if (!readInputChar(c)) { break; }

        std::lock_guard<std::mutex> lock(sim_mutex);

        if (c == '\n' || c == '\r') {
            if (!buf.empty()) {
                processCommand(buf, sim, tick_ms, grid_h, quit);
                buf.clear();
            }
        } else if (c == 127 || c == '\b') {
            if (!buf.empty()) { buf.pop_back(); }
        } else if (c == 3) {
            quit = true;
        } else if (c >= 32 && c < 127) {
            buf += c;
        }

        if (!quit) {
            renderInputLine(grid_h, buf);
        }
    }

    running = false;
    tick_thread.join();
    return 0;
}
