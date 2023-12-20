#include <stdio.h>
#include <stdint.h>

#include "tools.h"
#include "gps.h"
#include "channel.h"

const int sv[NUM_CHANNELS] = {30, 31, 29, 1};
const double lo_dop[NUM_CHANNELS] = {-2162, -1900, -2200, 1400};
const double ca_shift[NUM_CHANNELS] = {857.8, 534.4, 39.9, 198.2};

uint8_t file_buf;
uint8_t file_buf_idx = 0;
FILE *file;

uint64_t time_limit = fs*60;

int main() {
    file = fopen("gps.samples.1bit.I.fs5456.if4092.bin", "rb");
    channel_t channels[NUM_CHANNELS];
    for(int i = 0; i < NUM_CHANNELS; i++) {
        init_channel(&channels[i], i, sv[i] - 1, lo_dop[i], ca_shift[i]);
    }

    while(time_limit--) {
        if(!file_buf_idx) {
            fread(&file_buf, 1, 1, file);
            if(feof(file)) {
                fclose(file);
                break;
            }
            file_buf_idx++;
        } else if (file_buf_idx == 7) {
            file_buf_idx = 0;
        } else {
            file_buf_idx++;
        }
        uint8_t sample = (file_buf >> file_buf_idx) & 0x1;
        for(int i = 0; i < NUM_CHANNELS; i++) {
            clock_channel(&channels[i], sample);
        }
    }

    if(file) {
        fclose(file);
    }

    // Plotting
    /*
    FILE *gnuplot_file = fopen("temp.dat", "w");

    FILE *gnuplot = _popen("gnuplot -persist", "w");
    
    for (int i = 0; i < ip_save_idx; i++)
    {
        fprintf(gnuplot_file, "%g %g\n", (double) ip_save[i], (double) qp_save[i]);
    }
    fclose(gnuplot_file);
    
    fprintf(gnuplot, "set xrange [-2000:2000]\n");
    fprintf(gnuplot, "set yrange [-2000:2000]\n");
    fprintf(gnuplot, "plot 'temp.dat' with points\n");

    _pclose(gnuplot);
    */
    return 0;
}
