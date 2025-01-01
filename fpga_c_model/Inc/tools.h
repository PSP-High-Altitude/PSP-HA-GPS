#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdint.h>
#include "gps.h"

#define HALF_PI 1.5707963267949
#define TWO_PI 6.2831853071796

#define PHASE_UNWRAP(x) ((x >= HALF_PI) ? (x - PI) : ((x <= -HALF_PI) ? (x + PI) : x))

typedef struct {
    uint8_t* buf;
    uint16_t write_idx;
    uint16_t read_idx;
    uint16_t size;
    uint16_t capacity;
} fifo;

// Modified from GNSS-SDR
typedef struct {
    double pll_w;
    double pll_w0p3;
    double pll_w0f2;
    double pll_x;
    double pll_a2;
    double pll_w0f;
    double pll_a3;
    double pll_w0p2;
    double pll_b3;
    double pll_w0p;
    int order;
} fll_pll_t;

void init_fifo(fifo* f, uint16_t capacity);

void free_fifo(fifo* f);

int fifo_write(fifo* f, uint8_t data, uint16_t size);

int fifo_read(fifo* f, uint8_t* data, uint16_t size);

int fifo_peek(fifo* f, uint8_t* data, uint16_t size);

uint16_t fifo_size(fifo* f);

void bytes_to_number(void *dest, uint8_t *buf, int dest_size, int src_size, uint8_t sign);

// Modified from GNSS-SDR
double cn0_svn_estimator(const int64_t* ip, const int64_t* qp, int length, double coh_integration_time_s);

// Modified from GNSS-SDR
double cn0_m2m4_estimator(const int64_t* ip, const int64_t* qp, int length, double coh_integration_time_s);

// Modified from GNSS-SDR
double fll_pll_filter(fll_pll_t *pll, double fll_discriminator, double pll_discriminator, double correlation_time_s);

// Modified from GNSS-SDR
void fll_pll_set_params(fll_pll_t *pll, double fll_bw_hz, double pll_bw_hz, int order);

#endif