F_CPU = 8000000
ARCH = AVR8
MCU = atmega8

VPATH = ../libjymcu3208/include

TARGET = life

SRC = life.c

include rules.mk

EXTRALIBDIRS = ../libjymcu3208
LDFLAGS += -ljymcu3208
