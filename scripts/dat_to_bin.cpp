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
    const char *out_filename = "out.bin";

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

    FILE *of = std::fopen(out_filename, "wb");

    int loop_frame = -1;

    for(size_t i = 0; i < frames.size(); i++)
    {
        auto frame = frames[i];

        if(frame.regs.size() > 0 && frame.regs[0].address == LOOP)
        {
            loop_frame = (frame.regs[1].address << 8) | frame.regs[1].value;
        } else {
            for(auto reg: frame.regs)
            {
                fwrite(&reg.address, 1, 1, of);
                fwrite(&reg.value, 1, 1, of);
            }

            unsigned char b = END_FRAME;

            fwrite(&b, 1, 1, of);
            fwrite(&b, 1, 1, of);
        }
    }

    if(loop_frame >= 0)
    {
        unsigned byte_count = 0;

        for(int i = 0; i < loop_frame; i++)
        {
            byte_count += 2*(frames[i].regs.size()+1);
        }

        std::printf("Loop at frame %d (0x%08X bytes)\n", loop_frame, byte_count);

        unsigned char b = LOOP;
        fwrite(&b, 1, 1, of);
        fwrite(&b, 1, 1, of);

        b = (byte_count >> 8) & 0xFF;
        fwrite(&b, 1, 1, of);
        b = byte_count & 0xFF;
        fwrite(&b, 1, 1, of);
    }

    unsigned char b = END_SONG;

    fwrite(&b, 1, 1, of);
    fwrite(&b, 1, 1, of);
    
    fclose(of);
}
