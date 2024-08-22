#ifndef E1B_EPHEMERIS_H
#define E1B_EPHEMERIS_H

#include "stdint.h"
#define mu 3.986005e14
#define omega_e 7.2921151467e-5
#define F -4.442807633e-10
#define E_K_ITER 20

typedef struct
{
    // Page 0 - WN and TOW
    uint16_t wn;
    uint32_t tow;

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
} e1b_ephemeris_t;

void e1b_get_satellite_ecef(e1b_ephemeris_t *ephm, double t, double *x, double *y, double *z);

double e1b_get_clock_correction(e1b_ephemeris_t *ephm, double t);

#endif