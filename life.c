/* Conway's Game of Life for JY-MCU 3208 "Lattice Clock"
 *
 * Copyright (C) Pete Hollobon 2016
 */

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ht1632c.h"
#include "mq.h"
#include "life.h"
#include "timers.h"
#include "io.h"

void initialise(void)
{
    HTpinsetup();
    HTsetup();

    /* Set high 3 bits of port D as input */
    DDRD &= 0b00011111;

    /* Turn on pull-up resistors on high 3 bits */
    PORTD |= 0b11100000;

    DDRC |= 1 << 5;
}

void set_up_timers(void)
{
    cli();                      /* disable interrupts */

    ICR1 = F_CPU / 1000;        /* input capture register 1 - interrupt frequency 1000Hz */

    TCCR1A = 0;                 /* zero output compare stuff and the low two WGM bits */

    // timer counter control register 1 B: Mode 12, CTC with ICR1 as TOP, no prescaling
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);

    // Set interrupt on compare match
    TIMSK = (1 << TICIE1);

    sei();
}

/* Interrupt handler for timer1. Polls keys and pushes events onto message queue. */
ISR (TIMER1_CAPT_vect, ISR_NOBLOCK)
{
    handle_keys();
    handle_timers();
}

uint8_t count(uint8_t x, uint8_t y)
{
    return (((leds[x] & (1 << y)) ? 1 : 0)
            + ((leds[x] & (1 << ((y - 1) & 7))) ? 1 : 0)
            + ((leds[x] & (1 << ((y + 1) & 7))) ? 1 : 0)

            + ((leds[(x + 1) & 0x1f] & (1 << y)) ? 1 : 0)
            + ((leds[(x + 1) & 0x1f] & (1 << ((y - 1) & 7))) ? 1 : 0)
            + ((leds[(x + 1) & 0x1f] & (1 << ((y + 1) & 7))) ? 1 : 0)

            + ((leds[(x - 1) & 0x1f] & (1 << y)) ? 1 : 0)
            + ((leds[(x - 1) & 0x1f] & (1 << ((y - 1) & 7))) ? 1 : 0)
            + ((leds[(x - 1) & 0x1f] & (1 << ((y + 1) & 7))) ? 1 : 0));
}

void write_offset(character c, uint8_t line, uint8_t offset)
{
    uint8_t *p = leds + line + c.columns;
    for (int8_t n = c.columns - 1; n >= 0; n--)
        *p-- = c.bitmap[n] << offset;
}

#define MIN_INTERVAL 50
#define MAX_INTERVAL 1000
#define INTERVAL_INCREMENT 50

int main(void)
{
    message_t message;
    uint8_t next_grid[32];
    uint16_t interval = 150;
    uint8_t current_pattern = 0;

    initialise();
    set_up_timers();
    init_timers();
    HTbrightness(1);

    goto init;
    while (1) {
        if (mq_get(&message)) {
            switch (msg_get_event(message)) {
            case M_TIMER:
                memset(next_grid, 0, 32);
                for (uint8_t x = 0; x < 32; x++) {
                    for (uint8_t y = 0; y < 8; y++) {
                        uint8_t c = count(x, y);
                        if (c == 3)
                            next_grid[x] |= 1 << y;
                        else if (c == 4)
                            next_grid[x] |= leds[x] & (1 << y);
                    }
                }
                memcpy(leds, next_grid, 32);
                HTsendscreen();
                break;

            case M_KEY_DOWN:
            case M_KEY_REPEAT:
                switch (msg_get_param(message)) {
                case KEY_LEFT:
                    if (interval > MIN_INTERVAL) {
                        interval -= INTERVAL_INCREMENT;
                        set_timer(interval, 0, true);
                    }
                    break;

                case KEY_RIGHT:
                    if (interval < MAX_INTERVAL) {
                        interval += INTERVAL_INCREMENT;
                        set_timer(interval, 0, true);
                    }
                    break;

                case KEY_MIDDLE:
                    current_pattern = (current_pattern + 1) % (sizeof(patterns) / sizeof(character));
                init:
                    memset(leds, 0, 32);
                    write_offset(patterns[current_pattern], 10, 2);
                    HTsendscreen();
                    set_timer(interval, 0, true);
                    break;
                }
                break;
            }
        }
    }
}
