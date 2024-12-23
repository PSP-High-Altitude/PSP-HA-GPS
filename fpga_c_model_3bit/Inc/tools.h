#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint8_t* buf;
    uint16_t write_idx;
    uint16_t read_idx;
    uint16_t size;
    uint16_t capacity;
} fifo;

void init_fifo(fifo* f, uint16_t capacity);

void free_fifo(fifo* f);

int fifo_write(fifo* f, uint8_t data, uint16_t size);

int fifo_read(fifo* f, uint8_t* data, uint16_t size);

int fifo_peek(fifo* f, uint8_t* data, uint16_t size);

uint16_t fifo_size(fifo* f);

void bytes_to_number(void *dest, uint8_t *buf, int dest_size, int src_size, uint8_t sign);

#endif