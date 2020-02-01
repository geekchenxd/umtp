/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 by Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307
 USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

/* Functional Description: Generic ring buffer library for deeply
   embedded system. See the unit tests for usage examples. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "ringbuf.h"

/****************************************************************************
* DESCRIPTION: Returns the number of elements in the ring buffer
* RETURN:      Number of elements in the ring buffer
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
unsigned ringbuf_count(
    ring_buffer const *b)
{
    unsigned head, tail;        /* used to avoid volatile decision */

    if (b) {
        head = b->head;
        tail = b->tail;
        return head - tail;
    }

    return 0;
}

/****************************************************************************
* DESCRIPTION: Returns the empty/full status of the ring buffer
* RETURN:      true if the ring buffer is full, false if it is not.
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool ringbuf_full(
    ring_buffer const *b)
{
    return (b ? (ringbuf_count(b) == b->element_count) : true);
}

/****************************************************************************
* DESCRIPTION: Returns the empty/full status of the ring buffer
* RETURN:      true if the ring buffer is empty, false if it is not.
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool ringbuf_empty(
    ring_buffer const *b)
{
    return (b ? (ringbuf_count(b) == 0) : true);
}

/****************************************************************************
* DESCRIPTION: Looks at the data from the front of the list without removing it
* RETURN:      pointer to the data, or NULL if nothing in the list
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
volatile uint8_t *ringbuf_peek(
    ring_buffer const *b)
{
    volatile uint8_t *data_element = NULL;      /* return value */

    if (!ringbuf_empty(b)) {
        data_element = b->buffer;
        data_element += ((b->tail % b->element_count) * b->element_size);
    }

    return data_element;
}

/****************************************************************************
* DESCRIPTION: Copy the data from the front of the list, and removes it
* RETURN:      true if data was copied, false if list is empty
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool ringbuf_pop(
    ring_buffer * b,
    uint8_t * data_element)
{
    bool status = false;        /* return value */
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */
    unsigned i; /* loop counter */

    if (!ringbuf_empty(b)) {
        ring_data = b->buffer;
        ring_data += ((b->tail % b->element_count) * b->element_size);
        if (data_element) {
            for (i = 0; i < b->element_size; i++) {
                data_element[i] = ring_data[i];
            }
        }
        b->tail++;
        status = true;
    }

    return status;
}

/****************************************************************************
* DESCRIPTION: Adds an element of data to the end of the ring buffer
* RETURN:      true on succesful add, false if not added
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool ringbuf_put(
    ring_buffer * b,    /* ring buffer structure */
    uint8_t * data_element)
{       /* one element to add to the ring */
    bool status = false;        /* return value */
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */
    unsigned i; /* loop counter */

    if (b && data_element) {
        /* limit the amount of elements that we accept */
        if (!ringbuf_full(b)) {
            ring_data = b->buffer;
            ring_data += ((b->head % b->element_count) * b->element_size);
            for (i = 0; i < b->element_size; i++) {
                ring_data[i] = data_element[i];
            }
            b->head++;
            status = true;
        }
    }

    return status;
}

/****************************************************************************
* DESCRIPTION: Adds an element of data to the front of the ring buffer
* RETURN:      true on succesful add, false if not added
* ALGORITHM:   none
* NOTES:       moves the tail on add instead of head, so this function
*              can't be used if keeping producer and consumer
*              as separate processes (i.e. interrupts)
*****************************************************************************/
bool ringbuf_put_front(
    ring_buffer * b,    /* ring buffer structure */
    uint8_t * data_element)
{       /* one element to add to the front of the ring */
    bool status = false;        /* return value */
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */
    unsigned i = 0;     /* loop counter */

    if (b && data_element) {
        /* limit the amount of elements that we accept */
        if (!ringbuf_full(b)) {
            b->tail--;
            ring_data = b->buffer;
            ring_data += ((b->tail % b->element_count) * b->element_size);
            /* copy the data to the ring data element */
            for (i = 0; i < b->element_size; i++) {
                ring_data[i] = data_element[i];
            }
            status = true;
        }
    }

    return status;
}

/****************************************************************************
* DESCRIPTION: Gets a pointer to the next free data element of the buffer
*              without adding it to the ring.
* RETURN:      pointer to the next data chunk, or NULL if ring buffer is full.
* ALGORITHM:   none
* NOTES:       Use Ringbuf_Data_Peek with Ringbuf_Data_Put
*****************************************************************************/
volatile uint8_t *ringbuf_data_peek(ring_buffer * b)
{
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */

    if (b) {
        /* limit the amount of elements that we accept */
        if (!ringbuf_full(b)) {
            ring_data = b->buffer;
            ring_data += ((b->head % b->element_count) * b->element_size);
        }
    }

    return ring_data;
}

/****************************************************************************
* DESCRIPTION: Adds the previously peeked element of data to the end of the
*              ring buffer
* RETURN:      true if the buffer has space and the data element points to the
*              same memory previously peeked.
* ALGORITHM:   none
* NOTES:       Use Ringbuf_Data_Peek with Ringbuf_Data_Put
*****************************************************************************/
bool ringbuf_data_put(ring_buffer * b, volatile uint8_t *data_element)
{
    bool status = false;
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */

    if (b) {
        /* limit the amount of elements that we accept */
        if (!ringbuf_full(b)) {
            ring_data = b->buffer;
            ring_data += ((b->head % b->element_count) * b->element_size);
            if (ring_data == data_element) {
                /* same chunk of memory - okay to signal the head */
                b->head++;
                status = true;
            }
        }
    }

    return status;
}

/****************************************************************************
* DESCRIPTION: Configures the ring buffer
* RETURN:      none
* ALGORITHM:   none
* NOTES:
*   element_count must be a power of two
*****************************************************************************/
void ringbuf_init(
    ring_buffer * b,    /* ring buffer structure */
    volatile uint8_t * buffer,  /* data block or array of data */
    unsigned element_size,      /* size of one element in the data block */
    unsigned element_count)
{       /* number of elements in the data block */
    if (b) {
        b->head = 0;
        b->tail = 0;
        b->buffer = buffer;
        b->element_size = element_size;
        b->element_count = element_count;
    }

    return;
}

