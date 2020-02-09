#ifndef _MBC1_HPP
#define _MBC1_HPP

#include "MBC.h"

class MBC1 : public MBC
{
public:
    MBC1(std::ifstream & input);
    virtual uint8_t read(uint16_t adr);
    virtual void write(uint16_t adr, uint8_t data);
private:
    uint8_t *romBanks = nullptr;
    int currentBank = 0;
    int romBankNumLower = 0;
    int bankNumUpper = 0;
    int bankMode = 0;
    bool ramEnable = false;
};

#endif // _MBC1_HPP
