#ifndef LOGGING_H
#define LOGGING_H

#include "stdio.h"

typedef enum
{
    LOG_EVENT_TYPE_IQ,     // Param1: SV, Param2: GNSS (0 - GPS, 1 - Galileo)
    LOG_EVENT_TYPE_TIME,   // Param1: SV, Param2: GNSS (0 - GPS, 1 - Galileo)
    LOG_EVENT_TYPE_FILTER, // Param1: SV, Param2: GNSS (0 - GPS, 1 - Galileo)
    LOG_EVENT_TYPE_PVT,
    LOG_EVENT_TYPE_POWER,  // Param1: SV, Param2: GNSS (0 - GPS, 1 - Galileo)
} log_event_type_t;

typedef struct
{
    FILE *file;
    char filename[256];
    log_event_type_t event_type;
    int param1; // Used for checking log conditions
    int param2; // Used for checking log conditions
} log_t;

void logging_init();
void logging_create_log(log_event_type_t event_type, int param1, int param2);
void logging_log(log_event_type_t event_type, void *data, int param1, int param2);
void logging_close_logs();

#endif // LOGGING_H
