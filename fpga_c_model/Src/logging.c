#include "logging.h"

log_t logs[100];
int num_logs = 0;

void logging_init()
{
    num_logs = 0;
}

void logging_create_log(log_event_type_t event_type, int param1, int param2)
{
    switch (event_type)
    {
    case LOG_EVENT_TYPE_IQ:
        snprintf(logs[num_logs].filename, 256, "log/iq-%s-%d.csv", param2 == 0 ? "gps" : "galileo", param1);
        break;
    case LOG_EVENT_TYPE_TIME:
        snprintf(logs[num_logs].filename, 256, "log/time-%s-%d.csv", param2 == 0 ? "gps" : "galileo", param1);
        break;
    case LOG_EVENT_TYPE_FILTER:
        snprintf(logs[num_logs].filename, 256, "log/filter-%s-%d.csv", param2 == 0 ? "gps" : "galileo", param1);
        break;
    case LOG_EVENT_TYPE_PVT:
        snprintf(logs[num_logs].filename, 256, "log/pvt.csv");
        break;
    }

    logs[num_logs].file = fopen(logs[num_logs].filename, "w");
    if (logs[num_logs].file == NULL)
    {
        printf("Error opening file %s\n", logs[num_logs].filename);
        return;
    }
    logs[num_logs].event_type = event_type;
    logs[num_logs].param1 = param1;
    logs[num_logs].param2 = param2;
    num_logs++;
}

void logging_log(log_event_type_t event_type, void *data, int param1, int param2)
{
    for (int i = 0; i < num_logs; i++)
    {
        if (logs[i].event_type == event_type)
        {
            switch (event_type)
            {
            case LOG_EVENT_TYPE_IQ:
                if (param1 == logs[i].param1 && param2 == logs[i].param2)
                {
                    fprintf(logs[i].file, "%d,%d\n", ((int *)data)[0], ((int *)data)[1]);
                }
                break;
            case LOG_EVENT_TYPE_TIME:
                if (param1 == logs[i].param1 && param2 == logs[i].param2)
                {
                    fprintf(logs[i].file, "%g,%g,%g,%g,%g,%.15f\n", ((double *)data)[0], ((double *)data)[1], ((double *)data)[2], ((double *)data)[3], ((double *)data)[4], ((double *)data)[5]);
                }
                break;
            case LOG_EVENT_TYPE_FILTER:
                if (param1 == logs[i].param1 && param2 == logs[i].param2)
                {
                    fprintf(logs[i].file, "%.0f,%.0f,%.0f,%.0f,%.0f,%.0f\n", ((double *)data)[0], ((double *)data)[1], ((double *)data)[2], ((double *)data)[3], ((double *)data)[4], ((double *)data)[5]);
                }
                break;
            case LOG_EVENT_TYPE_PVT:
                // x, y, z, t_bias, lat, lon, alt
                fprintf(logs[i].file, "%.15f,%.15f,%.15f,%.15f,%.15f,%.15f,%.15f\n", ((double *)data)[0], ((double *)data)[1], ((double *)data)[2], ((double *)data)[3], ((double *)data)[4], ((double *)data)[5], ((double *)data)[6]);
                break;
            }
        }
    }
}

void logging_close_logs()
{
    for (int i = 0; i < num_logs; i++)
    {
        fclose(logs[i].file);
    }
}