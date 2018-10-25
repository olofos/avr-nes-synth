Q=@

MCU=atmega88p
F_CPU=14318180UL
PROGRAMMER?=usbtiny

TARGET=avr-nessynth-channel-bootloader

SOURCES=main.asm

CFLAGS=-Wall -DF_CPU=$(F_CPU) -mmcu=$(MCU) -fverbose-asm
LDFLAGS=-DF_CPU=$(F_CPU) -mmcu=$(MCU) -Wl,--section-start=.text=0x1800 -nostartfiles  -nodefaultlibs  -nostdlib

CSOURCES=$(filter %.c, $(SOURCES))
ASMSOURCES=$(filter %.asm, $(SOURCES))

OBJECTS=$(CSOURCES:.c=.o) $(ASMSOURCES:.asm=.o)
ASM=$(CSOURCES:.c=.s) $(ASMSOURCES:.asm=.s)

CC=avr-gcc
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
BIN_FORMAT=ihex

BAUD=115200
PROTOCOL=stk500v1
PART=$(MCU)
AVRDUDE=avrdude

RM=rm -f


all: $(TARGET).hex

clean:
	@echo Cleaning up
	$(Q)$(RM) $(OBJECTS) $(ASM) $(TARGET).elf $(TARGET).hex

upload: $(TARGET).hex
ifeq ($(PROGRAMMER),arduino)
	$(AVRDUDE) -p$(PART) -P$(PORT) -c$(PROGRAMMER) -b$(BAUD) -U flash:w:$<
else
#	$(AVRDUDE) -p$(PART) -c$(PROGRAMMER) -U flash:w:$<
	$(AVRDUDE) -p$(PART) -c$(PROGRAMMER) -B1 -U flash:w:$<
endif

lss: $(TARGET).lss

%.hex: %.elf
	@echo Copying $< to $@
	$(Q)$(OBJCOPY) -O $(BIN_FORMAT) -R .eeprom $< $@

$(TARGET).elf: $(OBJECTS)
	@echo Linking $@
	$(Q)$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(TARGET).lss: $(TARGET).elf
	@echo Dumping $< to $@
	$(Q)$(OBJDUMP) -h -S $< > $@

%.o: %.c
	@echo Compiling $<
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<
	$(Q)$(CC) -MM $(CFLAGS) $< > .deps/$*.d

%.o: %.asm
	@echo Assembling $<
	$(Q)$(CC) $(CFLAGS) -x assembler-with-cpp -c -o $@ $<

%.o: %.s
	@echo Assembling $<
	$(Q)$(CC) $(CFLAGS) -c -s -o $@ $<

%.s: %.c
	@echo Compiling $<
	$(Q)$(CC) $(CFLAGS) -S -o $@ $<
	$(Q)$(CC) -MM $(CFLAGS) $< > .deps/$*.d

%.s: %.asm
	@echo Pre-processing $<
	$(Q)$(CC) $(CFLAGS) -x assembler-with-cpp -E -o $@ $<
