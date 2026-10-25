#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

volatile uint8_t count = 0;
volatile uint8_t flag  = 0;

ISR(INT0_vect)
{
    EIMSK &= ~(1 << INT0);
    count++;
    flag = 1;
}

int main(void)
{
    uart_init();

    DDRD  &= ~(1 << PD2);
    PORTD |=  (1 << PD2);

    EICRA |=  (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    EIMSK |=  (1 << INT0);

    sei();

    while (1)
    {
        if (flag)
        {
            flag = 0;
            uart_puts("count = ");
            uart_put_u16(count);
            uart_puts("\r\n");

            _delay_ms(50);
            while (!(PIND & (1 << PD2)));//button still held
            _delay_ms(50);

            EIFR  = (1 << INTF0);//12.2.3 EIFR – External Interrupt Flag Register
            EIMSK |= (1 << INT0);
        }
    }
}