#ifndef _MBC1_HPP
#define _MBC1_HPP

#include "MBC.h"

class MBC1 : public MBC
{
public:
    MBC1(std::ifstream & input);// : MBC(input) {}
    virtual uint8_t read(uint16_t adr);
    virtual void write(uint16_t adr, uint8_t data);
};

#endif // _MBC1_HPP
