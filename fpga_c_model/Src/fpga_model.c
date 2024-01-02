#include <stdio.h>
#include <stdint.h>

#include "tools.h"
#include "gps.h"
#include "channel.h"
#include "solve.h"
#include "math.h"

const int sv[NUM_CHANNELS] = {30, 31, 29, 1};
const double lo_dop[NUM_CHANNELS] = {-2162, -1900, -2200, 1350};
const double ca_shift[NUM_CHANNELS] = {857.8, 534.4, 39.9, 198.2};

uint8_t file_buf;
uint8_t file_buf_idx = 0;
FILE *file;

uint64_t clock = 0;
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

        if(clock % (lrint(fs)*1) == 0) {
            double x, y, z, t_bias;
            double lat, lon, alt;
            channel_t *channels_in_soln[] = {&channels[0], &channels[1], &channels[2], &channels[3]};
            uint8_t ready = 1;
            for(int i = 0; i < 4; i++) {
                if(channels_in_soln[i]->wait_frames) {
                    ready = 0;
                    break;
                }
            }
            if(ready && solve(channels_in_soln, 4, &x, &y, &z, &t_bias)) {
                //printf("x: %g, y: %g, z: %g, t_bias: %g\n", x, y, z, t_bias);
                to_coords(x, y, z, &lat, &lon, &alt);
                printf("lat: %g, lon: %g, alt: %g\n", lat, lon, alt);
            }
        }

        clock++;
    }

    if(file) {
        fclose(file);
    }

    return 0;
}
