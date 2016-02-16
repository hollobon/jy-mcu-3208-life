/* Conway's Game of Life for JY-MCY 3208 "Lattice Clock"
 *
 * Copyright (C) Pete Hollobon 2016
 */

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ht1632c.h"
#include "mq.h"
#include "life.h"
#include "timers.h"

#define M_KEY_DOWN 8
#define M_KEY_UP 9
#define M_KEY_REPEAT 10
#define M_FADE_COMPLETE 11

#define DEBOUNCE_TIME 10

#define key_down(n) ((PIND & (1 << ((n) + 5))) == 0)
#define KEY_LEFT 0
#define KEY_MIDDLE 1
#define KEY_RIGHT 2
 
uint8_t EEMEM seed_address[32];

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

    // timer 2: CTC, OCR2 as TOP, clock / 64
    //    TCCR2 |= _BV(WGM21) | _BV(CS22);

    sei();
}

/* Interrupt handler for timer1. Polls keys and pushes events onto message queue. */
ISR (TIMER1_CAPT_vect, ISR_NOBLOCK)
{
    typedef enum {down, up} key_state;
    static key_state last_state[3] = {up, up, up};
    static bool steady_state[3] = {true, true, true};
    static bool key_repeating[3] = {false, false, false};
    static uint16_t state_change_clock[3] = {0, 0, 0};
    static uint16_t repeat_initial_delay[3] = {300, 300, 300};
    static uint16_t repeat_subsequent_delay[3] = {200, 50, 200};

    for (int key = 0; key < 3; key++) {
        if (key_down(key)) {
            if (last_state[key] == up) {
                last_state[key] = down;
                steady_state[key] = false;
                state_change_clock[key] = clock_count;
            }
            else if (clock_count - state_change_clock[key] > (key_repeating[key] ? repeat_subsequent_delay[key] : repeat_initial_delay[key])) {
                state_change_clock[key] = clock_count;
                mq_put(msg_create(M_KEY_REPEAT, key));
                key_repeating[key] = true;
            }
            else if (!steady_state[key] && clock_count - state_change_clock[key] > DEBOUNCE_TIME) {
                mq_put(msg_create(M_KEY_DOWN, key));
                steady_state[key] = true;
            }
        }
        else {
            if (last_state[key] == down) {
                last_state[key] = up;
                steady_state[key] = false;
                state_change_clock[key] = clock_count;
            }
            else if (!steady_state[key] && clock_count - state_change_clock[key] > DEBOUNCE_TIME) {
                mq_put(msg_create(M_KEY_UP, key));
                steady_state[key] = true;
                key_repeating[key] = false;
            }
        }
    }

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

int main(void)
{
    message_t message;
    uint8_t next_grid[32];

    initialise();
    set_up_timers();
    init_timers();
    HTbrightness(1);

    eeprom_read_block(leds, &seed_address, 32);
    memset(leds, 0, 32);
    //leds[3] = 8; leds[4] = 4; leds[5] = 28;
    leds[3] = 0xa << 2; leds[4] = 0x1 << 2; leds[5] = 0x1 << 2; leds[6] = 0x9 << 2; leds[7] = 0x7 << 2; 
    HTsendscreen();
    
    set_timer(150, 0, true);
    while (1) {
        if (mq_get(&message)) {
            if (msg_get_event(message) == M_TIMER) {
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
            }
        }
    }
}
