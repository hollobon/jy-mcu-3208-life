/* Conway's Game of Life for JY-MCU 3208 "Lattice Clock"
 * 
 * Copyright (C) Pete Hollobon 2016
 */

#ifndef LIFE_H
#define LIFE_H

#include <stdint.h>
#include "character.h"

const uint8_t _glider[] = {0x8, 0x4, 0x1c};

const uint8_t _lwss[] = {0xa, 0x1, 0x1, 0x9, 0x7};

const uint8_t _rpentomino[] = {0x4, 0xe, 0x2};

const uint8_t _diehard[] = {0x4, 0xc, 0x0, 0x0, 0x0, 0x8, 0xa, 0x8};

const uint8_t _acorn[] = {0x8, 0xc, 0x0, 0x4, 0x8, 0x8, 0x8, 0x8};

character patterns[]= {
    CHARDEF(_glider),
    CHARDEF(_lwss),
    CHARDEF(_rpentomino),
    CHARDEF(_diehard),
    CHARDEF(_acorn),
};

#endif  /* LIFE_H */
