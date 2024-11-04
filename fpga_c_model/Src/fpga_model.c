#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tools.h"
#include "gps.h"
#include "e1b_channel.h"
#include "solve.h"
#include "math.h"

int chan_count = 0;
int e1b_chan_count = 0;

const int sv[] = {
    24,
    5,
    4,
    21,
};
const double lo_dop[] = {
    -246.0,
    1400.0,
    -3000.0,
    -2400.0,
};
const double ca_shift[] = {
    2837.5,
    969.4,
    367.0,
    817.4,
};

uint8_t file_buf;
uint8_t file_buf_idx = 0;
FILE *file;

uint64_t clock = 0;
uint64_t time_limit = fs * 60;

int main()
{
    printf("lon,lat,alt\n");

    file = fopen("../gnss-20170427-L1.1bit.I.bin", "rb");
    e1b_channel_t e1b_channels[1];
    channel_t channels[3];
    
    e1b_init_channel(&e1b_channels[0], 0, sv[0] - 1, lo_dop[0], ca_shift[0]);
    //e1b_init_channel(&e1b_channels[1], 1, sv[1] - 1, lo_dop[1], ca_shift[1]);
    e1b_chan_count+=1;
    
    init_channel(&channels[0], 1, sv[1] - 1, lo_dop[1], ca_shift[1]);
    init_channel(&channels[1], 2, sv[2] - 1, lo_dop[2], ca_shift[2]);
    init_channel(&channels[2], 3, sv[3] - 1, lo_dop[3], ca_shift[3]);
    chan_count+=3;

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
            //for (int i = 0; i < NUM_CHANNELS; i++)
            //{
            //    if (channels_in_soln[i]->wait_frames)
            //    {
            //        ready = 0;
            //        break;
            //    }
            //}
            
            if (ready && solve(channels_in_soln, 3, e1b_channels_in_soln, 1, &x, &y, &z, &t_bias))
            {
                // printf("x: %g, y: %g, z: %g, t_bias: %g\n", x, y, z, t_bias);
                to_coords(x, y, z, &lat, &lon, &alt);
                //if (avg_count < AVG_COUNT)
                //{
                //    avg_lat[avg_count] = lat;
                //    avg_lon[avg_count] = lon;
                //    avg_alt[avg_count] = alt;
                //    avg_count++;
                //}
                //else
                //{
                //    // Add new point
                //    memmove(avg_lat + 1, avg_lat, (AVG_COUNT - 1) * sizeof(double));
                //    avg_lat[0] = lat;
                //    avg_lon[0] = lon;
                //    avg_alt[0] = alt;
                //
                //    // Calculate average
                //    lat = 0;
                //    lon = 0;
                //    alt = 0;
                //    for (int i = 0; i < AVG_COUNT; i++)
                //    {
                //        lat += avg_lat[i];
                //        lon += avg_lon[i];
                //        alt += avg_alt[i];
                //    }
                //    lat /= AVG_COUNT;
                //    lon /= AVG_COUNT;
                //    alt /= AVG_COUNT;
                //    printf("%.8f,%.8f,%.8f\n", lon, lat, alt);
                //}
                
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
