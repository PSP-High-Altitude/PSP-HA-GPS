#include "ephemeris.h"
#include "channel.h"
#include "gps.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "tools.h"

void save_ephemeris_data(channel_t *chan)
{
    ephemeris_t *ephm = &chan->ephm;
    uint8_t *buf = chan->nav_buf;

    uint8_t subframe_id;
    bytes_to_number(&subframe_id, buf + 49, 8, 3, 0);

    if (subframe_id == 0x1)
    {
        bytes_to_number(&ephm->ura, buf + 72, 8, 4, 0);
        bytes_to_number(&ephm->sv_health, buf + 76, 8, 6, 0);
        bytes_to_number(&ephm->T_GD, buf + 196, 8, 8, 1);
        bytes_to_number(&ephm->t_oc, buf + 218, 16, 16, 0);
        bytes_to_number(&ephm->a_f2, buf + 240, 8, 8, 1);
        bytes_to_number(&ephm->a_f1, buf + 248, 16, 16, 1);
        bytes_to_number(&ephm->a_f0, buf + 270, 32, 22, 1);
    }
    else if (subframe_id == 0x2)
    {
        bytes_to_number(&ephm->C_rs, buf + 68, 16, 16, 1);
        bytes_to_number(&ephm->delta_n, buf + 90, 16, 16, 1);
        int32_t M_0_up;
        int32_t M_0_dn;
        bytes_to_number(&M_0_up, buf + 106, 32, 8, 0);
        bytes_to_number(&M_0_dn, buf + 120, 32, 24, 0);
        ephm->M_0 = (M_0_up << 24) | M_0_dn;
        bytes_to_number(&ephm->C_uc, buf + 150, 16, 16, 1);
        int32_t e_up;
        int32_t e_dn;
        bytes_to_number(&e_up, buf + 166, 32, 8, 0);
        bytes_to_number(&e_dn, buf + 180, 32, 24, 0);
        ephm->e = (e_up << 24) | e_dn;
        bytes_to_number(&ephm->C_us, buf + 210, 16, 16, 1);
        uint32_t root_A_up;
        uint32_t root_A_dn;
        bytes_to_number(&root_A_up, buf + 226, 32, 8, 0);
        bytes_to_number(&root_A_dn, buf + 240, 32, 24, 0);
        ephm->root_A = (root_A_up << 24) | root_A_dn;
        bytes_to_number(&ephm->t_oe, buf + 270, 16, 16, 0);
    }
    else if (subframe_id == 0x3)
    {
        bytes_to_number(&ephm->C_ic, buf + 60, 16, 16, 1);
        int32_t Omega_0_up;
        int32_t Omega_0_dn;
        bytes_to_number(&Omega_0_up, buf + 76, 32, 8, 0);
        bytes_to_number(&Omega_0_dn, buf + 90, 32, 24, 0);
        ephm->Omega_0 = (Omega_0_up << 24) | Omega_0_dn;
        bytes_to_number(&ephm->C_is, buf + 120, 16, 16, 1);
        int32_t i_0_up;
        int32_t i_0_dn;
        bytes_to_number(&i_0_up, buf + 136, 32, 8, 0);
        bytes_to_number(&i_0_dn, buf + 150, 32, 24, 0);
        ephm->i_0 = (i_0_up << 24) | i_0_dn;
        bytes_to_number(&ephm->C_rc, buf + 180, 16, 16, 1);
        int32_t omega_up;
        int32_t omega_dn;
        bytes_to_number(&omega_up, buf + 196, 32, 8, 0);
        bytes_to_number(&omega_dn, buf + 210, 32, 24, 0);
        ephm->omega = (omega_up << 24) | omega_dn;
        bytes_to_number(&ephm->omega_dot, buf + 240, 32, 24, 1);
        bytes_to_number(&ephm->IDOT, buf + 278, 16, 14, 1);
    }
    else if (subframe_id == 0x4)
    {
        uint8_t sv_id;
        bytes_to_number(&sv_id, buf + 62, 8, 6, 0);
        if (sv_id == 56)
        {
            bytes_to_number(&ephm->alpha_0, buf + 68, 8, 8, 1);
            bytes_to_number(&ephm->alpha_1, buf + 76, 8, 8, 1);
            bytes_to_number(&ephm->alpha_2, buf + 90, 8, 8, 1);
            bytes_to_number(&ephm->alpha_3, buf + 98, 8, 8, 1);
            bytes_to_number(&ephm->beta_0, buf + 106, 8, 8, 1);
            bytes_to_number(&ephm->beta_1, buf + 120, 8, 8, 1);
            bytes_to_number(&ephm->beta_2, buf + 128, 8, 8, 1);
            bytes_to_number(&ephm->beta_3, buf + 136, 8, 8, 1);
        }
    }
}

double time_from_epoch(double t, double t_epoch)
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

double eccentric_anomaly(ephemeris_t *ephm, double t_k)
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

void get_satellite_ecef(ephemeris_t *ephm, double t, double *x, double *y, double *z)
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
    double t_oe = ephm->t_oe * pow(2, 4);

    double A = root_A * root_A;
    double t_k = time_from_epoch(t, t_oe);
    double E_k = eccentric_anomaly(ephm, t_k);
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
}

double get_clock_correction(ephemeris_t *ephm, double t)
{
    double a_f0 = ephm->a_f0 * pow(2, -31);
    double a_f1 = ephm->a_f1 * pow(2, -43);
    double a_f2 = ephm->a_f2 * pow(2, -55);
    double root_A = ephm->root_A * pow(2, -19);
    double e = ephm->e * pow(2, -33);
    double t_oe = ephm->t_oe * pow(2, 4);
    double t_oc = ephm->t_oc * pow(2, 4);
    double T_GD = ephm->T_GD * pow(2, -31);

    double t_k = time_from_epoch(t, t_oe);
    double E_k = eccentric_anomaly(ephm, t_k);
    double t_R = F * e * root_A * sin(E_k);
    t = time_from_epoch(t, t_oc);
    printf("clock correction: %g\n", a_f0 + a_f1 * t + a_f2 * (t * t) + t_R - T_GD);

    return a_f0 +
           a_f1 * t +
           a_f2 * (t * t) +
           t_R - T_GD;
}