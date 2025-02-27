#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tools.h"
#include "gps.h"
#include "e1b_channel.h"
#include "solve.h"
#include "math.h"
#include "logging.h"
#include "signal.h"
#include "stdlib.h"

int chan_count = 0;
int e1b_chan_count = 0;

const int sv[] = {
    24,
    // 26,
    5,
    4,
    21,
};
const double lo_dop[] = {
    -246.0,
    // 1000.0,
    1400.0,
    -3000.0,
    -2400.0,
};
const double ca_shift[] = {
    2837.5,
    // 1001.1 - 0.5,
    969.4,
    367.0,
    817.4,
};

int8_t file_buf;
int8_t dummy;
FILE *file;

uint64_t clock = 0;
uint64_t time_limit = fs * 60;

void sigint_handler(int sig)
{
    printf("Closing files on SIGINT\n");
    if (file)
    {
        fclose(file);
    }
    logging_close_logs();
    exit(0);
}

int main()
{
    logging_init();
    signal(SIGINT, sigint_handler);

    printf("lon,lat,alt\n");

    file = fopen("../gnss-20170427-L1.bin", "rb");
    e1b_channel_t e1b_channels[1];
    channel_t channels[3];

    e1b_init_channel(&e1b_channels[0], 0, sv[0] - 1, lo_dop[0], ca_shift[0]);
    logging_create_log(LOG_EVENT_TYPE_IQ, sv[0], 1);
    logging_create_log(LOG_EVENT_TYPE_TIME, sv[0], 1);
    logging_create_log(LOG_EVENT_TYPE_FILTER, sv[0], 1);
    // e1b_init_channel(&e1b_channels[1], 1, sv[1] - 1, lo_dop[1], ca_shift[1]);
    e1b_chan_count += 1;

    init_channel(&channels[0], 1, sv[1] - 1, lo_dop[1], ca_shift[1]);
    init_channel(&channels[1], 2, sv[2] - 1, lo_dop[2], ca_shift[2]);
    init_channel(&channels[2], 3, sv[3] - 1, lo_dop[3], ca_shift[3]);
    logging_create_log(LOG_EVENT_TYPE_IQ, sv[1], 0);
    logging_create_log(LOG_EVENT_TYPE_IQ, sv[2], 0);
    logging_create_log(LOG_EVENT_TYPE_IQ, sv[3], 0);
    logging_create_log(LOG_EVENT_TYPE_TIME, sv[1], 0);
    logging_create_log(LOG_EVENT_TYPE_TIME, sv[2], 0);
    logging_create_log(LOG_EVENT_TYPE_TIME, sv[3], 0);
    logging_create_log(LOG_EVENT_TYPE_FILTER, sv[1], 0);
    logging_create_log(LOG_EVENT_TYPE_FILTER, sv[2], 0);
    logging_create_log(LOG_EVENT_TYPE_FILTER, sv[3], 0);
    chan_count += 3;

    while (time_limit--)
    {
        fread(&file_buf, 1, 1, file);
        fread(&dummy, 1, 1, file); // Skip Q
        if (feof(file))
        {
            fclose(file);
            break;
        }
        // 3-bit 2's complement (-4 to 3)
        int8_t sample = file_buf; //(file_buf >> 5);
        for (int i = 0; i < e1b_chan_count; i++)
        {
            e1b_clock_channel(&e1b_channels[i], sample);
        }
        for (int i = 0; i < chan_count; i++)
        {
            clock_channel(&channels[i], sample);
        }

        if (clock % (lrint(fs) / 1) == 0)
        {
            double x, y, z, t_bias;
            double lat, lon, alt;
            e1b_channel_t *e1b_channels_in_soln[] = {
                &e1b_channels[0],
                //&e1b_channels[1],
            };
            channel_t *channels_in_soln[] = {
                &channels[0],
                &channels[1],
                &channels[2],
            };
            uint8_t ready = 1;
            // for (int i = 0; i < NUM_CHANNELS; i++)
            //{
            //     if (channels_in_soln[i]->wait_frames)
            //     {
            //         ready = 0;
            //         break;
            //     }
            // }

            if (ready && solve(channels_in_soln, 3, e1b_channels_in_soln, 1, &x, &y, &z, &t_bias))
            {
                // printf("x: %g, y: %g, z: %g, t_bias: %g\n", x, y, z, t_bias);
                to_coords(x, y, z, &lat, &lon, &alt);
                printf("lat,lon,alt: %.8f,%.8f,%.8f\n", lat, lon, alt);
                // if (avg_count < AVG_COUNT)
                //{
                //     avg_lat[avg_count] = lat;
                //     avg_lon[avg_count] = lon;
                //     avg_alt[avg_count] = alt;
                //     avg_count++;
                // }
                // else
                //{
                //     // Add new point
                //     memmove(avg_lat + 1, avg_lat, (AVG_COUNT - 1) * sizeof(double));
                //     avg_lat[0] = lat;
                //     avg_lon[0] = lon;
                //     avg_alt[0] = alt;
                //
                //     // Calculate average
                //     lat = 0;
                //     lon = 0;
                //     alt = 0;
                //     for (int i = 0; i < AVG_COUNT; i++)
                //     {
                //         lat += avg_lat[i];
                //         lon += avg_lon[i];
                //         alt += avg_alt[i];
                //     }
                //     lat /= AVG_COUNT;
                //     lon /= AVG_COUNT;
                //     alt /= AVG_COUNT;
                //     printf("%.8f,%.8f,%.8f\n", lon, lat, alt);
                // }
            }
        }

        clock++;
    }

    printf("Closing files\n");

    if (file)
    {
        fclose(file);
    }

    logging_close_logs();

    return 0;
}
