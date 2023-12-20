#include "tools.h"
#include "stdlib.h"

void init_fifo(fifo* f, uint16_t capacity) {
    f->capacity = capacity;
    f->size = 0;
    f->write_idx = 0;
    f->read_idx = 0;
    f->buf = (uint8_t*)malloc(capacity);
}

void free_fifo(fifo* f) {
    free(f->buf);
}

int fifo_write(fifo* f, uint8_t data, uint16_t size) {
    if (f->size + size > f->capacity) {
        return -1;
    }
    for(int i = 0; i < size; i++) {
        f->buf[f->write_idx] = data;
        f->write_idx = (f->write_idx + 1) % f->capacity;
    }
    f->size += size;
    return 0;
}

int fifo_read(fifo* f, uint8_t* data, uint16_t size) {
    if (f->size < size) {
        return -1;
    }
    for(int i = 0; i < size; i++) {
        data[i] = f->buf[f->read_idx];
        f->read_idx = (f->read_idx + 1) % f->capacity;
    }
    f->size -= size;
    return 0;
}

int fifo_peek(fifo* f, uint8_t* data, uint16_t size) {
    if (f->size < size) {
        return -1;
    }
    for(int i = 0; i < size; i++) {
        data[i] = f->buf[(f->read_idx + i) % f->capacity];
    }
    return 0;
}