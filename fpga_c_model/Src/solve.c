#include "solve.h"
#include "e1b_channel.h"
#include "channel.h"
#include "e1b_ephemeris.h"
#include "ephemeris.h"
#include "gps.h"
#include "math.h"
#include "stdio.h"
#include "logging.h"
#include "stdlib.h"

uint8_t solve(channel_t *chan, int num_chans, e1b_channel_t *e1b_chan, int num_e1b_chans, double *x, double *y, double *z, double *t_bias)
{
    *x = 0;
    *y = 0;
    *z = 0;
    *t_bias = 0;

    double *t_tx = (double*)malloc(sizeof(double) * (num_chans + num_e1b_chans));
    double *x_sat = (double*)malloc(sizeof(double) * (num_chans + num_e1b_chans));
    double *y_sat = (double*)malloc(sizeof(double) * (num_chans + num_e1b_chans));
    double *z_sat = (double*)malloc(sizeof(double) * (num_chans + num_e1b_chans));
    double *weights = (double*)malloc(sizeof(double) * (num_chans + num_e1b_chans));
    double *dPR = (double*)malloc(sizeof(double) * (num_chans + num_e1b_chans));

    double **jac = (double**)malloc(sizeof(double*) * (num_chans + num_e1b_chans));
    for (int i = 0; i < num_chans + num_e1b_chans; i++)
    {
        jac[i] = (double*)malloc(sizeof(double) * 4);
    }

    double *mc[4];
    for (int i = 0; i < 4; i++)
    {
        mc[i] = (double*)malloc(sizeof(double) * (num_chans + num_e1b_chans));
    }

    double ma[4][4], mb[4][4], md[4];
    double t_rx;
    double t_pc = 0;

    for (int i = 0; i < num_chans; i++)
    {
        t_tx[i] = get_tx_time(&chan[i]);
        t_tx[i] -= get_clock_correction(&chan[i].ephm, t_tx[i]);
        get_satellite_ecef(&chan[i].ephm, t_tx[i], x_sat + i, y_sat + i, z_sat + i);
        // printf("ecef: %g, %g, %g\n", x_sat[i], y_sat[i], z_sat[i]);
        t_pc += t_tx[i];
        printf("t_tx[%d]: %.5f\n", i, t_tx[i]);
        weights[i] = 1;
    }

    for (int i = num_chans; i < num_e1b_chans + num_chans; i++)
    {
        t_tx[i] = e1b_get_tx_time(&e1b_chan[i - num_chans]);
        t_tx[i] -= e1b_get_clock_correction(&e1b_chan[i - num_chans].ephm, t_tx[i]);
        e1b_get_satellite_ecef(&e1b_chan[i - num_chans].ephm, t_tx[i], x_sat + i, y_sat + i, z_sat + i);
        // printf("ecef: %g, %g, %g\n", x_sat[i], y_sat[i], z_sat[i]);
        t_pc += t_tx[i];
        printf("t_tx[%d]: %.5f\n", i, t_tx[i]);
        weights[i] = 1;
    }

    num_chans = num_chans + num_e1b_chans;

    // Starting value for receiver time
    t_pc = t_pc / num_chans + 75e-3;

    // Taylor series solution
    int i;
    for (i = 0; i < MAX_ITER; i++)
    {
        t_rx = t_pc - *t_bias;

        for (int j = 0; j < num_chans; j++)
        {
            // SV position in ECI
            double theta = (t_tx[j] - t_rx) * omega_e;
            double x_sat_eci = x_sat[j] * cos(theta) - y_sat[j] * sin(theta);
            double y_sat_eci = x_sat[j] * sin(theta) + y_sat[j] * cos(theta);
            double z_sat_eci = z_sat[j];

            // Geometric range
            double gr = sqrt(
                (*x - x_sat_eci) * (*x - x_sat_eci) +
                (*y - y_sat_eci) * (*y - y_sat_eci) +
                (*z - z_sat_eci) * (*z - z_sat_eci));

            // Pseudorange error
            dPR[j] = C * (t_rx - t_tx[j]) - gr;

            // Jacobian
            jac[j][0] = (*x - x_sat_eci) / gr;
            jac[j][1] = (*y - y_sat_eci) / gr;
            jac[j][2] = (*z - z_sat_eci) / gr;
            jac[j][3] = C;
        }

        // ma = H^T * W * H
        for (int r = 0; r < 4; r++)
        {
            for (int c = 0; c < 4; c++)
            {
                ma[r][c] = 0;
                for (int j = 0; j < num_chans; j++)
                {
                    ma[r][c] += jac[j][r] * weights[j] * jac[j][c];
                }
            }
        }

        // Determinant
        double determinant =
            ma[0][3] * ma[1][2] * ma[2][1] * ma[3][0] - ma[0][2] * ma[1][3] * ma[2][1] * ma[3][0] - ma[0][3] * ma[1][1] * ma[2][2] * ma[3][0] + ma[0][1] * ma[1][3] * ma[2][2] * ma[3][0] +
            ma[0][2] * ma[1][1] * ma[2][3] * ma[3][0] - ma[0][1] * ma[1][2] * ma[2][3] * ma[3][0] - ma[0][3] * ma[1][2] * ma[2][0] * ma[3][1] + ma[0][2] * ma[1][3] * ma[2][0] * ma[3][1] +
            ma[0][3] * ma[1][0] * ma[2][2] * ma[3][1] - ma[0][0] * ma[1][3] * ma[2][2] * ma[3][1] - ma[0][2] * ma[1][0] * ma[2][3] * ma[3][1] + ma[0][0] * ma[1][2] * ma[2][3] * ma[3][1] +
            ma[0][3] * ma[1][1] * ma[2][0] * ma[3][2] - ma[0][1] * ma[1][3] * ma[2][0] * ma[3][2] - ma[0][3] * ma[1][0] * ma[2][1] * ma[3][2] + ma[0][0] * ma[1][3] * ma[2][1] * ma[3][2] +
            ma[0][1] * ma[1][0] * ma[2][3] * ma[3][2] - ma[0][0] * ma[1][1] * ma[2][3] * ma[3][2] - ma[0][2] * ma[1][1] * ma[2][0] * ma[3][3] + ma[0][1] * ma[1][2] * ma[2][0] * ma[3][3] +
            ma[0][2] * ma[1][0] * ma[2][1] * ma[3][3] - ma[0][0] * ma[1][2] * ma[2][1] * ma[3][3] - ma[0][1] * ma[1][0] * ma[2][2] * ma[3][3] + ma[0][0] * ma[1][1] * ma[2][2] * ma[3][3];

        // mb = inverse(ma)
        mb[0][0] = (ma[1][2] * ma[2][3] * ma[3][1] - ma[1][3] * ma[2][2] * ma[3][1] + ma[1][3] * ma[2][1] * ma[3][2] - ma[1][1] * ma[2][3] * ma[3][2] - ma[1][2] * ma[2][1] * ma[3][3] + ma[1][1] * ma[2][2] * ma[3][3]) / determinant;
        mb[0][1] = (ma[0][3] * ma[2][2] * ma[3][1] - ma[0][2] * ma[2][3] * ma[3][1] - ma[0][3] * ma[2][1] * ma[3][2] + ma[0][1] * ma[2][3] * ma[3][2] + ma[0][2] * ma[2][1] * ma[3][3] - ma[0][1] * ma[2][2] * ma[3][3]) / determinant;
        mb[0][2] = (ma[0][2] * ma[1][3] * ma[3][1] - ma[0][3] * ma[1][2] * ma[3][1] + ma[0][3] * ma[1][1] * ma[3][2] - ma[0][1] * ma[1][3] * ma[3][2] - ma[0][2] * ma[1][1] * ma[3][3] + ma[0][1] * ma[1][2] * ma[3][3]) / determinant;
        mb[0][3] = (ma[0][3] * ma[1][2] * ma[2][1] - ma[0][2] * ma[1][3] * ma[2][1] - ma[0][3] * ma[1][1] * ma[2][2] + ma[0][1] * ma[1][3] * ma[2][2] + ma[0][2] * ma[1][1] * ma[2][3] - ma[0][1] * ma[1][2] * ma[2][3]) / determinant;
        mb[1][0] = (ma[1][3] * ma[2][2] * ma[3][0] - ma[1][2] * ma[2][3] * ma[3][0] - ma[1][3] * ma[2][0] * ma[3][2] + ma[1][0] * ma[2][3] * ma[3][2] + ma[1][2] * ma[2][0] * ma[3][3] - ma[1][0] * ma[2][2] * ma[3][3]) / determinant;
        mb[1][1] = (ma[0][2] * ma[2][3] * ma[3][0] - ma[0][3] * ma[2][2] * ma[3][0] + ma[0][3] * ma[2][0] * ma[3][2] - ma[0][0] * ma[2][3] * ma[3][2] - ma[0][2] * ma[2][0] * ma[3][3] + ma[0][0] * ma[2][2] * ma[3][3]) / determinant;
        mb[1][2] = (ma[0][3] * ma[1][2] * ma[3][0] - ma[0][2] * ma[1][3] * ma[3][0] - ma[0][3] * ma[1][0] * ma[3][2] + ma[0][0] * ma[1][3] * ma[3][2] + ma[0][2] * ma[1][0] * ma[3][3] - ma[0][0] * ma[1][2] * ma[3][3]) / determinant;
        mb[1][3] = (ma[0][2] * ma[1][3] * ma[2][0] - ma[0][3] * ma[1][2] * ma[2][0] + ma[0][3] * ma[1][0] * ma[2][2] - ma[0][0] * ma[1][3] * ma[2][2] - ma[0][2] * ma[1][0] * ma[2][3] + ma[0][0] * ma[1][2] * ma[2][3]) / determinant;
        mb[2][0] = (ma[1][1] * ma[2][3] * ma[3][0] - ma[1][3] * ma[2][1] * ma[3][0] + ma[1][3] * ma[2][0] * ma[3][1] - ma[1][0] * ma[2][3] * ma[3][1] - ma[1][1] * ma[2][0] * ma[3][3] + ma[1][0] * ma[2][1] * ma[3][3]) / determinant;
        mb[2][1] = (ma[0][3] * ma[2][1] * ma[3][0] - ma[0][1] * ma[2][3] * ma[3][0] - ma[0][3] * ma[2][0] * ma[3][1] + ma[0][0] * ma[2][3] * ma[3][1] + ma[0][1] * ma[2][0] * ma[3][3] - ma[0][0] * ma[2][1] * ma[3][3]) / determinant;
        mb[2][2] = (ma[0][1] * ma[1][3] * ma[3][0] - ma[0][3] * ma[1][1] * ma[3][0] + ma[0][3] * ma[1][0] * ma[3][1] - ma[0][0] * ma[1][3] * ma[3][1] - ma[0][1] * ma[1][0] * ma[3][3] + ma[0][0] * ma[1][1] * ma[3][3]) / determinant;
        mb[2][3] = (ma[0][3] * ma[1][1] * ma[2][0] - ma[0][1] * ma[1][3] * ma[2][0] - ma[0][3] * ma[1][0] * ma[2][1] + ma[0][0] * ma[1][3] * ma[2][1] + ma[0][1] * ma[1][0] * ma[2][3] - ma[0][0] * ma[1][1] * ma[2][3]) / determinant;
        mb[3][0] = (ma[1][2] * ma[2][1] * ma[3][0] - ma[1][1] * ma[2][2] * ma[3][0] - ma[1][2] * ma[2][0] * ma[3][1] + ma[1][0] * ma[2][2] * ma[3][1] + ma[1][1] * ma[2][0] * ma[3][2] - ma[1][0] * ma[2][1] * ma[3][2]) / determinant;
        mb[3][1] = (ma[0][1] * ma[2][2] * ma[3][0] - ma[0][2] * ma[2][1] * ma[3][0] + ma[0][2] * ma[2][0] * ma[3][1] - ma[0][0] * ma[2][2] * ma[3][1] - ma[0][1] * ma[2][0] * ma[3][2] + ma[0][0] * ma[2][1] * ma[3][2]) / determinant;
        mb[3][2] = (ma[0][2] * ma[1][1] * ma[3][0] - ma[0][1] * ma[1][2] * ma[3][0] - ma[0][2] * ma[1][0] * ma[3][1] + ma[0][0] * ma[1][2] * ma[3][1] + ma[0][1] * ma[1][0] * ma[3][2] - ma[0][0] * ma[1][1] * ma[3][2]) / determinant;
        mb[3][3] = (ma[0][1] * ma[1][2] * ma[2][0] - ma[0][2] * ma[1][1] * ma[2][0] + ma[0][2] * ma[1][0] * ma[2][1] - ma[0][0] * ma[1][2] * ma[2][1] - ma[0][1] * ma[1][0] * ma[2][2] + ma[0][0] * ma[1][1] * ma[2][2]) / determinant;

        // mc = inverse(H^T * W * H) * H^T
        for (int r = 0; r < 4; r++)
        {
            for (int c = 0; c < num_chans; c++)
            {
                mc[r][c] = 0;
                for (int j = 0; j < 4; j++)
                {
                    mc[r][c] += mb[r][j] * jac[c][j];
                }
            }
        }

        // md = inverse(H^T * W * H) * H^T * W * dPR
        for (int r = 0; r < 4; r++)
        {
            md[r] = 0;
            for (int j = 0; j < num_chans; j++)
            {
                md[r] += mc[r][j] * weights[j] * dPR[j];
            }
        }

        double dx = md[0];
        double dy = md[1];
        double dz = md[2];
        double dt = md[3];

        double error = sqrt(dx * dx + dy * dy + dz * dz);
        if (i + 1 == MAX_ITER)
            printf("error: %.5f\n", error);

        if (error < 1.0) {
            printf("error: %.5f\n", error);
            break;
        }
        *x += dx;
        *y += dy;
        *z += dz;
        *t_bias += dt;
    }

    return (i != MAX_ITER);
}

void to_coords(double x, double y, double z, double *lat, double *lon, double *alt)
{
    const double a = WGS84_A;
    const double e2 = WGS84_E2;
    const double p = sqrt(x * x + y * y);

    *lon = 2.0 * atan2(y, x + p);
    *lat = atan(z / (p * (1.0 - e2)));
    *alt = 0.0;

    for (int i = 0; i < 100; i++)
    {
        double N = a / sqrt(1.0 - e2 * sin(*lat) * sin(*lat));
        double alt_new = p / cos(*lat) - N;
        *lat = atan(z / (p * (1.0 - e2 * N / (N + alt_new))));
        if (fabs(alt_new - *alt) < 1e-3)
        {
            *alt = alt_new;
            break;
        }
        *alt = alt_new;
    }

    *lat *= 180.0 / PI;
    *lon *= 180.0 / PI;
}