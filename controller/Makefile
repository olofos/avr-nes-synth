MCU=atmega328p
F_CPU=14318180UL
PROGRAMMER?=usbtiny

TARGET=avr-nessynth-controller

SOURCES=main.c io-bridge.c menu.c

LIBDIR=../lib/
include $(LIBDIR)/Makefile.inc

# -U lfuse:w:0xc7:m -U hfuse:w:0xd1:m -U efuse:w:0xf9:m
