#ifndef _FLWFIFO_H
#define _FLWFIFO_H

#include "stdint.h"


typedef struct _fifo
{
    uint16_t num;       /* need be power of 2 */
    uint16_t mask;      /* mask = num - 1 */
    uint16_t rd;
    uint16_t wr;
    uint8_t *data;
}flwfifo_s;


#define fifo_used_size(fifo)    (uint16_t)(((fifo)->wr - (fifo)->rd))
#define fifo_unused_size(fifo)  (uint16_t)(((fifo)->num + (fifo)->rd - (fifo)->wr))
#define is_fifo_empty(fifo)     (((fifo)->wr - (fifo)->rd) <= 0u)
#define is_fifo_full(fifo)      (((fifo)->wr - (fifo)->rd) >= (fifo)->num)


void init_fifo(flwfifo_s *fifo, uint8_t *data, uint16_t num);

int fifo_put_c(flwfifo_s *fifo, uint8_t ch);
int fifo_puts(flwfifo_s *fifo, uint8_t *s, uint8_t len);

int fifo_get_c(flwfifo_s *fifo);
int fifo_gets(flwfifo_s *fifo, uint8_t *s, uint8_t len);


#endif
