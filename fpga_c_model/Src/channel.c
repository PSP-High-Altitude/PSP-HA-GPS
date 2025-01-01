#include "channel.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "waves.h"
#include "logging.h"
#include "tools.h"

extern uint64_t clock;

const int SVs[] = {
    // PRN, Navstar, taps
    1,
    63,
    2,
    6,
    2,
    56,
    3,
    7,
    3,
    37,
    4,
    8,
    4,
    35,
    5,
    9,
    5,
    64,
    1,
    9,
    6,
    36,
    2,
    10,
    7,
    62,
    1,
    8,
    8,
    44,
    2,
    9,
    9,
    33,
    3,
    10,
    10,
    38,
    2,
    3,
    11,
    46,
    3,
    4,
    12,
    59,
    5,
    6,
    13,
    43,
    6,
    7,
    14,
    49,
    7,
    8,
    15,
    60,
    8,
    9,
    16,
    51,
    9,
    10,
    17,
    57,
    1,
    4,
    18,
    50,
    2,
    5,
    19,
    54,
    3,
    6,
    20,
    47,
    4,
    7,
    21,
    52,
    5,
    8,
    22,
    53,
    6,
    9,
    23,
    55,
    1,
    3,
    24,
    23,
    4,
    6,
    25,
    24,
    5,
    7,
    26,
    26,
    6,
    8,
    27,
    27,
    7,
    9,
    28,
    48,
    8,
    10,
    29,
    61,
    1,
    6,
    30,
    39,
    2,
    7,
    31,
    58,
    3,
    8,
    32,
    22,
    4,
    9,
};

void clock_ca(ca_t *ca)
{
    uint16_t g1 = ca->g1;
    uint16_t g2 = ca->g2;
    uint16_t g1_shift_in = ((g1 >> 9) ^ (g1 >> 2)) & 0x1;
    uint16_t g2_shift_in = ((g2 >> 9) ^ (g2 >> 8) ^ (g2 >> 7) ^ (g2 >> 5) ^ (g2 >> 2) ^ (g2 >> 1)) & 0x1;
    ca->g1 = (g1 << 1) | g1_shift_in;
    ca->g2 = (g2 << 1) | g2_shift_in;
    ca->chip = (ca->chip + 1) % 1023;
}

uint8_t get_ca(ca_t *ca)
{
    uint8_t ca_out = ((ca->g1 >> 9) ^ (ca->g2 >> (ca->T0 - 1)) ^ (ca->g2 >> (ca->T1 - 1))) & 0x1;
    return ca_out;
}

uint8_t is_ca_epoch(ca_t *ca)
{
    return (ca->g1 & 0x3FF) == 0x3FF;
}

void init_channel(channel_t *chan, int chan_num, int sv, double lo_dop, double ca_shift)
{
    chan->chan_num = chan_num;
    chan->sv = sv;
    double ca_dop = lo_dop / 1575.42e6 * 1023000.0;
    chan->phase_offset = (uint32_t)(fs / 500 - (ca_shift * fs / 1023000.0)) % ((int64_t)fs / 1000);

    chan->ca_rate = (uint32_t)((1023000.0 + ca_dop) / fs * pow(2, 32));
    chan->lo_rate = (uint32_t)((fc + lo_dop) / fs * pow(2, 32));

    chan->ca_phase = 0;
    chan->lo_phase = 0;

    chan->ca.T0 = SVs[chan->sv * 4 + 2];
    chan->ca.T1 = SVs[chan->sv * 4 + 3];
    chan->ca.g1 = 0x3FF;
    chan->ca.g2 = 0x3FF;
    chan->ca.chip = 0;

    chan->ie = 0, chan->qe = 0, chan->ip = 0, chan->qp = 0, chan->il = 0, chan->ql = 0;
    chan->ca_e = 0, chan->ca_p = 0, chan->ca_l = 0;
    chan->prompt_hist_len = 0;

    chan->ca_freq_integrator = ((uint64_t)chan->ca_rate) << 32;
    chan->lo_freq_integrator = ((uint64_t)chan->lo_rate) << 32;

    chan->ca_en = 0;
    chan->tracked_this_epoch = 0;
    chan->wait_frames = 2;
    chan->nav_ms = 0;
    chan->total_ms = 0;
    chan->nav_valid = 0;
    chan->nav_bit_count = 0;
    chan->last_z_count = 0;
    chan->bit_sync_locked = 0;
    chan->bit_sync_count = 1000;    // 1 second
    chan->last_bit = 0;
    memset(chan->bit_edge_hist, 0, sizeof(chan->bit_edge_hist));
    memset(&chan->ephm, 0, sizeof(ephemeris_t));
}

void do_sample(channel_t *chan, uint8_t sample)
{
    uint8_t lo_i = lo_sin[chan->lo_phase >> 30];
    uint8_t lo_q = lo_cos[chan->lo_phase >> 30];

    uint8_t die, dqe, dip, dqp, dil, dql;

    // Mixers
    die = sample ^ chan->ca_e ^ lo_i;
    dqe = sample ^ chan->ca_e ^ lo_q;
    dip = sample ^ chan->ca_p ^ lo_i;
    dqp = sample ^ chan->ca_p ^ lo_q;
    dil = sample ^ chan->ca_l ^ lo_i;
    dql = sample ^ chan->ca_l ^ lo_q;

    // Integrators
    chan->ie = chan->ca_en ? (chan->ie + (die ? -1 : 1)) : 0;
    chan->qe = chan->ca_en ? (chan->qe + (dqe ? -1 : 1)) : 0;
    chan->ip = chan->ca_en ? (chan->ip + (dip ? -1 : 1)) : 0;
    chan->qp = chan->ca_en ? (chan->qp + (dqp ? -1 : 1)) : 0;
    chan->il = chan->ca_en ? (chan->il + (dil ? -1 : 1)) : 0;
    chan->ql = chan->ca_en ? (chan->ql + (dql ? -1 : 1)) : 0;
}

void process_ip_to_bit(channel_t *chan)
{
    int log_data[2] = {chan->ip, chan->qp};
    logging_log(LOG_EVENT_TYPE_IQ, (void *)&log_data, chan->sv + 1, 0);
    
    // Use the histogram method for synchronizing bits: first we will sample for
    // for a while then pick the most common transition edge as the bit edge point

    // Not locked
    if(chan->bit_sync_locked == 0) {
        // Update histogram if a bit edge is found
        if(chan->last_bit != (chan->ip > 0) ? 1 : 0) {
            chan->bit_edge_hist[chan->nav_ms] += 1;
            chan->last_bit = (chan->ip > 0) ? 1 : 0;
        }

        // Count down
        chan->bit_sync_count--;

        // Check if we have enough samples to make a decision
        if(chan->bit_sync_count == 0) {
            // Find the most common edge
            int max = 0;
            int max_idx = 0;
            for(int i = 0; i < 20; i++) {
                if(chan->bit_edge_hist[i] > max) {
                    max = chan->bit_edge_hist[i];
                    max_idx = i;
                }
            }
            
            // Center the bit edge point to nav_ms = 0
            chan->bit_sync_locked = 1;  
            chan->nav_ms -= max_idx + 1;
            if(chan->nav_ms < 0) chan->nav_ms += 20;
        }    
    } else {    // Locked
        // End of bit period
        if(chan->nav_ms == 0) {
            chan->nav_buf[chan->nav_bit_count] = chan->bit_avg > 0 ? 1 : 0;
            chan->nav_bit_count++;
            chan->bit_avg = 0;
        }
        // Middle of bit period
        chan->bit_avg += (chan->ip > 0) ? 1 : -1;
    }

    chan->nav_ms = (chan->nav_ms + 1) % 20;
    chan->total_ms += 1;

    // printf("SV: %d\n\tBit: %d\n\tms: %d\n", chan->sv + 1, chan->last_bit, chan->nav_ms);
}

uint8_t check_parity(uint8_t *bits, uint8_t *p, uint8_t D29, uint8_t D30)
{
    // D29 and D30 are the bits from the previous word
    uint8_t *d = bits - 1;
    for (uint8_t i = 1; i < 25; i++)
        d[i] ^= D30; // Flip to correct polarity as we go
    p[0] = D29 ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[6] ^ d[10] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[17] ^ d[18] ^ d[20] ^ d[23];
    p[1] = D30 ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[7] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[18] ^ d[19] ^ d[21] ^ d[24];
    p[2] = D29 ^ d[1] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[8] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[19] ^ d[20] ^ d[22];
    p[3] = D30 ^ d[2] ^ d[4] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[20] ^ d[21] ^ d[23];
    p[4] = D30 ^ d[1] ^ d[3] ^ d[5] ^ d[6] ^ d[7] ^ d[9] ^ d[10] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[18] ^ d[21] ^ d[22] ^ d[24];
    p[5] = D29 ^ d[3] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[10] ^ d[11] ^ d[13] ^ d[15] ^ d[19] ^ d[22] ^ d[23] ^ d[24];

    return (memcmp(d + 25, p, 6) == 0);
}

uint8_t process_message(channel_t *chan)
{
    uint8_t preamble_norm[] = {1, 0, 0, 0, 1, 0, 1, 1};
    uint8_t preamble_inv[] = {0, 1, 1, 1, 0, 1, 0, 0};
    uint8_t p[6];

    if (memcmp(chan->nav_buf, preamble_norm, 8) == 0)
        p[4] = p[5] = 0;
    else if (memcmp(chan->nav_buf, preamble_inv, 8) == 0)
        p[4] = p[5] = 1;
    else {
        if(chan->sv == 3) {
            printf("Preamble not found\n");
        }
        return 0;
    }

    uint8_t tmp_buf[300];
    memcpy(tmp_buf, chan->nav_buf, 300);

    // Check 10 word parities
    for (uint8_t i = 0; i < 10; i++)
    {
        if (!check_parity(chan->nav_buf + i * 30, p, p[4], p[5]))
        {
            chan->wait_frames = 2;
            return 0;
        }
    }

    // Verify HOW subframe ID
    uint8_t subframe_id = (chan->nav_buf[29 + 20] << 2) | (chan->nav_buf[29 + 21] << 1) | chan->nav_buf[29 + 22];
    if (subframe_id < 1 || subframe_id > 5)
    {
        chan->wait_frames = 2;
        return 0;
    }

    // Final verification is the Z count incrementing by 1
    int32_t this_z_count = 0;
    for (uint8_t i = 30; i < 30 + 17; i++)
    {
        this_z_count = (this_z_count << 1) | chan->nav_buf[i];
    }
    if ((chan->last_z_count + 1) % 100800 == this_z_count)
    {
        chan->nav_valid = 1;
    }
    else
    {
        chan->nav_valid = 0;
    }
    chan->last_z_count = this_z_count;
    if (chan->wait_frames)
        chan->wait_frames--;

    return 1;
}

void clock_channel(channel_t *chan, uint8_t sample)
{
    uint8_t ca_full = (chan->ca_phase >> 31) & 0x1;
    uint8_t last_ca_full = ca_full;
    if (chan->ca_en)
    {
        chan->ca_phase += chan->ca_rate;
    }
    chan->lo_phase += chan->lo_rate;
    ca_full = (chan->ca_phase >> 31) & 0x1;
    uint8_t ca_half = (chan->ca_phase >> 30) & 0x1;

    if (chan->phase_offset)
    {
        chan->phase_offset--;
    }
    else
    {
        chan->ca_en = 1;
    }

    if (ca_full & !last_ca_full)
    {
        clock_ca(&chan->ca);
        chan->ca_e = get_ca(&chan->ca);
    }

    if (ca_full && !ca_half)
    {
        chan->ca_l = chan->ca_p;
    }
    else if (!ca_full && !ca_half)
    {
        chan->ca_p = chan->ca_e;
    }

    do_sample(chan, sample);

    if (is_ca_epoch(&chan->ca) && !chan->tracked_this_epoch)
    {
        // Estimate CN0
        if(chan->prompt_hist_len < 1000) {
            chan->ip_hist[chan->prompt_hist_len] = chan->ip;
            chan->qp_hist[chan->prompt_hist_len] = chan->qp;
            chan->prompt_hist_len++;
        } else {
            memmove(chan->ip_hist, chan->ip_hist + 1, 999 * sizeof(int64_t));
            memmove(chan->qp_hist, chan->qp_hist + 1, 999 * sizeof(int64_t));
            chan->ip_hist[999] = chan->ip;
            chan->qp_hist[999] = chan->qp;
        }
        chan->cn0 = cn0_svn_estimator(chan->ip_hist, chan->qp_hist, chan->prompt_hist_len, 0.001);    
        if(!isnan(chan->cn0)) {
            logging_log(LOG_EVENT_TYPE_POWER, (void *)&chan->cn0, chan->sv + 1, 0);    
        }

        int64_t power_early = chan->ie * chan->ie + chan->qe * chan->qe;
        int64_t power_late = chan->il * chan->il + chan->ql * chan->ql;
        int64_t power_prompt = chan->ip * chan->ip + chan->qp * chan->qp;

        int64_t code_phase_err = power_early - power_late;
        int64_t carrier_phase_err = chan->ip * chan->qp;

        if(power_prompt < 1000000) {
            code_phase_err <<= 1;
            carrier_phase_err <<= 1;
        }

        chan->ca_freq_integrator += code_phase_err << 15;
        int64_t new_ca_rate = chan->ca_freq_integrator + (code_phase_err << 21);
        chan->ca_rate = new_ca_rate >> 32;

        chan->lo_freq_integrator += carrier_phase_err << 16;
        int64_t new_lo_rate = chan->lo_freq_integrator + (carrier_phase_err << 24);
        chan->lo_rate = new_lo_rate >> 32;

        // printf("carrier doppler from code lock: %lld\n",  (ca_freq_integrator >> 32));
        // printf("carrier doppler from code lock:    %20f\n",  (((ca_freq_integrator/4294967296.0)*fs/(4294967296.0))-1023000.0)*1575.42e6/1023000.0);
        // printf("carrier doppler from carrier lock: %20f\n",  ((lo_freq_integrator/4294967296.0)*fs/(4294967296.0))-fc);
        // printf("carrier doppler from carrier lock: %llu\n", lo_freq_integrator)

        double log_data[6];
        log_data[0] = code_phase_err;
        log_data[1] = chan->ca_freq_integrator;
        log_data[2] = chan->ca_rate;
        log_data[3] = carrier_phase_err;
        log_data[4] = chan->lo_freq_integrator;
        log_data[5] = chan->lo_rate;
        logging_log(LOG_EVENT_TYPE_FILTER, (void *)log_data, chan->sv + 1, 0);

        // Process nav bits
        process_ip_to_bit(chan);
        log_data[0] = chan->last_z_count * 6.0;
        log_data[1] = chan->nav_bit_count / 50.0;
        log_data[2] = chan->nav_ms / 1000.0;
        log_data[3] = chan->ca.chip / 1023000.0;
        log_data[4] = (chan->ca_phase + (1U << 31)) * pow(2, -32) / 1023000.0;
        log_data[5] = 0;
        logging_log(LOG_EVENT_TYPE_TIME, (void *)log_data, chan->sv + 1, 0);
        if (chan->nav_bit_count >= 300)
        {
            if (process_message(chan))
            {
                printf("SV: %d found message at %d ms!\n", chan->sv + 1, chan->total_ms);
                chan->bit_sync_locked = 1; // Lock in the bit synchronization if we got a valid message
                save_ephemeris_data(chan);
                memmove(chan->nav_buf, chan->nav_buf + 300, chan->nav_bit_count -= 300);
            }
            else
            {
                memmove(chan->nav_buf, chan->nav_buf + 1, chan->nav_bit_count--);
            }
        }

        // Report CN0 on the second
        if(chan->total_ms % 1000 == 0) {
            printf("GPS L1CA SV %d CN0=%.2f\n", chan->sv+1, chan->cn0);
        }

        chan->ie = 0;
        chan->qe = 0;
        chan->ip = 0;
        chan->qp = 0;
        chan->il = 0;
        chan->ql = 0;
        chan->tracked_this_epoch = 1;
    }
    else if (!is_ca_epoch(&chan->ca))
    {
        chan->tracked_this_epoch = 0;
    }
}

double get_tx_time(channel_t *chan)
{
    double t = (chan->last_z_count * 6.0) +
               (chan->nav_bit_count / 50.0) +
               ((chan->nav_ms + 1) / 1000.0) +
               (chan->ca.chip / 1023000.0) +
               ((chan->ca_phase + (1U << 31)) * pow(2, -32) / 1023000.0);

    return t;
}