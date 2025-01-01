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

// GPS L1 C/A Channels
channel_t channels_gps_l1ca[24];
int count_channels_gps_l1ca = 0;
#define CREATE_GPS_L1CA_CHANNEL(sv, lo_dop, ca_shift) \
    init_channel(&channels_gps_l1ca[count_channels_gps_l1ca], count_channels_gps_l1ca, sv-1, lo_dop, ca_shift); \
    logging_create_log(LOG_EVENT_TYPE_IQ, sv, 0); \
    logging_create_log(LOG_EVENT_TYPE_TIME, sv, 0); \
    logging_create_log(LOG_EVENT_TYPE_FILTER, sv, 0); \
    logging_create_log(LOG_EVENT_TYPE_POWER, sv, 0); \
    count_channels_gps_l1ca++;

e1b_channel_t channels_galileo_e1b[24];
int count_channels_galileo_e1b = 0;
#define CREATE_GALILEO_E1B_CHANNEL(sv, lo_dop, ca_shift) \
    e1b_init_channel(&channels_galileo_e1b[count_channels_galileo_e1b], count_channels_galileo_e1b, sv-1, lo_dop, ca_shift); \
    logging_create_log(LOG_EVENT_TYPE_IQ, sv, 1); \
    logging_create_log(LOG_EVENT_TYPE_TIME, sv, 1); \
    logging_create_log(LOG_EVENT_TYPE_FILTER, sv, 1); \
    logging_create_log(LOG_EVENT_TYPE_POWER, sv, 1); \
    count_channels_galileo_e1b++;

uint8_t file_buf;
uint8_t file_buf_idx = 0;
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

    file = fopen("../gnss-20170427-L1.1bit.I.bin", "rb");

    CREATE_GALILEO_E1B_CHANNEL(24, -285.0, 2837.5);
    //CREATE_GALILEO_E1B_CHANNEL(14, -3200.0, 3770.1);
    //CREATE_GALILEO_E1B_CHANNEL(26, 1000.0, 1000.6);
    //CREATE_GPS_L1CA_CHANNEL(5, 1400.0, 969.4);
    //CREATE_GPS_L1CA_CHANNEL(2, 1600.0, 7.0);
    //CREATE_GPS_L1CA_CHANNEL(21, -2400.0, 817.4);
    //CREATE_GPS_L1CA_CHANNEL(26, -3400, 446.3);

    logging_create_log(LOG_EVENT_TYPE_PVT, 0, 0);

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
        for (int i = 0; i < count_channels_galileo_e1b; i++)
        {
            e1b_clock_channel(&channels_galileo_e1b[i], sample);
        }
        for (int i = 0; i < count_channels_gps_l1ca; i++)
        {
            clock_channel(&channels_gps_l1ca[i], sample);
        }

        if (clock % (lrint(fs) / 1) == 0)
        {
            double x, y, z, t_bias;
            double lat, lon, alt;
            uint8_t ready = 1;
            // for (int i = 0; i < NUM_CHANNELS; i++)
            //{
            //     if (channels_in_soln[i]->wait_frames)
            //     {
            //         ready = 0;
            //         break;
            //     }
            // }

            if (ready && solve(channels_gps_l1ca, count_channels_gps_l1ca, channels_galileo_e1b, count_channels_galileo_e1b, &x, &y, &z, &t_bias))
            {
                // printf("x: %g, y: %g, z: %g, t_bias: %g\n", x, y, z, t_bias);
                to_coords(x, y, z, &lat, &lon, &alt);
                double solve_log[7] = {x, y, z, t_bias, lat, lon, alt};
                logging_log(LOG_EVENT_TYPE_PVT, (void *)&solve_log, 0, 0);
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
