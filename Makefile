F_CPU = 8000000
ARCH = AVR8
MCU = atmega8

TARGET = life

SRC = life.c mq.c ht1632c.c timers.c

include rules.mk
