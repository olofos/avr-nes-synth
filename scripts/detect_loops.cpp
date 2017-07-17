#include <cstdio>
#include <cstdlib>

#include <vector>

#define BEGIN_FRAME    0xf0
#define END_FRAME      0xf1
#define END_SONG       0xff
#define LOOP           0xfe


struct reg_t
{
    unsigned address;
    unsigned value;
};

bool operator==(const reg_t& reg1, const reg_t& reg2)
{
    return (reg1.address == reg2.address) && (reg1.value == reg2.value);
}

bool operator!=(const reg_t& reg1, const reg_t& reg2)
{
    return !(reg1 == reg2);
}

struct frame_t
{
    int num;
    std::vector<reg_t> regs;
};

bool operator==(const frame_t& frame1, const frame_t& frame2)
{
    return frame1.regs == frame2.regs;    
}

bool operator!=(const frame_t& frame1, const frame_t& frame2)
{
    return frame1.regs != frame2.regs;    
}

int main(int argc, char *argv[])
{
    const char *filename = "out.dat";

    const char *out_filename = "out.dat";

    if(argc > 1)
    {
        filename = argv[1];
    }

    if(argc > 2)
    {
        out_filename = argv[2];
    }
    
    FILE *file = std::fopen(filename, "r");

    if(!file)
    {
        char str[256];
        std::snprintf(str, 256, "Error opening '%s'", filename);
        std::perror(str);
        std::exit(1);
    }

    bool song_done = false;
    int frame_count = 0;

    std::vector<frame_t> frames;

    while(!song_done)
    {
        bool frame_done = false;

        frame_t frame;
        frame.num = frame_count;

        while(!frame_done) {
            unsigned address;
            unsigned value;

            if(!std::fscanf(file, "0x%02x,\t0x%02x,\n", &address, &value))
            {
                std::perror("Error reading file");
                std::exit(1);
            }

            if(address == END_SONG)
            {
                frame_done = true;
                song_done  = true;
            } else if(address == END_FRAME) {
                frame_done = true;
            } else {
                reg_t reg;
                reg.address = address;
                reg.value = value;
                
                frame.regs.push_back(reg);
            }
        }

        frames.push_back(frame);
        frame_count++;
    }

    std::fclose(file);

    std::printf("Read %d frames\n", frame_count);

    int check_len = 8;

    int loop_start = -1, loop_end = -1;

    bool loop_found = false;

    for(int i = 1, j = 0; (i < frame_count - check_len) && !loop_found ; i+=2, j+=1)
    {
        bool loop_test = true;
        
        for(int k = 0; k < check_len; k++)
        {
            if(frames[i+k] != frames[j+k])
            {
                loop_test = false;
                break;
            }
        }

        if(loop_test)
        {
            printf("Possible loop at %d == %d\n", i, j);
            
            for(int k = 0; k < frame_count - i - 1; k++)
            {
                if(frames[i + k] != frames[j + k])
                {
                    std::printf("No loop: %d != %d\n", i+k, j+k);
                    loop_test = false;
                    break;
                }
            }

            if(loop_test)
            {
                loop_start = j;
                loop_end = i;
                loop_found = true;

                std::printf("Found loop at %d == %d (period %d)\n", i, j, i - j);
            }
        }
    }

    int loop_period = loop_end - loop_start;

    if(loop_found)
    {
        std::printf("Checking for an earlier loop\n");

        for(int i = 0; i < loop_start; i++)
        {
            bool loop_test = true;
        
            for(int k = 0; k < loop_period; k++)
            {
                if(frames[i+k] != frames[i+loop_period+k])
                {
                    loop_test = false;
                    break;
                }
            }

            if(loop_test)
            {
                loop_start = i;
                loop_end = i + loop_period;

                std::printf("Found loop at %d == %d (period %d)\n", i, i+loop_period, loop_period);
                break;
            }
        }

        std::printf("Checking for a shorter loop\n");

        for(int j = 1; j <= loop_period; j++)
        {
            bool loop_test = true;

            for(int k = 0; k < loop_period; k++)
            {
                if(frames[loop_start + k] != frames[loop_start + j + k])
                {
                    loop_test = false;
                    break;
                }
            }

            if(loop_test)
            {
                loop_end = loop_start + j;
                loop_period = loop_end - loop_start;
                std::printf("Found loop at %d == %d (period %d)\n", loop_start, loop_end, j);
                break;
            }
        }
    }

    bool end_song;

    if(!loop_found || ((loop_period == 1) && (frames[loop_start].regs.size() == 0)))
    {
        end_song = true;

        if(!loop_found)
        {
            loop_start = frames.size() - 1;
        }
        
        std::printf("Song ends at %d\n", loop_start);
    } else {
        end_song = false;
    }

    file = fopen(out_filename, "w");
    for(int i = 0; i < loop_start; i++)
    {
        for(unsigned j = 0; j < frames[i].regs.size(); j++)
        {
            fprintf(file, "0x%02x,\t0x%02x,\n", frames[i].regs[j].address, frames[i].regs[j].value);
        }
        fprintf(file, "0x%02x,\t0x%02x,\n", END_FRAME, END_FRAME);
    }
    
    if(!end_song)
    {
        for(int i = loop_start; i < loop_end; i++)
        {
            for(unsigned j = 0; j < frames[i].regs.size(); j++)
            {
                fprintf(file, "0x%02x,\t0x%02x,\n", frames[i].regs[j].address, frames[i].regs[j].value);
            }
            fprintf(file, "0x%02x,\t0x%02x,\n", END_FRAME, END_FRAME);
        }
        fprintf(file, "0x%02x,\t0x%02x,\n", LOOP, LOOP);
        fprintf(file, "0x%02x,\t0x%02x,\n", (loop_start >> 8) & 0x00ff, loop_start & 0x00ff);
        
        if(loop_start >= (1 << 16))
        {
            printf("Warning: loop start %d larger than %d!\n", loop_start, 1 << 16);
        }
    }

    fprintf(file, "0x%02x,\t0x%02x,\n", END_SONG, END_SONG);
    fclose(file);
}
