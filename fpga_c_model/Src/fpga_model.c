#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tools.h"
#include "gps.h"
#include "channel.h"
#include "solve.h"
#include "math.h"

const int sv[NUM_CHANNELS] = {
    30,
    31,
    29,
    1,
    21,
    25,
};
const double lo_dop[NUM_CHANNELS] = {
    -2162,
    -1900,
    -2200,
    1350,
    2050,
    -3650,
};
const double ca_shift[NUM_CHANNELS] = {
    857.8,
    534.4,
    39.9,
    198.2,
    962.4,
    435.9,
};

uint8_t file_buf;
uint8_t file_buf_idx = 0;
FILE *file;

uint64_t clock = 0;
uint64_t time_limit = fs * 60;

int main()
{
    printf("lon,lat,alt\n");

    file = fopen("gps.samples.1bit.I.fs5456.if4092.bin", "rb");
    channel_t channels[NUM_CHANNELS];
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        init_channel(&channels[i], i, sv[i] - 1, lo_dop[i], ca_shift[i]);
    }

    while (time_limit--)
    {
        if (!file_buf_idx)
        {
            fread(&file_buf, 1, 1, file);
            if (feof(file))
            {
                fclose(file);
                break;
            }
            file_buf_idx++;
        }
        else if (file_buf_idx == 7)
        {
            file_buf_idx = 0;
        }
        else
        {
            file_buf_idx++;
        }
        uint8_t sample = (file_buf >> file_buf_idx) & 0x1;
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            clock_channel(&channels[i], sample);
        }

        if (clock % (lrint(fs) / 10) == 0)
        {
            double x, y, z, t_bias;
            double lat, lon, alt;
            channel_t *channels_in_soln[] = {
                &channels[0],
                &channels[1],
                &channels[2],
                &channels[3],
                &channels[4],
                &channels[5],
            };
            uint8_t ready = 1;
            for (int i = 0; i < NUM_CHANNELS; i++)
            {
                if (channels_in_soln[i]->wait_frames)
                {
                    ready = 0;
                    break;
                }
            }
            if (ready && solve(channels_in_soln, NUM_CHANNELS, &x, &y, &z, &t_bias))
            {
                // printf("x: %g, y: %g, z: %g, t_bias: %g\n", x, y, z, t_bias);
                to_coords(x, y, z, &lat, &lon, &alt);
                /*if (avg_count < AVG_COUNT)
                {
                    avg_lat[avg_count] = lat;
                    avg_lon[avg_count] = lon;
                    avg_alt[avg_count] = alt;
                    avg_count++;
                }
                else
                {
                    // Add new point
                    memmove(avg_lat + 1, avg_lat, (AVG_COUNT - 1) * sizeof(double));
                    avg_lat[0] = lat;
                    avg_lon[0] = lon;
                    avg_alt[0] = alt;

                    // Calculate average
                    lat = 0;
                    lon = 0;
                    alt = 0;
                    for (int i = 0; i < AVG_COUNT; i++)
                    {
                        lat += avg_lat[i];
                        lon += avg_lon[i];
                        alt += avg_alt[i];
                    }
                    lat /= AVG_COUNT;
                    lon /= AVG_COUNT;
                    alt /= AVG_COUNT;
                    printf("%.8f,%.8f,%.8f\n", lon, lat, alt);
                }
                */
                printf("%.8f,%.8f,%.8f\n", lon, lat, alt);
            }
        }

        clock++;
    }

    if (file)
    {
        fclose(file);
    }

    return 0;
}
