#ifndef CHANNEL_H
#define CHANNEL_H

#include "gps.h"
#include "stdint.h"
#include "ephemeris.h"

#define NAV_BUFFER_SIZE 300

typedef struct
{
    uint8_t T0;
    uint8_t T1;
    uint16_t g1;
    uint16_t g2;
    uint16_t chip;
} ca_t;

typedef struct
{
    int chan_num;
    int sv;
    uint32_t ca_rate;
    uint32_t lo_rate;
    uint32_t ca_phase;
    uint32_t lo_phase;
    uint64_t ca_freq_integrator;
    uint64_t lo_freq_integrator;
    uint64_t phase_offset;

    ca_t ca;
    int64_t ie;
    int64_t qe;
    int64_t ip;
    int64_t qp;
    int64_t il;
    int64_t ql;
    uint8_t ca_e;
    uint8_t ca_p;
    uint8_t ca_l;
    uint8_t ca_en;

    int64_t ip_hist[1000];
    int64_t qp_hist[1000];
    int64_t prompt_hist_len;

    double cn0;

    uint8_t tracked_this_epoch;
    uint8_t wait_frames;
    int nav_ms;
    uint64_t total_ms;
    uint8_t nav_buf[NAV_BUFFER_SIZE + 1];
    uint16_t nav_bit_count;
    uint8_t nav_valid;
    int32_t last_z_count;

    uint8_t bit_sync_locked;
    uint8_t last_bit;
    int bit_sync_count;
    int bit_edge_hist[20];
    int8_t bit_avg;

    ephemeris_t ephm;
} channel_t;

void save_ephemeris_data(channel_t *chan);
void clock_ca(ca_t *ca);
uint8_t get_ca(ca_t *ca);
void init_channel(channel_t *chan, int chan_num, int sv, double lo_dop, double ca_shift);
void clock_channel(channel_t *chan, uint8_t sample);
double get_tx_time(channel_t *chan);

#endif