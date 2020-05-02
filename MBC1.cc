#include "MBC1.h"
#include "Bus.h"

MBC1::MBC1(std::ifstream & in) : MBC(in)
{
    romBanks = new uint8_t[romSize];
    input.read((char*) romBanks, romSize);
    if(!input.good())
    {
        std::cout << "ERROR MBC1: error while reading file\n";
        if(input.eof())
        {
            std::cout << "End of file reached before rom loading finished. Read: " << input.gcount() << " expected: " << romSize << std::endl;
        }
    }
    currentBank = 0;
    if(romType == 3)
    {
        std::cout << "Battery ROM\n"; //TODO: implement saving
    }
}

uint8_t MBC1::read(uint16_t adr)
{
    if(adr >= 0x0000 && adr <= 0x7FFF) //ROM
    {
        if(adr <= 0x3FFF)
            return romBank0[adr];
        else
        {
            int index = currentBank*0x4000 + (adr-0x4000);
            if(index >= romSize)
            {
                std::cout << "ERROR - Reading outside of ROM: Rombank number: " << currentBank<<" adress: "<<adr<<std::endl;
                bus->triggerDebugMode(true);
                return 0;
                //currentBank &= 0b00'111'111;
                //index = currentBank*0x4000 + (adr-0x4000);
            }
            return romBanks[index];
        }

    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        if(!ramEnable) return 0;
        int ramBank = 0;
        if(bankMode == 1)
            ramBank = bankNumUpper;
        int ramIndex = ramBank * 0x2000 + (adr-0xA000);
        if((ramIndex >= ramSize) || ((ramBank == 0) && (adr >= (0xA000+ramSize)))) //bound checking
        {
            std::cout << "ERROR: Accessing Non Existent External RAM (R) \n";
            return 0;
        }
        return extRam[ramIndex];
    }
    return 0x00; //Should never reach here
}

void MBC1::write(uint16_t adr, uint8_t data)
{
    if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        if(!ramEnable) return;
        int ramBank = 0;
        if(bankMode == 1)
            ramBank = bankNumUpper;
        int ramIndex = ramBank * 0x2000 + (adr-0xA000);
        if((ramIndex >= ramSize) || ((ramBank == 0) && (adr >= (0xA000+ramSize)))) //bound checking
        {
            std::cout << "ERROR: Accessing Non Existent External RAM (W) \n";
            return;
        }
        extRam[ramIndex] = data;
    }
    else
    {
        uint8_t adrRange = adr >> (12+1);
        adrRange &= 3;
        switch(adrRange)
        {
        case 0:
            if((data & 0b0000'1111) == 0x0A)
                ramEnable = true;
            else
                ramEnable = false;
            break;
        case 1: //ROM Bank Number (lower 5 bits)
            romBankNumLower = data & 0b0001'1111;
            if(romBankNumLower != 0)
                romBankNumLower--;
            break;
        case 2:
            bankNumUpper = data & 0b0000'0011;
            break;
        case 3:
            bankMode = data & 1;
            break;
        }
        if(bankMode == 0) //ROM Mode
        {
            currentBank = bankNumUpper * 0x1F;
        }
        else
            currentBank = 0;
        currentBank+=romBankNumLower;
    }

}

