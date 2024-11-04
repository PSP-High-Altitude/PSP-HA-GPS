#ifndef EPHEMERIS_H
#define EPHEMERIS_H

#include "stdint.h"
#include "gps.h"

typedef struct
{
    // Subframe 1
    uint8_t ura;
    uint8_t sv_health;
    int8_t T_GD;
    uint16_t t_oc;
    int8_t a_f2;
    int16_t a_f1;
    int32_t a_f0;

    // Subframes 2 and 3
    int16_t C_rs;
    int16_t delta_n;
    int32_t M_0;
    int16_t C_uc;
    uint32_t e;
    int16_t C_us;
    uint32_t root_A;
    uint16_t t_oe;
    int16_t C_ic;
    int32_t Omega_0;
    int16_t C_is;
    int32_t i_0;
    int16_t C_rc;
    int32_t omega;
    int32_t omega_dot;
    int16_t IDOT;

    // Subframe 4 page 18
    int8_t alpha_0;
    int8_t alpha_1;
    int8_t alpha_2;
    int8_t alpha_3;
    int8_t beta_0;
    int8_t beta_1;
    int8_t beta_2;
    int8_t beta_3;
} ephemeris_t;

void save_ephemeris_data(uint8_t *buf, ephemeris_t *ephm);

void get_satellite_ecef(ephemeris_t *ephm, double t, double *x, double *y, double *z);

double get_clock_correction(ephemeris_t *ephm, double t);

#endif