/**********************************************************
 * SearchFFT.cpp - Extract GPS signal alignment details
 * from a raw GPS sample bitstream
 *
 * Requires data file : gps.samples.1bit.I.fs5456.if4092.bin
 *
 * Requires Library FFTW - see http://www.fftw.org/
 *
 * Original author Andrew Holme
 * Copyright (C) 2013 Andrew Holme
 * http://www.holmea.demon.co.uk/GPS/Main.htm
 *
 **********************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fftw3.h"

#ifdef USE_FFTW_SPREC
#define fftw_complex fftwf_complex
#define fftw_malloc fftwf_malloc
#define fftw_plan fftwf_plan
#define fftw_plan_dft_1d fftwf_plan_dft_1d
#define fftw_execute fftwf_execute
#define fftw_destroy_plan fftwf_destroy_plan
#define fftw_free fftwf_free
#endif

/******************************************
 * Details of the space vehicles
 ******************************************/
int SVs[] = {
    // PRN, SVID, G2 shift
    131,
    0,
    0551,
    133,
    1,
    01731,
    135,
    2,
    01216,
};

/**********************************************************
 * Class to generate the C/A gold codes that are transmitted
 * by each Satallite. Combines the output of the G1 and G2
 * LFSRs to produce the 1023 symbol sequence
 **********************************************************/
struct CACODE
{ // GPS coarse acquisition (C/A) gold code generator.  Sequence
  // length = 1023.
    char g1[11], g2[11];

    CACODE(int g2i)
    {
        memset(g1 + 1, 1, 10);
        for (int i = 0; i < 10; i++)
        {
            g2[i + 1] = (g2i >> i) & 1;
        }
    }

    int Chip() { return g1[10] ^ g2[10]; }

    void Clock()
    {
        g1[0] = g1[3] ^ g1[10];
        g2[0] = g2[2] ^ g2[3] ^ g2[6] ^ g2[8] ^ g2[9] ^ g2[10];
        memmove(g1 + 1, g1, 10);
        memmove(g2 + 1, g2, 10);
    }
};

int main(int argc, char *argv[])
{
    /****************************
     * Detals of the input file
     ****************************/
    const int fc = 9334875; // or 1364000
    const int fs = 69984000;
    const char *in = "gnss-20170427-L1.1bit.I.bin";
    const int ms = 20; // Length of data to process (milliseconds)
    const int Start = 0;

    /**************************************
     * Derived values
     **************************************/
    const int Len = ms * fs / 1000;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    fftw_complex *code = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Len);
    fftw_complex *data = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Len);
    fftw_complex *prod = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Len);

    fftw_plan p;

    /***************************************
     * Read in the file file . Data is
     * packed LSB first (sample 0 in bit 0)
     ****************************************/

    static char sample_data[Len + 7];
    int i, j, ch;

    FILE *fp = fopen(in, "rb");
    if (!fp)
    {
        perror(in);
        return 0;
    }
    fseek(fp, Start, SEEK_SET);

    for (i = 0; i < Len; i += 8)
    {
        ch = fgetc(fp);
        for (j = 0; j < 8; j++)
        {
            sample_data[i + j] = ch & 1;
            ch >>= 1;
        }
    }

    fclose(fp);

    printf("PRN Nav Doppler   Phase   MaxSNR\n");
    /********************************************
     *  Process all space vehicles in turn
     ********************************************/
    for (int sv = 0; sv < sizeof(SVs) / sizeof(int);)
    {

        int PRN = SVs[sv++];
        int Navstar = SVs[sv++];
        int G2i = SVs[sv++];

        if (!PRN)
            break;

        /*************************************************
         * Generate the C/A code for the window of the
         * data (at the sample rate used by the data stream
         *************************************************/

        CACODE ca(G2i);

        double ca_freq = 1023000, ca_phase = 0, ca_rate = ca_freq / fs;

        for (i = 0; i < Len; i++)
        {
            code[i][0] = ca.Chip() ? -1 : 1;
            code[i][1] = 0;

            ca_phase += ca_rate;

            if (ca_phase >= 1)
            {
                ca_phase -= 1;
                ca.Clock();
            }
        }

        /******************************************
         * Now run the FFT on the C/A code stream  *
         ******************************************/
        p = fftw_plan_dft_1d(Len, code, code, FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_execute(p);
        fftw_destroy_plan(p);

        if (PRN == 31)
        {
            FILE *test_file = fopen("test_prn", "w");
            for (i = 0; i < Len; i++)
            {
                fprintf(test_file, "%f\n", code[i][0]);
            }
            fclose(test_file);
        }

        /*************************************************
         * Now generate the same for the sample data, but
         * removing the Local Oscillator from the samples.
         *************************************************/
        const int lo_sin[] = {1, 1, 0, 0};
        const int lo_cos[] = {1, 0, 0, 1};

        double lo_freq = fc, lo_phase = 0, lo_rate = lo_freq / fs * 4;

        for (i = 0; i < Len; i++)
        {

            data[i][1] = (sample_data[i] ^ lo_cos[int(lo_phase)]) ? -1 : 1;
            data[i][0] = (sample_data[i] ^ lo_sin[int(lo_phase)]) ? -1 : 1;
            // if(PRN == 30) printf("sample_data[%d] = %d\n", i+1000, sample_data[i+1000]);
            // printf("%i %i\n",lo_sin[int(lo_phase)],lo_cos[int(lo_phase)]);
            lo_phase += lo_rate;
            if (lo_phase >= 4)
                lo_phase -= 4;
        }

        p = fftw_plan_dft_1d(Len, data, data, FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_execute(p);
        fftw_destroy_plan(p);

        /***********************************************
         * Generate the execution plan for the Inverse
         * FFT (which will be reused multiple times
         ***********************************************/

        p = fftw_plan_dft_1d(Len, prod, prod, FFTW_BACKWARD, FFTW_ESTIMATE);

        double max_snr = 0;
        int max_snr_dop, max_snr_i;

        /************************************************
         * Test at different doppler shifts (+/- 5kHz)
         ************************************************/
        for (int dop = -5000.0 * Len / fs; dop <= 5000.0 * Len / fs; dop++)
        {
            double max_pwr = 0, tot_pwr = 0;
            int max_pwr_i;

            /*********************************************
             * Complex muiltiply the C/A code spectrum
             * with the spectrum that came from the data
             ********************************************/
            for (i = 0; i < Len; i++)
            {
                int j = (i - dop + Len) % Len;
                prod[i][0] = data[i][0] * code[j][0] + data[i][1] * code[j][1];
                prod[i][1] = data[i][0] * code[j][1] - data[i][1] * code[j][0];
            }

            /**********************************
             * Run the inverse FFT
             **********************************/
            fftw_execute(p);

            /*********************************
             * Look through the result to find
             * the point of max absolute power
             *********************************/
            for (i = 0; i < fs / 1000; i++)
            {
                double pwr = prod[i][0] * prod[i][0] + prod[i][1] * prod[i][1];
                if (pwr > max_pwr)
                    max_pwr = pwr, max_pwr_i = i;
                tot_pwr += pwr;
            }
            /*****************************************
             * Normalise the units and find the maximum
             *****************************************/
            double ave_pwr = tot_pwr / i;
            double snr = max_pwr / ave_pwr;
            if (snr > max_snr)
                max_snr = snr, max_snr_dop = dop, max_snr_i = max_pwr_i;
        }
        fftw_destroy_plan(p);

        /*****************************************
         * Display the result
         *****************************************/
        printf("%-2d %4d %7.0f %8.1f %7.1f    ",
               PRN,
               Navstar,
               max_snr_dop * double(fs) / Len,
               (max_snr_i * 1023.0) / (fs / 1000),
               max_snr);

        for (i = int(max_snr) / 10; i--;)
            putchar('*');
        putchar('\n');
    }

    fftw_free(code);
    fftw_free(data);
    fftw_free(prod);
    return 0;
}