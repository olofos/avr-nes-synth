#include <iostream>
#include <vector>

#include "dat_file.h"

int main(int argc, char *argv[])
{
    std::string filename_in = "out.dat";

    std::string filename_out = "out.dat";

    if(argc > 1)
    {
        filename_in = argv[1];
    }

    if(argc > 2)
    {
        filename_out = argv[2];
    }

    DatFile dat_file;

    dat_file.load_ascii(filename_in);

    std::vector<Frame> frames = dat_file.frames;

    std::cout << "Read " << frames.size() << " frames\n";

    int check_len = 8;

    int loop_start = -1, loop_end = -1;

    bool loop_found = false;

    for(unsigned i = 1, j = 0; (i < frames.size() - check_len) && !loop_found ; i+=2, j+=1)
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
            std::cout << "Possible loop at " << i << " == " << j << "\n";
            
            for(unsigned k = 0; k < frames.size() - i - 1; k++)
            {
                if(frames[i + k] != frames[j + k])
                {
                    std::cout << "Not a loop: " << (i+k) << " != " << (j+k) << "\n";
                    loop_test = false;
                    break;
                }
            }

            if(loop_test)
            {
                loop_start = j;
                loop_end = i;
                loop_found = true;

                std::cout << "Found loop at " << i << " == " << j << " (period " << (i-j) << ")\n";
            }
        }
    }

    int loop_period = loop_end - loop_start;

    if(loop_found)
    {
        std::cout << "Checking for an earlier loop\n";

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

                std::cout << "Found loop at " << i << " == " << (i+loop_period) << " (period " << loop_period << ")\n";
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
                std::cout << "Found loop at " << loop_start << " == " << loop_end << " (period " << j << ")\n";
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
        
        std::cout << "Song ends at " << loop_start << "\n";
    } else {
        end_song = false;
    }

    if(!end_song)
    {
        frames.erase(frames.begin() + loop_end, frames.end());
        Frame frame;
        frame.regs.push_back(Reg(LOOP_FRAME, LOOP_FRAME));
        frame.regs.push_back(Reg((loop_start >> 8) & 0x00ff, loop_start & 0x00ff));
        frames.push_back(frame);
    }

    dat_file.frames = frames;

    dat_file.save_ascii(filename_out);
}
