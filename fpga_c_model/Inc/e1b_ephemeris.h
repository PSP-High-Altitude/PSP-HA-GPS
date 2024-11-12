#ifndef E1B_EPHEMERIS_H
#define E1B_EPHEMERIS_H

#include "stdint.h"
#include "gps.h"

typedef struct
{
    // Page 0 - WN and TOW
    uint16_t wn;
    uint32_t tow;

    // Page 1 - Ephmeris 1/4
    uint16_t t_oe;
    int32_t M_0;
    uint32_t e;
    uint32_t root_A;

    // Page 2 - Ephemeris 2/4
    int32_t Omega_0;
    int32_t i_0;
    int32_t omega;
    int16_t IDOT;

    // Page 3 - Ephemeris 3/4, SISA
    int32_t omega_dot;
    int16_t delta_n;
    int16_t C_uc;
    int16_t C_us;
    int16_t C_rc;
    int16_t C_rs;

    // Page 4 - SVID, Ephemeris 4/4, clock correction
    int16_t C_ic;
    int16_t C_is;
    uint16_t t_oc;
    int32_t a_f0;
    int32_t a_f1;
    int8_t a_f2;

    // Page 5 - Ionospheric correction, BGD, health and validity, GST
    uint16_t a_i0;
    int16_t a_i1;
    int16_t a_i2;
    int16_t BGD;
    uint8_t signal_health;
    uint8_t data_validity;

    // Page 10 - GST-GPS conversion parameters
    int16_t A_0G;
    int16_t A_1G;
    uint8_t t_0G;
    uint8_t WN_0G;

} e1b_ephemeris_t;

void e1b_get_satellite_ecef(e1b_ephemeris_t *ephm, double t, double *x, double *y, double *z);

double e1b_get_clock_correction(e1b_ephemeris_t *ephm, double t);

#endif