#ifndef CHANNEL_H
#define CHANNEL_H

#include "gps.h"
#include "stdint.h"

#define NAV_BUFFER_SIZE 300

typedef struct {
    uint8_t T0;
    uint8_t T1;
    uint16_t g1;
    uint16_t g2;
} ca_t;

typedef struct {
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

    uint8_t tracked_this_epoch;
    uint8_t first_bit;
    uint8_t wait_epoch;
    uint8_t nav_ms;
    uint32_t total_ms;
    uint8_t nav_buf[NAV_BUFFER_SIZE+1];
    uint16_t nav_bit_count;
    uint8_t nav_valid;
    int32_t last_z_count;
} channel_t;

void init_channel(channel_t *chan, int chan_num, int sv, double lo_dop, double ca_shift);
void clock_channel(channel_t *chan, uint8_t sample);

#endif