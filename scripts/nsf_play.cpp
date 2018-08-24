// C++ example that opens a game music file and records 10 seconds to "out.wav"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "gme/Music_Emu.h"
#include "gme/Nsf_Emu.h"
#include "Wave_Writer.h"

#include <iostream>
#include <fstream>

#include "dat_file.h"

void handle_error( const char* str );

void print_usage(char *p)
{
    fprintf(stderr, "Usage: %s [-t track] [-s nsecs] [-o out] filename\n"
            "       %s -c|-b|-f filename\n",
            p, p);
}

int main(int argc, char **argv)
{    
	long sample_rate = 44100; // number of samples per second
	int track = 0; // index of track to play (0 = first)
        char *filename = 0;
        char *filename_out = 0;
        int timeout = 30;
        int print_track_count = 0;
        int print_chip_flags = 0;
        int print_bank_count = 0;

        char *filename_wav = 0;

        const char *opts = "t:o:s:w:cbf";
        int opts_done = 0;
        
        while(!opts_done)
        {
            switch(getopt (argc, argv, opts))
            {
            case EOF:
                opts_done = 1;
                break;

            case 't':
                track = strtol(optarg, 0, 10) - 1;
                break;

            case 'o':
                filename_out = (char*) malloc(strlen(optarg)+1);
                strcpy(filename_out, optarg);
                break;

            case 'w':
                filename_wav = (char*) malloc(strlen(optarg)+1);
                strcpy(filename_wav, optarg);
                break;

            case 's':
                timeout = strtol(optarg, 0, 10);
                break;

            case 'b':
                print_bank_count = 1;
                break;

            case 'c':
                print_track_count = 1;
                break;

            case 'f':
                print_chip_flags = 1;
                break;

            default:
                print_usage(argv[0]);
                exit(1);
                break;
            }
        }

        if(argc <= optind)
        {
            print_usage(argv[0]);
            exit(1);
        }

        filename = (char*) malloc(strlen(argv[optind])+1);
        strcpy(filename, argv[optind]);
        
        Nsf_Emu* emu = new Nsf_Emu();
       
	handle_error( emu->set_sample_rate( sample_rate ) );
	
	// Load music file into emulator
	handle_error( emu->load_file( filename ) );

        if(print_track_count)
        {
            printf("%d\n", emu->header().track_count);
            return 0;
        }

        if(print_chip_flags)
        {
            if(emu->header().chip_flags & 0x01) {
                printf("Uses VRC6 audio\n");
            }

            if(emu->header().chip_flags & 0x02) {
                printf("Uses VRC7 audio\n");
            }

            if(emu->header().chip_flags & 0x04) {
                printf("Uses FDS audio\n");
            }

            if(emu->header().chip_flags & 0x08) {
                printf("Uses MMC5 audio\n");
            }

            if(emu->header().chip_flags & 0x10) {
                printf("Uses Namco 163 audio\n");
            }

            if(emu->header().chip_flags & 0x20) {
                printf("Uses Sunsoft 5B audio\n");
            }
            return 0;
        }

        if(print_bank_count) {
            printf("%d\n", emu->rom_size() / 4096);
            return 0;
        }

	// Start track
	handle_error( emu->start_track( track ) );

        Wave_Writer *wave = 0;

        if(filename_wav)
        {            
            // Begin writing to wave file
            wave = new Wave_Writer( sample_rate, filename_wav );
            wave->enable_stereo();
        }
	
	// Record 10 seconds of track
	while ( emu->tell() < timeout * 1000L )
	{
		// Sample buffer
		const long size = 1024; // can be any multiple of 2
		short buf [size];
		
		// Fill buffer
		handle_error( emu->play( size, buf ) );

                if(wave)
                {
                    // Write samples to wave file
                    wave->write( buf, size );
                }
	}

        if(wave)
        {
            delete wave;
        }

		DatFile dat_file;
		Frame frame;

        int prev_time = 0;
        const int frame_time_const = 29830;
        for(r : emu->apu_()->reg_writes)
        {
            if(r.time - prev_time >= 10000) // try to sync with frames in the audio
            {
                dat_file.frames.push_back(frame);
                frame.regs.clear();

                while(r.time - prev_time >= frame_time_const + 3000) // allow for some extra time
                {
                    dat_file.frames.push_back(frame);
                    prev_time += frame_time_const;
                }

            }

            prev_time = r.time;

            frame.regs.push_back(Reg(r.address & 0xFF, r.data));
        }

        if(filename_out)
        {
            dat_file.save_ascii(filename_out);
        } else {
            dat_file.save_ascii(std::cout);
        }

	// Cleanup
	delete emu;
	
	return 0;
}

void handle_error( const char* str )
{
	if ( str )
	{
		printf( "Error: %s\n", str ); 
		exit( EXIT_FAILURE );
	}
}
