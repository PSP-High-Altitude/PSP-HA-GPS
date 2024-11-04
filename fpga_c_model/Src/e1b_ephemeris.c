#include "e1b_channel.h"
#include "gps.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "tools.h"

void e1b_save_ephemeris_data(e1b_channel_t *chan)
{
    e1b_ephemeris_t *ephm = &chan->ephm;

    // Spare page
    if (chan->last_page_type == 0 && chan->data[6] && !chan->data[7])
    {
        bytes_to_number(&ephm->wn, chan->data + 96, 16, 12, 0);
        bytes_to_number(&ephm->tow, chan->data + 108, 32, 20, 0);
        // Resynchronize tGST
        chan->tGST = ((ephm->wn * 604800) + ephm->tow + 2) % ((uint32_t)604800 * 4096);
    }

    // Ephemeris 1/4
    if(chan->last_page_type == 1) {
        bytes_to_number(&ephm->t_oe, chan->data + 16,       16, 14, 0);
        bytes_to_number(&ephm->M_0, chan->data + 30,        32, 32, 1);
        bytes_to_number(&ephm->e, chan->data + 62,          32, 32, 0);
        bytes_to_number(&ephm->root_A, chan->data + 94,     32, 32, 0);
    }

    // Ephemeris 2/4
    if(chan->last_page_type == 2) {
        bytes_to_number(&ephm->Omega_0, chan->data + 16,    32, 32, 1);
        bytes_to_number(&ephm->i_0, chan->data + 48,        32, 32, 1);
        bytes_to_number(&ephm->omega, chan->data + 80,      32, 32, 1);
        bytes_to_number(&ephm->IDOT, chan->data + 112,      16, 14, 1);
    }

    // Ephemeris 3/4, SISA
    if(chan->last_page_type == 2) {
        bytes_to_number(&ephm->omega_dot, chan->data + 16,  32, 24, 1);
        bytes_to_number(&ephm->delta_n, chan->data + 40,    16, 16, 1);
        bytes_to_number(&ephm->C_uc, chan->data + 56,       16, 16, 1);
        bytes_to_number(&ephm->C_us, chan->data + 72,       16, 16, 1);
        bytes_to_number(&ephm->C_rc, chan->data + 88,       16, 16, 1);
        bytes_to_number(&ephm->C_rs, chan->data + 104,      16, 16, 1);
    }

    // SVID, Ephemeris 4/4, clock correction
    if(chan->last_page_type == 2) {
        bytes_to_number(&ephm->C_ic, chan->data + 22,       16, 16, 1);
        bytes_to_number(&ephm->C_is, chan->data + 38,       16, 16, 1);
        bytes_to_number(&ephm->t_oc, chan->data + 54,       16, 14, 0);
        bytes_to_number(&ephm->a_f0, chan->data + 68,       32, 31, 1);
        bytes_to_number(&ephm->C_rc, chan->data + 99,       32, 21, 1);
        bytes_to_number(&ephm->C_rs, chan->data + 120,      8,  6,  1);
    }

    // Ionospheric correction, BGD, health and validity, GST
    if (chan->last_page_type == 5)
    {
        bytes_to_number(&ephm->a_i0, chan->data + 6,        16, 11, 0);
        bytes_to_number(&ephm->a_i1, chan->data + 17,       16, 11, 1);
        bytes_to_number(&ephm->a_i2, chan->data + 28,       16, 14, 1);
        bytes_to_number(&ephm->BGD, chan->data + 57,        16, 10, 1);
        bytes_to_number(&ephm->wn, chan->data + 73,         16, 12, 0);
        bytes_to_number(&ephm->tow, chan->data + 85,        32, 20, 0);
        // Resynchronize tGST
        chan->tGST = ((ephm->wn * 604800) + ephm->tow + 2) % ((uint32_t)604800 * 4096);
    }

    // GST-GPS conversion parameters
    if (chan->last_page_type == 10)
    {
        bytes_to_number(&ephm->A_0G, chan->data + 86,       16, 16, 1);
        bytes_to_number(&ephm->A_1G, chan->data + 102,      16, 12, 1);
        bytes_to_number(&ephm->t_0G, chan->data + 114,      8,  8,  0);
        bytes_to_number(&ephm->WN_0G, chan->data + 122,     8,  6,  0);
    }
}

double e1b_time_from_epoch(double t, double t_epoch)
{
    t -= t_epoch;
    if (t > 302400)
    {
        t -= 604800;
    }
    else if (t < -302400)
    {
        t += 604800;
    }
    return t;
}

double e1b_eccentric_anomaly(e1b_ephemeris_t *ephm, double t_k)
{
    double root_A = ephm->root_A * pow(2, -19);
    double delta_n = ephm->delta_n * pow(2, -43) * PI;
    double M_0 = ephm->M_0 * pow(2, -31) * PI;
    double e = ephm->e * pow(2, -33);

    double A = root_A * root_A;
    double n_0 = sqrt(mu / (A * A * A));
    double n = n_0 + delta_n;
    double M_k = M_0 + n * t_k;
    double E_k = M_k;

    int iter = 0;
    while (iter++ < E_K_ITER)
    {
        double E_k_new = M_k + e * sin(E_k);
        if (fabs(E_k_new - E_k) < 1e-10)
        {
            E_k = E_k_new;
            break;
        }
        E_k = E_k_new;
    }
    return E_k;
}

void e1b_get_satellite_ecef(e1b_ephemeris_t *ephm, double t, double *x, double *y, double *z)
{
    double root_A = ephm->root_A * pow(2, -19);
    double e = ephm->e * pow(2, -33);
    double omega = ephm->omega * pow(2, -31) * PI;
    double C_uc = ephm->C_uc * pow(2, -29);
    double C_us = ephm->C_us * pow(2, -29);
    double C_rc = ephm->C_rc * pow(2, -5);
    double C_rs = ephm->C_rs * pow(2, -5);
    double C_ic = ephm->C_ic * pow(2, -29);
    double C_is = ephm->C_is * pow(2, -29);
    double i_0 = ephm->i_0 * pow(2, -31) * PI;
    double IDOT = ephm->IDOT * pow(2, -43) * PI;
    double Omega_0 = ephm->Omega_0 * pow(2, -31) * PI;
    double omega_dot = ephm->omega_dot * pow(2, -43) * PI;
    double t_oe = ephm->t_oe * 60.0;

    double A = root_A * root_A;
    double t_k = e1b_time_from_epoch(t, t_oe);
    double E_k = e1b_eccentric_anomaly(ephm, t_k);
    double v_k = atan2(sqrt(1 - e * e) * sin(E_k), cos(E_k) - e);
    double phi_k = v_k + omega;
    double delta_u_k = C_us * sin(2 * phi_k) + C_uc * cos(2 * phi_k);
    double delta_r_k = C_rs * sin(2 * phi_k) + C_rc * cos(2 * phi_k);
    double delta_i_k = C_is * sin(2 * phi_k) + C_ic * cos(2 * phi_k);
    double u_k = phi_k + delta_u_k;
    double r_k = A * (1 - e * cos(E_k)) + delta_r_k;
    double i_k = i_0 + delta_i_k + IDOT * t_k;
    double Omega_k = Omega_0 + (omega_dot - omega_e) * t_k - omega_e * t_oe;
    double x_k_prime = r_k * cos(u_k);
    double y_k_prime = r_k * sin(u_k);

    *x = x_k_prime * cos(Omega_k) - y_k_prime * cos(i_k) * sin(Omega_k);
    *y = x_k_prime * sin(Omega_k) + y_k_prime * cos(i_k) * cos(Omega_k);
    *z = y_k_prime * sin(i_k);
    //printf("x: %g, y: %g, z: %g\n", *x, *y, *z);
}

double e1b_get_clock_correction(e1b_ephemeris_t *ephm, double t)
{
    double a_f0 = ephm->a_f0 * pow(2, -34);
    double a_f1 = ephm->a_f1 * pow(2, -46);
    double a_f2 = ephm->a_f2 * pow(2, -59);
    double root_A = ephm->root_A * pow(2, -19);
    double e = ephm->e * pow(2, -33);
    double t_oe = ephm->t_oe * 60;
    double t_oc = ephm->t_oc * 60;
    double BGD = ephm->BGD * pow(2, -32);

    double t_k = e1b_time_from_epoch(t, t_oe);
    double E_k = e1b_eccentric_anomaly(ephm, t_k);
    double t_R = F * e * root_A * sin(E_k);
    t = e1b_time_from_epoch(t, t_oc);

    double A_0G = ephm->A_0G * pow(2, -35);
    double A_1G = ephm->A_1G * pow(2, -51);
    double t_0G = ephm->t_0G * 3600;
    double WN_0G = ephm->WN_0G;
    double dt_systems = A_0G + A_1G * (e1b_time_from_epoch(ephm->tow, t_0G) + (ephm->wn - WN_0G) * 604800);
    printf("dt_systems: %g\n", dt_systems);

    return a_f0 +
           a_f1 * t +
           a_f2 * (t * t) +
           t_R - BGD + dt_systems;
}