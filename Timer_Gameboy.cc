#include "Timer_Gameboy.h"
#include "Bus.h"

void Timer_Gameboy::write(uint16_t adr, uint8_t data)
{
    if(adr == 0xFF04)
    {
        div = 0;
    }
    if(adr == 0xFF05)
    {
        tima = data; //TODO:manage glitch here
    }
    if(adr == 0xFF06)
    {
        tma = data;
    }
    if(adr == 0xFF07)
    {
        tac = data;
    }
}

uint8_t Timer_Gameboy::read(uint16_t adr)
{
    if(adr == 0xFF04)
    {
        return div>>8;
    }
    if(adr == 0xFF05)
    {
        return tima;
    }
    if(adr == 0xFF06)
    {
        return tma;
    }
    if(adr == 0xFF07)
    {
        return tac;
    }
    return 0;
}

void Timer_Gameboy::tick()
{
    div++;
    //if(div == 0);
    uint8_t freq_selec = tac & 3;
    uint8_t timer_enable = (tac>>2)&1;
    uint8_t bit_num[] = {9,3,5,7};

    uint8_t output = (div >> (bit_num[freq_selec]))&1;
    output = output & timer_enable;

    if(prev_output && !output)
    {
        //std::cout << "Timer:"<<(int)tima<<std::endl;
        tima++;
        if(tima == 0)
        {
            //todo:glitches
            //std::cout << "interupt!\n";
            tima = tma;
            bus->write(0xFF0F,bus->read(0xFF0F)|(1<<2));
        }
    }

    prev_output = output;
}
