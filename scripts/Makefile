TARGETS=nsf_play dat_to_bin detect_loops bin_play


SOURCES_dat_to_bin=dat_to_bin.cpp dat_file.cpp
SOURCES_detect_loops=detect_loops.cpp 
SOURCES_nsf_play=nsf_play.cpp Wave_Writer.cpp dat_file.cpp gme/Blip_Buffer.cpp gme/Classic_Emu.cpp gme/Data_Reader.cpp gme/Effects_Buffer.cpp gme/gme.cpp gme/Gme_File.cpp gme/Multi_Buffer.cpp gme/Music_Emu.cpp gme/Nes_Apu.cpp gme/Nes_Cpu.cpp gme/Nes_Fme7_Apu.cpp gme/Nes_Namco_Apu.cpp gme/Nes_Oscs.cpp gme/Nes_Vrc6_Apu.cpp gme/Nsfe_Emu.cpp gme/Nsf_Emu.cpp

SOURCES_bin_play=bin_play.cpp dat_file.cpp gme/Blip_Buffer.cpp gme/Nes_Apu.cpp gme/Nes_Oscs.cpp Wave_Writer.cpp

OBJECTS_dat_to_bin=$(SOURCES_dat_to_bin:.cpp=.o)
OBJECTS_detect_loops=$(SOURCES_detect_loops:.cpp=.o)
OBJECTS_nsf_play=$(SOURCES_nsf_play:.cpp=.o)

OBJECTS_bin_play=$(SOURCES_bin_play:.cpp=.o)

CXXFLAGS=--std=gnu++1z -Wall

.phony: all clean

all: $(TARGETS)

dat_to_bin: $(OBJECTS_dat_to_bin)
	g++ $(CXXFLAGS) -o $@ $^

detect_loops: $(OBJECTS_detect_loops)
	g++ $(CXXFLAGS) -o $@ $^

nsf_play: $(OBJECTS_nsf_play)
	g++ $(CXXFLAGS) -o $@ $^

bin_play: $(OBJECTS_bin_play)
	g++ $(CXXFLAGS) -o $@ $^

dat_file_test: dat_file.cpp
	g++ $(CXXFLAGS) -DTEST_DAT_FILE -o $@ $^

%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $^

clean:
	rm -f $(OBJECTS_dat_to_bin) $(OBJECTS_detect_loops) $(OBJECTS_nsf_play) $(OBJECTS_bin_play) $(TARGETS)
