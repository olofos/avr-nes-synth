#ifndef DAT_FILE_H_
#define DAT_FILE_H_

#include <vector>
#include <string>
#include <istream>

enum
{
    BEGIN_FRAME = 0xf0,
    END_FRAME = 0xf1,
    END_SONG = 0xff,
    LOOP = 0xfe
};

struct Reg
{
    Reg(unsigned a, unsigned v) : address(a), value(v) {};

    unsigned address;
    unsigned value;
};

struct Frame
{
    std::vector<Reg> regs;
};

bool operator==(const Reg& reg1, const Reg& reg2);
bool operator!=(const Reg& reg1, const Reg& reg2);
bool operator==(const Frame& frame1, const Frame& frame2);
bool operator!=(const Frame& frame1, const Frame& frame2);

class DatFileException
{
public:
    DatFileException(std::string msg) : message(msg) {}

    std::string message;
};

class DatFile
{
public:
    void load_ascii(std::istream& in);
    void load_binary(std::istream& in);
    void save_ascii(std::ostream& out);
    void save_binary(std::ostream& out);

    void load_ascii(const std::string& filename);
    void load_binary(const std::string& filename);
    void save_ascii(const std::string& filename);
    void save_binary(const std::string& filename);

    void clear();

    std::vector<Frame> frames;
};




#endif
