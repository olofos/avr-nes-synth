// Inspired by https://github.com/dhylands/TimerUART/blob/master/CBUF.h

#ifndef CBUF_H_
#define CBUF_H_

/*
Usage:

#define my_cbuf_LEN 16; // Must be a power of two
struct
{
    uint8_t head;
    uint8_t tail;
    uint8_t buf[my_cbuf_LEN];
} my_cbuf;

cbuf_init(my_cbuf);
cbuf_push(my_cbuf, 'A');
c = cbuf_pop(my_cbuf);


*/

#define cbuf_init(cbuf) cbuf.head = cbuf.tail = 0

// numer of elements currently in the buffer
#define cbuf_len(cbuf) ( ( typeof(cbuf.head) ) ((cbuf.head) - (cbuf.tail)) )

#define cbuf_push(cbuf, elem)  (cbuf.buf)[ cbuf.head++ & ((cbuf##_LEN) - 1) ] = (elem)

#define cbuf_pop(cbuf) (cbuf.buf)[ cbuf.tail++ & ((cbuf##_LEN) -1 ) ]

#define cbuf_full(cbuf) ( cbuf_len(cbuf) == (cbuf##_LEN) )

#define cbuf_empty(cbuf) ( cbuf_len(cbuf) == 0 )

#define cbuf_peek(cbuf) (cbuf.buf)[ cbuf.tail & ((cbuf##_LEN) -1 ) ]


#endif
