#pragma once

#include "quantum.h"
#include <stdint.h>
#include <stdbool.h>

#define XXX KC_NO

#define LAYOUT( \
    k00, k01, k02, k03, k04, k05, k06,  k49, k4A, k4B, k4C, k4D, k4E, k4F, k48,  k0E, \
    k07, k10, k11, k12, k13, k14,  k39, k3A, k3B, k3C, k3D, k3E, k3F, k38,       k0D, \
    k17, k20, k21, k22, k16, k15,  k29, k2A, k2B, k2C, k2D, k2E,      k28,       k2F, \
    k27,      k31, k26, k25, k24, k23,  k1A, k1B, k1C, k1D, k1E, k1F, k18,            \
    k37,      k35,      k34, k33,       k19, k0A,      k0B, k0C, k0F, k08             \
) { \
    { k00, k01, k02, k03, k04, k05, k06, k07,   k08, XXX, k0A, k0B, k0C, XXX, k0E, k0F }, \
    { k10, k11, k12, k13, k14, k15, k16, k17,   k18, k19, k1A, k1B, k1C, k1D, k1E, k1F }, \
    { k20, k21, k22, k23, k24, k25, k26, k27,   k28, k29, k2A, k2B, k2C, k2D, k2E, k2F }, \
    { XXX, k31, XXX, k33, k34, k35, XXX, k37,   k38, k39, k3A, k3B, k3C, k3D, k3E, k3F }, \
    { XXX, XXX, XXX, XXX, XXX, XXX, XXX, XXX,   k48, k49, k4A, k4B, k4C, k4D, k4E, k4F } \
}

