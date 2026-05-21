#include "ui/GridDisplay.hpp"
#include <iostream>

static constexpr const char* GREEN  = "\033[1;32m";
static constexpr const char* RED    = "\033[1;31m";
static constexpr const char* YELLOW = "\033[1;33m";
static constexpr const char* CYAN   = "\033[1;36m";
static constexpr const char* RESET  = "\033[0m";

GridDisplay::GridDisplay(int width, int height)
    : _width(width), _height(height) {}

void GridDisplay::render(Position rvc, Heading heading,
                          const std::set<std::pair<int,int>>& obstacles,
                          const std::set<std::pair<int,int>>& dust_cells,
                          Direction dir, CleanPower power, int tick) const {
    // Move cursor to top-left without clearing (avoids flicker)
    std::cout << "\033[H";

    // Top border
    std::cout << '+';
    for (int x = 0; x < _width; ++x) { std::cout << "--"; }
    std::cout << "+\n";

    // Rows
    for (int y = 0; y < _height; ++y) {
        std::cout << '|';
        for (int x = 0; x < _width; ++x) {
            if (x == rvc.x && y == rvc.y) {
                std::cout << GREEN << toArrow(heading) << ' ' << RESET;
            } else if (obstacles.count({x, y}) != 0U) {
                std::cout << RED << "X " << RESET;
            } else if (dust_cells.count({x, y}) != 0U) {
                std::cout << CYAN << "* " << RESET;
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "|\n";
    }

    // Bottom border
    std::cout << '+';
    for (int x = 0; x < _width; ++x) { std::cout << "--"; }
    std::cout << "+\n";

    // Status line
    const char* const power_color = (power == CleanPower::POWER_UP) ? YELLOW : RESET;
    std::cout << " Tick:" << tick
              << " | Dir:" << toString(dir)
              << " | Clean:" << power_color << toString(power) << RESET
              << " | Pos:(" << rvc.x << "," << rvc.y << ")"
              << " | Heading:" << toString(heading)
              << "    \n";

    std::cout.flush();
}
