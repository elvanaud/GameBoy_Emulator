#include "NoMBC.h"

NoMBC::NoMBC(std::ifstream & in) : MBC(in)
{
    input.read((char*) romBank1,0x4000);
    if(ramSize > 0x2000)
    {
        std::cout << "Error: No MBC Chip with too much extram specifed\n";
        ramSize = 0x2000;
    }
}

uint8_t NoMBC::read(uint16_t adr)
{
    if(adr >= 0x0000 && adr <= 0x7FFF) //ROM
    {
        if(adr <= 0x3FFF)
            return romBank0[adr];
        else
            return romBank1[adr-0x4000];
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        if(adr >= 0xA000+ramSize)
        {
            std::cout << "ERROR: Accessing Non Existent External RAM (R) \n";
            return 0;
        }
        return extRam[adr-0xA000];
    }
    return 0x00; //Should never reach here
}

void NoMBC::write(uint16_t adr, uint8_t data)
{
    if(adr >= 0x0000 && adr <= 0x7FFF) //ROM
    {
        //std::cout<<"Writing in ROM!: adr:"<<(int)adr<<" , value: "<<(int)data<<std::endl;
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        if(adr >= 0xA000+ramSize)
        {
            std::cout << "ERROR: Accessing Non Existent External RAM (W) \n";
            return;
        }
        extRam[adr-0xA000] = data;
    }
}
