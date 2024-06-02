#include "solve.h"
#include "channel.h"
#include "gps.h"
#include "math.h"
#include "stdio.h"

extern const int SVs[];

double get_tx_time(channel_t *chan)
{
    ca_t ca;
    ca.T0 = SVs[chan->sv * 4 + 2];
    ca.T1 = SVs[chan->sv * 4 + 3];
    ca.g1 = 0x3FF;
    ca.g2 = 0x3FF;
    uint16_t chips = 0;
    while (ca.g1 != chan->ca.g1)
    {
        chips++;
        clock_ca(&ca);
    }

    double t = chan->last_z_count * 6.0 +
               chan->nav_bit_count / 50.0 +
               chan->nav_ms / 1000.0 +
               chips / 1023000.0 +
               (chan->ca_phase + (1U << 31)) * pow(2, -32) / 1023000.0;

    // if (chan->sv + 1 == 29)
    //     printf("%d,%u,%u,%u,%lu,%.10f\n", chan->last_z_count, chan->nav_bit_count, chan->nav_ms, chips, chan->ca_phase, t);

    return t;
}

uint8_t solve(channel_t **chan, int num_chans, double *x, double *y, double *z, double *t_bias)
{
    *x = 0;
    *y = 0;
    *z = 0;
    *t_bias = 0;

    double t_tx[NUM_CHANNELS];
    double x_sat[NUM_CHANNELS];
    double y_sat[NUM_CHANNELS];
    double z_sat[NUM_CHANNELS];
    double weights[NUM_CHANNELS];
    double dPR[NUM_CHANNELS];
    double jac[NUM_CHANNELS][4], ma[4][4], mb[4][4], mc[4][NUM_CHANNELS], md[4];
    double t_rx;

    double t_pc = 0;

    for (int i = 0; i < num_chans; i++)
    {
        t_tx[i] = get_tx_time(chan[i]);
        t_tx[i] -= get_clock_correction(&chan[i]->ephm, t_tx[i]);
        get_satellite_ecef(&chan[i]->ephm, t_tx[i], x_sat + i, y_sat + i, z_sat + i);
        t_pc += t_tx[i];
        weights[i] = 1;
    }

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

        if (error < 1.0)
            break;
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