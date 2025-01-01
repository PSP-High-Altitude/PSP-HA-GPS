#include "tools.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

void init_fifo(fifo* f, uint16_t capacity) {
    f->capacity = capacity;
    f->size = 0;
    f->write_idx = 0;
    f->read_idx = 0;
    f->buf = (uint8_t*)malloc(capacity);
}

void free_fifo(fifo* f) {
    free(f->buf);
}

int fifo_write(fifo* f, uint8_t data, uint16_t size) {
    if (f->size + size > f->capacity) {
        return -1;
    }
    for(int i = 0; i < size; i++) {
        f->buf[f->write_idx] = data;
        f->write_idx = (f->write_idx + 1) % f->capacity;
    }
    f->size += size;
    return 0;
}

int fifo_read(fifo* f, uint8_t* data, uint16_t size) {
    if (f->size < size) {
        return -1;
    }
    for(int i = 0; i < size; i++) {
        data[i] = f->buf[f->read_idx];
        f->read_idx = (f->read_idx + 1) % f->capacity;
    }
    f->size -= size;
    return 0;
}

int fifo_peek(fifo* f, uint8_t* data, uint16_t size) {
    if (f->size < size) {
        return -1;
    }
    for(int i = 0; i < size; i++) {
        data[i] = f->buf[(f->read_idx + i) % f->capacity];
    }
    return 0;
}

void bytes_to_number(void *dest, uint8_t *buf, int dest_size, int src_size, uint8_t sign)
{
    if ((dest_size < src_size) || (dest_size % 8 != 0))
    {
        return;
    }
    uint64_t result = 0;

    // Pack bytes
    for (int i = 0; i < src_size; i++)
    {
        result |= (buf[src_size - i - 1] << i);
    }

    // Preserve sign if signed
    if (sign)
    {
        uint8_t sign_bit = (result >> (src_size - 1)) & 0x1;
        result |= sign_bit ? (0xFFFFFFFFFFFFFFFF << src_size) : 0;
    }

    // Copy result to dest
    memcpy(dest, &result, dest_size / 8);
}

double cn0_svn_estimator(const int64_t* ip, const int64_t* qp, int length, double coh_integration_time_s)
{
    double SNR = 0.0;
    double SNR_dB_Hz = 0.0;
    double Psig = 0.0;
    double Ptot = 0.0;
    for (int i = 0; i < length; i++)
        {
            Psig += abs(ip[i]);
            Ptot += ip[i] * ip[i] + qp[i] * qp[i];
        }
    Psig /= (double)(length);
    Psig = Psig * Psig;
    Ptot /= (double)(length);
    SNR = Psig / (Ptot - Psig);
    SNR_dB_Hz = 10.0 * log10(SNR) - 10.0 * log10(coh_integration_time_s);
    return SNR_dB_Hz;
}

double cn0_m2m4_estimator(const int64_t* ip, const int64_t* qp, int length, double coh_integration_time_s)
{
    double SNR_aux = 0.0;
    double SNR_dB_Hz = 0.0;
    double Psig = 0.0;
    double m_2 = 0.0;
    double m_4 = 0.0;
    double aux;
    const double n = length;
    for (int i = 0; i < length; i++)
        {
            Psig += abs(ip[i]);
            aux = qp[i] * qp[i] + ip[i] * ip[i];
            m_2 += aux;
            m_4 += (aux * aux);
        }
    Psig /= n;
    Psig = Psig * Psig;
    m_2 /= n;
    m_4 /= n;
    aux = sqrt(2.0 * m_2 * m_2 - m_4);
    if (isnan(aux))
        {
            SNR_aux = Psig / (m_2 - Psig);
        }
    else
        {
            SNR_aux = aux / (m_2 - aux);
        }
    SNR_dB_Hz = 10.0 * log10(SNR_aux) - 10.0 * log10(coh_integration_time_s);

    return SNR_dB_Hz;
}

double fll_pll_filter(fll_pll_t *pll, double fll_discriminator, double pll_discriminator, double correlation_time_s)
{
    double carrier_error_hz;
    if (pll->order == 3)
        {
            /*
             * 3rd order PLL with 2nd order FLL assist
             */
            pll->pll_w = pll->pll_w + correlation_time_s * (pll->pll_w0p3 * pll_discriminator + pll->pll_w0f2 * fll_discriminator);
            pll->pll_x = pll->pll_x + correlation_time_s * (0.5 * pll->pll_w + pll->pll_a2 * pll->pll_w0f * fll_discriminator + pll->pll_a3 * pll->pll_w0p2 * pll_discriminator);
            carrier_error_hz = 0.5 * pll->pll_x + pll->pll_b3 * pll->pll_w0p * pll_discriminator;
        }
    else if(pll->order == 2)
        {
            /*
             * 2nd order PLL with 1st order FLL assist
             */
            const double pll_w_new = pll->pll_w + pll_discriminator * pll->pll_w0p2 * correlation_time_s + fll_discriminator * pll->pll_w0f * correlation_time_s;
            carrier_error_hz = 0.5 * (pll_w_new + pll->pll_w) + pll->pll_a2 * pll->pll_w0p * pll_discriminator;
            pll->pll_w = pll_w_new;
        }
    else 
        {
            /*
             * 1st order PLL
             */
            carrier_error_hz = pll_discriminator * pll->pll_w0p;
        }

    return carrier_error_hz;
}

void fll_pll_set_params(fll_pll_t *pll, double fll_bw_hz, double pll_bw_hz, int order)
{
    /*
     * Filter design (Kaplan 2nd ed., Pag. 181 Fig. 181)
     */
    pll->order = order;
    if (pll->order == 3)
        {
            /*
             *  3rd order PLL with 2nd order FLL assist
             */
            pll->pll_b3 = 2.400;
            pll->pll_a3 = 1.100;
            pll->pll_a2 = 1.414;
            pll->pll_w0p = pll_bw_hz / 0.7845;
            pll->pll_w0p2 = pll->pll_w0p * pll->pll_w0p;
            pll->pll_w0p3 = pll->pll_w0p2 * pll->pll_w0p;

            pll->pll_w0f = fll_bw_hz / 0.53;
            pll->pll_w0f2 = pll->pll_w0f * pll->pll_w0f;
        }
    else if(pll->order == 2)
        {
            /*
             * 2nd order PLL with 1st order FLL assist
             */
            pll->pll_a2 = 1.414;
            pll->pll_w0p = pll_bw_hz / 0.53;
            pll->pll_w0p2 = pll->pll_w0p * pll->pll_w0p;
            pll->pll_w0f = fll_bw_hz / 0.25;
        }
    else 
        {
            /*
             * 1st order PLL
             */
            pll->pll_w0p = pll_bw_hz / 0.25;
        }
}