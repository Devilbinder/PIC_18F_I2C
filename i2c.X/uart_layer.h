

#ifndef UART_LAYER_H
#define	UART_LAYER_H

#include <xc.h> 
#include <stdint.h>
#include <stdbool.h>
#include <p18f4520.h>

void uart_init(uint16_t gen_reg, unsigned sync,unsigned brgh, unsigned brg16);
void uart_send(uint8_t c);
void uart_receiver(uint8_t *c, bool *rx_flag);
void uart_send_array(uint8_t *c,uint16_t len);
void uart_send_string(uint8_t *c);


#endif	

