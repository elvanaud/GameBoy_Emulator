#include "MBC1.h"

MBC1::MBC1(std::ifstream & in) : MBC(in)
{

}

uint8_t MBC1::read(uint16_t adr)
{
    if(adr >= 0x0000 && adr <= 0x7FFF) //ROM
    {
        return romBank0[adr];
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        return extRam[adr-0xA000];
    }
}

void MBC1::write(uint16_t adr, uint8_t data)
{
    if(adr >= 0x0000 && adr <= 0x7FFF) //ROM
    {
        //std::cout<<"Writing in ROM!: adr:"<<(int)adr<<" , value: "<<(int)data<<std::endl;
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        extRam[adr-0xA000] = data;
    }
}

