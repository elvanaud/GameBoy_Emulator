#ifndef _NoMBC_HPP
#define _NoMBC_HPP

#include "MBC.h"

class NoMBC : public MBC
{
public:
    NoMBC(std::ifstream & input);
    virtual uint8_t read(uint16_t adr);
    virtual void write(uint16_t adr, uint8_t data);
private:
    uint8_t romBank1[0x4000];
};

#endif // _NoMBC_HPP
