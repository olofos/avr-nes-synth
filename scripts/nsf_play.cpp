// C++ example that opens a game music file and records 10 seconds to "out.wav"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "gme/Music_Emu.h"
#include "gme/Nsf_Emu.h"
#include "Wave_Writer.h"

void handle_error( const char* str );

void print_usage(char *p)
{
    fprintf(stderr, "Usage: %s [-t ntracks] [-s nsecs] [-o out] filename\n", p);
}

int main(int argc, char **argv)
{    
	long sample_rate = 44100; // number of samples per second
	int track = 0; // index of track to play (0 = first)
        char *filename = 0;
        char *filename_out = 0;
        int timeout = 30;

        char *filename_wav = 0;

        const char *opts = "t:o:s:w:";
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

        FILE *out = stdout;

        if(filename_out)
        {
            out = fopen(filename_out, "w");
        }

        if(wave)
        {
            delete wave;
        }

        // Print out
        int prev_time = 0;
        for(r : emu->apu_()->reg_writes)
        {
            if(r.time - prev_time > 10000)
                fprintf(out, "0xf1,\t0xf1,\n");

            prev_time = r.time;

            fprintf(out, "0x%02x,\t0x%02x,\n", r.address & 0xFF, r.data);
        }
        
        fprintf(out, "0xf1,\t0xf1,\n");
        fprintf(out, "0xff,\t0xff,\n");

        fclose(out);

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
