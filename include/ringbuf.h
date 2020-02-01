/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#ifndef _RINGBUF_H__
#define _RINGBUF_H__

/* Functional Description: Generic ring buffer library for deeply
   embedded system. See the unit tests for usage examples. */

#include <stdint.h>
#include <stdbool.h>

struct ring_buffer_t {
    volatile uint8_t *buffer;   /* block of memory or array of data */
    unsigned element_size;      /* how many bytes for each chunk */
    unsigned element_count;     /* number of chunks of data */
    volatile unsigned head;     /* where the writes go */
    volatile unsigned tail;     /* where the reads come from */
};
typedef struct ring_buffer_t ring_buffer;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    unsigned ringbuf_count(
        ring_buffer const *b);
    bool ringbuf_full(
        ring_buffer const *b);
    bool ringbuf_empty(
        ring_buffer const *b);
    volatile uint8_t *ringbuf_peek(
        ring_buffer const *b);
    bool ringbuf_pop(
        ring_buffer * b,
        uint8_t * data_element);
    bool ringbuf_put(
        ring_buffer * b,        /* ring buffer structure */
        uint8_t * data_element);        /* one element to add to the ring */
    bool ringbuf_put_front(
        ring_buffer * b,        /* ring buffer structure */
        uint8_t * data_element);
    volatile uint8_t *ringbuf_data_peek(
        ring_buffer * b);
    bool ringbuf_data_put(
        ring_buffer * b, volatile uint8_t *data_element);
    /* Note: element_count must be a power of two */
    void ringbuf_init(
        ring_buffer * b,        /* ring buffer structure */
        volatile uint8_t * buffer,      /* data block or array of data */
        unsigned element_size,  /* size of one element in the data block */
        unsigned element_count);        /* number of elements in the data block */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
