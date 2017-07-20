#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "gme/gme.h"
#include "gme/Nes_Apu.h"
#include "Wave_Writer.h"

#include <string>
#include <fstream>

#include "dat_file.h"

#ifdef ALSA
#include <alsa/asoundlib.h>
#endif

Blip_Buffer buf;
Nes_Apu apu;

void output_samples( const blip_sample_t*, size_t count );
const int out_size = 4096;
blip_sample_t out_buf [out_size];

int total_cycles;
int frame_cycles;

const int apu_addr = 0x4000;

static unsigned int freq = 44100;


/// ALSA /////////////////////////////////////////////////////////////////////////////////////

#ifdef ALSA

snd_pcm_t *pcm_handle;

static void open_hardware(const char *device)
{
    int pcm_error;

    /* Open the PCM device in playback mode */
    pcm_error = snd_pcm_open(&pcm_handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (pcm_error < 0)
    {
        printf("ERROR: Can't open \"%s\" PCM device. %s\n", device, snd_strerror(pcm_error));
        exit(1);
    }
}

static void init_hardware(void)
{
    int pcm_error;
    unsigned tmp;

    snd_pcm_hw_params_t *params;

    /* Allocate parameters object and fill it with default values*/

    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(pcm_handle, params);

    /* Set parameters */
    pcm_error = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (pcm_error < 0)
    {
        printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm_error));
        exit(1);
    }

    pcm_error = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    if (pcm_error < 0)
    {
        printf("ERROR: Can't set format. %s\n", snd_strerror(pcm_error));
        exit(1);
    }

    pcm_error = snd_pcm_hw_params_set_channels(pcm_handle, params, 2);
    if (pcm_error < 0)
    {
        printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm_error));
        exit(1);
    }

    pcm_error = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &freq, 0);
    if (pcm_error < 0)
    {
        printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm_error));
        exit(1);
    }

    pcm_error = snd_pcm_hw_params_set_buffer_size(pcm_handle, params, out_size * 4);
    if (pcm_error < 0)
    {
        printf("ERROR: Can't set buffer size. %s\n", snd_strerror(pcm_error));
        exit(1);
    }


    /* Write parameters */
    pcm_error = snd_pcm_hw_params(pcm_handle, params);
    if (pcm_error < 0)
    {
        printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm_error));
        exit(1);
    }

    /* Resume information */
    printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));
    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

    snd_pcm_uframes_t f;
    snd_pcm_hw_params_get_buffer_size(params, &f);
    printf("PCM buffer size: %lu\n", f);

    snd_pcm_hw_params_get_channels(params, &tmp);
    printf("Channels: %i ", tmp);

    if (tmp == 1)
        printf("(mono)\n");
    else if (tmp == 2)
        printf("(stereo)\n");

    snd_pcm_hw_params_get_rate(params, &tmp, 0);
    printf("Rate: %d bps\n", tmp);
}

/* close what we've opened */
static void close_hardware(void)
{
    snd_pcm_drop(pcm_handle);
    snd_pcm_close(pcm_handle);
}

void output_samples_alsa( const blip_sample_t* buf, size_t count )
{
    int pcm_error = snd_pcm_writei(pcm_handle, buf, count/2);
    if (pcm_error == -EPIPE) {
        printf("XRUN.\n");
        snd_pcm_prepare(pcm_handle);
    } else if (pcm_error < 0) {
        printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm_error));
    }
}

#endif


//////////////////////////////////////////////////////////////////////////////////////////////

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

#ifdef ALSA
    output_samples_alsa(buf,count);
#endif
}

int dmc_read( void*, nes_addr_t addr )
{
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s bin_file [wav_file]\n", argv[0]);
        exit(1);
    }
    
    std::string filename_in(argv[1]);
    std::string filename_out("out.wav");

    if(argc > 2)
    {
        filename_out = argv[2];
    }

    wave = new Wave_Writer(freq, filename_out.c_str());
    wave->enable_stereo();

    blargg_err_t error = buf.sample_rate( freq );
    if ( error )
        report_error( error );
    buf.clock_rate( 1789773 );
    apu.output( &buf );
    apu.dmc_reader( dmc_read );
    apu.reset(false);
    apu.dmc_reader( dmc_read );

#ifdef ALSA
    open_hardware("default");
    init_hardware();
#endif

    DatFile dat_file;

    dat_file.load_binary(filename_in);

    int start_byte = 0;
    bool continue_playing  = true;

    while(continue_playing)
    {
        continue_playing = false;

        int start_frame = 0;
        int bytes_read = 0;

        while(bytes_read < start_byte)
        {
            bytes_read += dat_file.frames[start_frame++].regs.size() * 2;
        }

        printf("Playing from byte %d, frame %d\n", start_byte, start_frame);

        for(auto it = dat_file.frames.begin() + start_frame; it != dat_file.frames.end(); it++)
        {
            Frame &frame = *it;
            bool found_loop = false;
            for(Reg reg : frame.regs)
            {
                if(found_loop)
                {
                    found_loop = false;
                    start_byte = (reg.address << 8) | reg.value;
                    continue_playing = true;
                    break;
                } else if(reg.address == LOOP_BYTE) {
                    found_loop = true;
                } else {
                    total_cycles += 0;
                    apu.write_register(0, total_cycles, reg.address + apu_addr, reg.value);
                    frame_cycles -= 0;
                }
            }

            end_time_frame( frame_cycles );
            total_cycles += frame_cycles;
            begin_frame();
        }
    }
    delete wave;

#ifdef ALSA
    snd_pcm_drain(pcm_handle);
    close_hardware();
#endif

    return 0;
}
