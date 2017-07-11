CC=avr-gcc
CFLAGS+=-Wall -Werror -Wno-error=unused-function -Wno-error=unused-variable -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU) -std=gnu11 -I. -I$(LIBDIR) -gdwarf-2 -fverbose-asm -fshort-enums
LDFLAGS+=-Wall -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU) -std=gnu11 -I. -I$(LIBDIR) -gdwarf-2 -fverbose-asm -fshort-enums

#CFLAGS+=-ffunction-sections -fdata-sections
#LDFLAGS+=-Wl,-gc-sections -Wl,-print-gc-sections
#LDFLAGS+=-Wl,-gc-sections

#CFLAGS+=-flto
#LDFLAGS+=-flto=4

OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
BIN_FORMAT=ihex

PART=$(MCU)

BAUD=115200
PROTOCOL=stk500v1
PART=$(MCU)
AVRDUDE=avrdude

RM=rm -f

LIBSOURCES=logo-paw-64x64.c logo-paw-128x64.c fat32.c glcdfont.c log.c sd.c uart.c ssd1306.c ssd1306-console.c i2c-master.c i2c-slave.c

LIBOBJECTS=$(LIBSOURCES:.c=.o)

CSOURCES=$(filter %.c, $(SOURCES))
ASMSOURCES=$(filter %.asm, $(SOURCES))

OBJECTS=$(CSOURCES:.c=.o) $(ASMSOURCES:.asm=.o)
ASM=$(CSOURCES:.c=.s) $(ASMSOURCES:.asm=.s)

VPATH=$(LIBDIR)

LIBNAME=common
LIBNAMEFULL:=lib$(LIBNAME).a

DEPS=$(patsubst %.c, .deps/%.d, $(CSOURCES)) $(patsubst %.o, .deps/%.d, $(LIBOBJECTS))

.PHONY: all clean upload
.SUFFIXES:

all: $(TARGET).hex

-include $(DEPS)

clean:
	@echo Cleaning up
	@$(RM) $(OBJECTS) $(LIBOBJECTS) $(ASM) $(TARGET).elf $(TARGET).hex $(DEPS) $(LIBNAMEFULL)

upload: $(TARGET).hex
ifeq ($(PROGRAMMER),arduino)	
	$(AVRDUDE) -p$(PART) -P$(PORT) -c$(PROGRAMMER) -b$(BAUD) -U flash:w:$<
else
#	$(AVRDUDE) -p$(PART) -c$(PROGRAMMER) -U flash:w:$<
	$(AVRDUDE) -p$(PART) -c$(PROGRAMMER) -B1 -U flash:w:$<
endif

lss: $(TARGET).lss


$(LIBNAMEFULL): $(LIBOBJECTS)
	@echo Creating library $@
	@avr-ar rcs $@ $^

%.hex: %.elf
	@echo Copying $< to $@
	@$(OBJCOPY) -O $(BIN_FORMAT) -R .eeprom $< $@

$(TARGET).elf: $(OBJECTS) $(LIBNAMEFULL)
	@echo Linking $@
	@$(CC) $(OBJECTS) $(LDFLAGS) -L. -l$(LIBNAME) -o $@ 

$(TARGET).lss: $(TARGET).elf
	@echo Dumping $< to $@
	@$(OBJDUMP) -h -S $< > $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > .deps/$*.d

%.o: %.asm
	@echo Assembling $<
	@$(CC) $(CFLAGS) -x assembler-with-cpp -c -o $@ $<

%.o: %.s
	@echo Assembling $<
	@$(CC) $(CFLAGS) -c -s -o $@ $<

%.s: %.c
	@echo Compiling $<
	@$(CC) $(CFLAGS) -S -o $@ $<
	@$(CC) -MM $(CFLAGS) $< > .deps/$*.d

%.s: %.asm
	@echo Pre-processing $<
	@$(CC) $(CFLAGS) -x assembler-with-cpp -E -o $@ $<
