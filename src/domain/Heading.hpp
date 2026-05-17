#pragma once

enum class Heading { NORTH, EAST, SOUTH, WEST };

inline const char* toString(Heading h) {
    switch (h) {
        case Heading::NORTH: return "NORTH";
        case Heading::EAST:  return "EAST";
        case Heading::SOUTH: return "SOUTH";
        case Heading::WEST:  return "WEST";
    }
    return "UNKNOWN";
}

inline char toArrow(Heading h) {
    switch (h) {
        case Heading::NORTH: return '^';
        case Heading::EAST:  return '>';
        case Heading::SOUTH: return 'v';
        case Heading::WEST:  return '<';
    }
    return '?';
}
