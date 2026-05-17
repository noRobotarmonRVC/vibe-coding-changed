#include <iostream>
#include <thread>
#include <chrono>
#include "simulator/Simulator.hpp"
#include "ui/GridDisplay.hpp"

using namespace std::chrono_literals;

static void step(Simulator& sim, GridDisplay& display, int& tick,
                 std::chrono::milliseconds delay = 220ms) {
    ++tick;
    sim.tick();
    display.render(sim.pos(), sim.heading(), sim.obstacles(),
                   sim.lastDirection(), sim.lastPower(), tick);
    std::this_thread::sleep_for(delay);
}

int main() {
    const int W = 22, H = 12;

    Simulator   sim(W, H, {1, 5}, Heading::EAST);
    GridDisplay display(W, H);
    int tick = 0;

    // ── map layout ────────────────────────────────────────────────────────────
    //
    //  +--------------------------------------------+
    //  |. . . . . . . . . . . . . . . . . . . . . .|
    //  |. . . . . . . . . . . . . . . . . . . . . .|
    //  |. . . . . X X X X X . . . . . . . . . . . .|
    //  |. . . . . . . . . X . . . . . . X . . . . .|
    //  |. . . . . . . . . X . . . . . . X . . . . .|
    //  |> . . . . . . . . . . . . . . . X . . . . .|  <- start (1,5) EAST
    //  |. . . . . . . . . . . . . . . . X . . . . .|
    //  |. . . . . . . . . X . . . . . . X . . . . .|
    //  |. . . . . . . . . X X X X X X . . . . . . .|
    //  |. . . . . . . . . . . . . . . . . . . . . .|
    //  |. . . . . . . . . . . . . . . . . . . . . .|
    //  |. . . . . . . . . . . . . . . . . . . . . .|
    //  +--------------------------------------------+

    // Top-right corridor wall (C-shape facing left, open at y=5)
    for (int x = 5; x <= 9;  ++x) sim.placeObstacle(x, 2);   // top bar
    for (int y = 3; y <= 4;  ++y) sim.placeObstacle(9, y);   // right side top
    for (int y = 7; y <= 8;  ++y) sim.placeObstacle(9, y);   // right side bottom
    for (int x = 5; x <= 14; ++x) sim.placeObstacle(x, 8);   // bottom bar

    // Vertical wall at x=16 (open bottom)
    for (int y = 3; y <= 7; ++y) sim.placeObstacle(16, y);

    // ── demo ─────────────────────────────────────────────────────────────────
    // Clear screen once before animation starts
    std::cout << "\033[2J";

    sim.start();
    display.render(sim.pos(), sim.heading(), sim.obstacles(),
                   sim.lastDirection(), sim.lastPower(), tick);
    std::this_thread::sleep_for(600ms);

    // Phase 1: Forward navigation toward the C-wall
    for (int i = 0; i < 12; ++i)
        step(sim, display, tick);

    // Phase 2: Inject dust — intensify for INTENSIFY_DURATION ticks
    sim.injectDust(true);
    step(sim, display, tick);
    sim.injectDust(false);
    for (int i = 0; i < RvcController::INTENSIFY_DURATION; ++i)
        step(sim, display, tick);

    // Phase 3: Continue navigating — RVC will encounter obstacles and walls
    for (int i = 0; i < 55; ++i)
        step(sim, display, tick);

    // Phase 4: Stop
    sim.stop();
    display.render(sim.pos(), sim.heading(), sim.obstacles(),
                   sim.lastDirection(), sim.lastPower(), tick);

    std::cout << "\n[demo complete]\n";
    return 0;
}
