#ifndef _MBC_HPP
#define _MBC_HPP

#include <cstdint>
#include <fstream>
#include <array>
#include <iostream>

class Bus;

class MBC
{
public:
    MBC(std::ifstream & in) : input(in)
    {
        input.read((char*)romBank0, 0x4000);
        romType = romBank0[0x0147];
        romSizeHeader = romBank0[0x0148];
        ramSizeHeader = romBank0[0x0149];
        ramSize = std::array<int,6>{0x0,0x800,0x2000,0x8000,0x20'000,0x10'000}[ramSizeHeader];
        romSize = 0x8000 << romSizeHeader;
        extRam = new uint8_t[ramSize];

        std::cout << "Color flag: " << std::hex<<std::showbase<< (int)romBank0[0x143] << std::endl;
    }

    void attachBus(Bus * b)
    {
        bus = b;
    }
    virtual ~MBC()
    {
        if(extRam)
            delete[] extRam;
    }
    virtual uint8_t read(uint16_t adr) = 0;
    virtual void write(uint16_t adr, uint8_t data) = 0;

protected:
    std::ifstream &input;
    uint8_t romType;
    uint8_t romSizeHeader;
    uint8_t ramSizeHeader;
    int ramSize;
    int romSize;
    uint8_t romBank0[0x4000];
    uint8_t * extRam = nullptr;
    Bus * bus;
};

#endif // _MBC_HPP
