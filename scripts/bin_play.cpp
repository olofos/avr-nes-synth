#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "gme/gme.h"
#include "gme/Nes_Apu.h"
#include "Wave_Writer.h"

Blip_Buffer buf;
Nes_Apu apu;

void output_samples( const blip_sample_t*, size_t count );
const int out_size = 4096;
blip_sample_t out_buf [out_size];

int total_cycles;
int frame_cycles;

const int apu_addr = 0x4000;

void report_error(blargg_err_t error)
{
    fprintf(stderr, "Error: %s\n", error);
    exit(1);
}

void end_time_frame( int length )
{
    apu.end_frame( length );
    buf.end_frame( length );
    total_cycles -= length;     
	
    // Read some samples out of Blip_Buffer if there are enough to
    // fill our output buffer
    if ( buf.samples_avail() >= out_size )
    {
        size_t count = buf.read_samples( out_buf, out_size );
        output_samples( out_buf, count );
    }
}

void begin_frame()
{
    frame_cycles = 29830*2;
}

Wave_Writer *wave = 0;

void output_samples( const blip_sample_t* buf, size_t count )
{
    wave->write( buf, count );
}

int dmc_read( void*, nes_addr_t addr )
{
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 1)
    {
        fprintf(stderr, "Usage: %s bin_file\n", argv[0]);
    }
    
    char *filename_in = (char*) malloc(strlen(argv[1])+1);

    strcpy(filename_in, argv[1]);

    wave = new Wave_Writer(44100, "out.wav");
    wave->enable_stereo();


    FILE *in = fopen(filename_in, "rb");

    blargg_err_t error = buf.sample_rate( 44100 );
    if ( error )
        report_error( error );
    buf.clock_rate( 1789773 );
    apu.output( &buf );
    apu.dmc_reader( dmc_read );
    apu.reset(false);
    apu.dmc_reader( dmc_read );


    uint8_t data[2];

    int done = 0;

    begin_frame();

    while(fread(data, 2, 1, in) && !done)
    {
        int address = data[0];
        int value = data[1];

        if(address == 0xFF)
        {
            done = true;
            break;
        }

        if(address == 0xF1)
        {
            end_time_frame( frame_cycles );
            total_cycles += frame_cycles;
            begin_frame();
        } else {
            total_cycles += 0;
            apu.write_register(0, total_cycles, address + apu_addr, value);
            frame_cycles -= 0;
        }
    }

    fclose(in);

    delete wave;
    
    return 0;
}
