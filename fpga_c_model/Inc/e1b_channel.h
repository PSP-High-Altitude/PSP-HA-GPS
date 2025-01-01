#ifndef E1B_CHANNEL_H
#define E1B_CHANNEL_H

#include "gps.h"
#include "stdint.h"
#include "e1b_ephemeris.h"
#include "tools.h"

#define NAV_BUFFER_SIZE 300

typedef struct
{
    uint8_t sv;
    uint16_t chip;
} e1b_ca_t;

typedef enum
{
    E1_TRACKING_UNLOCKED = 0,
    E1_TRACKING_PILOT_NO_SEC = 1,
    E1_TRACKING_PILOT_SEC = 2,
} e1b_tracking_state;

// State machine for assigning all the correlator outputs dynamically
typedef enum {
    E1_CODE_VE,
    E1_CODE_E,
    E1_CODE_P,
    E1_CODE_L,
    E1_CODE_VL,
} e1b_correlator_state;

typedef struct
{
    int chan_num;
    int sv;
    double start_lo_freq;
    double start_ca_freq;
    uint32_t ca_rate;
    uint32_t lo_rate;
    uint32_t ca_phase;
    double last_clock;
    uint32_t lo_phase;
    uint64_t ca_freq_integrator;
    uint64_t lo_freq_integrator;
    uint64_t phase_offset;
    e1b_tracking_state tracking_state;
    e1b_correlator_state correlator_state;
    fll_pll_t fll_pll;
    fll_pll_t dll;

    e1b_ca_t ca;
    e1b_ca_t sec_ca;
    int64_t ive;
    int64_t qve;
    int64_t ie;
    int64_t qe;
    int64_t ip;
    int64_t qp;
    int64_t il;
    int64_t ql;
    int64_t ivl;
    int64_t qvl;
    uint8_t ca_ve;
    uint8_t ca_e;
    uint8_t ca_p;
    uint8_t ca_l;
    uint8_t ca_vl;
    uint8_t ca_en;
    uint8_t ca_p_data;
    int64_t ip_data;

    int64_t last_ip;
    int64_t last_qp;

    int64_t ip_hist[250];
    int64_t qp_hist[250];
    int64_t prompt_hist_len;

    double cn0;
    
    uint8_t secondary_hist[25];
    uint8_t secondary_hist_inv[25];
    uint8_t secondary_hist_len;
    uint8_t secondary_pol;

    uint8_t tracked_this_epoch;
    uint8_t wait_frames;
    uint8_t nav_ms;
    uint32_t total_ms;
    uint8_t nav_buf[NAV_BUFFER_SIZE + 1];
    uint16_t nav_bit_count;
    uint8_t nav_valid;
    uint8_t last_bit;
    uint8_t last_page_half;
    uint8_t last_page_type;
    uint8_t last_t0;
    uint8_t data[128];
    uint8_t crc[24];
    uint8_t page_parts;
    uint32_t tGST;

    e1b_ephemeris_t ephm;
} e1b_channel_t;

void e1b_clock_ca(e1b_ca_t *ca);
void e1b_clock_secondary_ca(e1b_ca_t *ca);
uint8_t e1b_get_ca(e1b_ca_t *ca);
uint8_t e1b_get_pilot_ca(e1b_ca_t *ca);
uint8_t e1b_get_secondary_ca(e1b_ca_t *ca);
void e1b_init_channel(e1b_channel_t *chan, int chan_num, int sv, double lo_dop, double ca_shift);
void e1b_clock_channel(e1b_channel_t *chan, uint8_t sample);
void e1b_save_ephemeris_data(e1b_channel_t *chan);
double e1b_get_tx_time(e1b_channel_t *chan);

#endif