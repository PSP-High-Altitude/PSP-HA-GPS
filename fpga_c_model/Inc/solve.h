#ifndef SOLVE_H
#define SOLVE_H

#include "channel.h"
#include "e1b_channel.h"

#define WGS84_A 6378137.0
#define WGS84_F_INV 298.257223563
#define WGS84_B 6356752.31424518
#define WGS84_E2 0.00669437999014132
#define C 299792458.0

#define MAX_ITER 20

uint8_t solve(channel_t *chan, int num_chans, e1b_channel_t *e1b_chan, int num_e1b_chans, double *x, double *y, double *z, double *t_bias);

void to_coords(double x, double y, double z, double *lat, double *lon, double *alt);

#endif