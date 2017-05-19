#include "flwFifo.h"
#include "string.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b));
/* 初始化FIFO */
void init_fifo(flwfifo_s *fifo, uint8_t *data, uint16_t num)
{
    if(!fifo || !data)
        return;
    fifo->num = num;
    fifo->mask = fifo->num - 1;
    fifo->rd = fifo->wr = 0;
    fifo->data = data;
}

/* 初始化FIFO */
int fifo_put_c(flwfifo_s *fifo, uint8_t ch)
{
    uint16_t index = 0;

    if(!fifo || is_fifo_full(fifo))
        return -1;
    index = fifo->wr & fifo->mask;
    fifo->data[index] = ch;
    fifo->wr++;
    return 0;
}

int fifo_puts(flwfifo_s *fifo, uint8_t *s, uint8_t len)
{
    uint16_t temp, index;
    
    if(!fifo || !s || (len > fifo_unused_size(fifo)))
        return -1;
    
    index = fifo->wr & fifo->mask;
    temp = MIN(len, fifo->num - index);
    
    memcpy(fifo->data+index, s, temp);
    memcpy(fifo->data, s + temp, len - temp);
    fifo->wr += len;
    
    return 0;
}

/* 从FIFO中取出一个字节 */
int fifo_get_c(flwfifo_s *fifo)
{
    uint16_t index = 0;
    int ret = -1;

    if(!fifo || is_fifo_empty(fifo))
        return ret;

    index = fifo->rd & fifo->mask;
    ret = fifo->data[index];
    fifo->rd++;
    return ret;
}

int fifo_gets(flwfifo_s *fifo, uint8_t *s, uint8_t len)
{
    uint16_t temp, index, used;
    
    if(!fifo || !s || (len == 0))
        return -1;
    
    used = fifo_used_size(fifo);
    if(used == 0)
        return -1;
    
    if(len > used)
        len = used;
    
    index = fifo->rd & fifo->mask;
    temp = MIN(len, fifo->num - index);
    
    memcpy(s, fifo->data + index, temp);
    memcpy(s + temp, fifo->data, len - temp);

    fifo->rd += len;
    return 0;
}

