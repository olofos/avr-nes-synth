MCU=atmega88p
F_CPU=14318180UL
PROGRAMMER?=usbtiny

TARGET=avr-nessynth-channel

SOURCES=main.c nes_apu.c interrupt.asm

CFLAGS=-ffixed-r2 -ffixed-r3 -ffixed-r4 -ffixed-r5 -ffixed-r6 -DASMINTERRUPT=1

LIBDIR=../lib/
include $(LIBDIR)/Makefile.inc

# -U lfuse:w:0xc7:m -U hfuse:w:0xdf:m -U efuse:w:0xf9:m

