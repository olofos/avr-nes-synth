#include <vector>
#include <iostream>

#include "dat_file.h"

int main(int argc, char *argv[])
{
    std::string filename_in = "out.dat";
    std::string filename_out = "out.bin";

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

    Frame &last_frame = dat_file.frames.back();

    if(!last_frame.regs.empty() && last_frame.regs[0].address == LOOP_FRAME)
    {
        int loop_frame_dest = (last_frame.regs[1].address << 8) | last_frame.regs[1].value;
        std::cout << "Loop to frame " << loop_frame_dest << "\n";

        int loop_byte_dest = 0;

        for(int i = 0; i < loop_frame_dest; i++)
        {
            loop_byte_dest += dat_file.frames[i].regs.size() * 2;
        }

        std::cout << "Loop to byte " << loop_byte_dest << "\n";

        dat_file.frames.pop_back();

        Frame loop_frame;

        loop_frame.regs.push_back(Reg(LOOP_BYTE, LOOP_BYTE));
        loop_frame.regs.push_back(Reg((loop_byte_dest >> 8)  & 0xFF , loop_byte_dest & 0xFF));

        dat_file.frames.push_back(loop_frame);
    }

    dat_file.save_binary(filename_out);
}
