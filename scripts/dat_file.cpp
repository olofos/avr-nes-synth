#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "dat_file.h"

bool operator==(const Reg& reg1, const Reg& reg2)
{
    return (reg1.address == reg2.address) && (reg1.value == reg2.value);
}

bool operator!=(const Reg& reg1, const Reg& reg2)
{
    return !(reg1 == reg2);
}

bool operator==(const Frame& frame1, const Frame& frame2)
{
    return frame1.regs == frame2.regs;    
}

bool operator!=(const Frame& frame1, const Frame& frame2)
{
    return frame1.regs != frame2.regs;    
}

void DatFile::clear()
{
    frames.clear();
}

void expect(std::string::iterator& it, char c)
{
    if(*it != c)
    {
        std::stringstream s;
        s << "Error: Expected '" << c << "' but got '" << *it << "'";
        throw DatFileException(s.str());
    }
    it++;
}

static void expect(std::string::iterator& it, std::string s)
{
    for(c : s)
    {
        if(*it != c)
        {
            std::stringstream s;
            s << "Error: Expected '" << c << "' but got '" << *it << "'";
            throw DatFileException(s.str());
        }
        it++;
    }
}

static void skip_whitespace(std::string::iterator& it)
{
    while((*it == ' ') || (*it == '\t'))
    {
        it++;
    }
}

static int read_hex2(std::string::iterator& it)
{
    std::string digits(it, it+2);
    it += 2;

    return std::stoi(digits, 0, 16);
}

void DatFile::load_ascii(std::istream& in)
{
    std::string line;

    Frame frame;

    while(std::getline(in, line))
    {
        auto it = line.begin();

        expect(it, "0x");
        int address = read_hex2(it);
        expect(it, ",");
        skip_whitespace(it);
        expect(it, "0x");
        int value = read_hex2(it);
        expect(it, ",");

        if(address == END_FRAME) {
            frames.push_back(frame);
            frame.regs.clear();
        } else if(address == END_SONG) {
            if(!frame.regs.empty())
            {
                frames.push_back(frame);
                frame.regs.clear();
            }
        } else {
            frame.regs.push_back(Reg(address, value));
        }
    }

    std::cout << "Read " << frames.size() << " frames\n";
}

void DatFile::load_binary(std::istream& in)
{
    int address, value;

    Frame frame;
    
    while((address = in.get()) >= 0 && (value = in.get()) >= 0)
    {
        if(address == END_FRAME) {
            frames.push_back(frame);
            frame.regs.clear();
        } else if(address == END_SONG) {
            if(!frame.regs.empty())
            {
                frames.push_back(frame);
                frame.regs.clear();
            }
        } else {
            frame.regs.push_back(Reg(address, value));
        }
    }

    std::cout << "Read " << frames.size() << " frames\n";
}

static void output_reg_ascii(std::ostream& out, Reg reg)
{
    out << "0x";
    out << std::hex << std::setfill('0') << std::setw(2) << reg.address;
    out << ",\t0x";
    out << std::hex << std::setfill('0') << std::setw(2) << reg.value;
    out << ",\n";
}

static void output_reg_binary(std::ostream& out, Reg reg)
{
    out.put((char)reg.address);
    out.put((char)reg.value);
}

void DatFile::save_ascii(std::ostream& out)
{
    for(auto frame : frames)
    {
        for(auto reg : frame.regs)
        {
            output_reg_ascii(out, reg);
        }
        output_reg_ascii(out, Reg(END_FRAME, END_FRAME));
    }
    output_reg_ascii(out, Reg(END_SONG, END_SONG));
}

void DatFile::save_binary(std::ostream& out)
{
    for(auto frame : frames)
    {
        for(auto reg : frame.regs)
        {
            output_reg_binary(out, reg);
        }
        output_reg_binary(out, Reg(END_FRAME, END_FRAME));
    }
    output_reg_binary(out, Reg(END_SONG, END_SONG));
}

static std::string file_open_error(const std::string& filename)
{
    std::stringstream s;

    s << "Error: Could not open file " << filename;
    return s.str();
}

void DatFile::load_ascii(const std::string& filename)
{
    std::ifstream in(filename);

    if(!in.good())
    {
        throw DatFileException(file_open_error(filename));
    }

    load_ascii(in);
}

void DatFile::load_binary(const std::string& filename)
{
    std::ifstream in(filename, std::ios::binary);

    if(!in.good())
    {
        throw DatFileException(file_open_error(filename));
    }

    load_binary(in);
}

void DatFile::save_ascii(const std::string& filename)
{
    std::ofstream out(filename);

    if(!out.good())
    {
        throw DatFileException(file_open_error(filename));
    }

    save_ascii(out);
}

void DatFile::save_binary(const std::string& filename)
{
        std::ofstream out(filename, std::ios::binary);

    if(!out.good())
    {
        throw DatFileException(file_open_error(filename));
    }

    save_binary(out);
}


#ifdef TEST_DAT_FILE

int main()
{
    DatFile dat_file;

    dat_file.load_ascii("out.dat");
    dat_file.save_ascii("out_new.dat");
    dat_file.clear();
    dat_file.load_binary("out.bin");
    dat_file.save_binary("out_new.bin");
}

#endif
