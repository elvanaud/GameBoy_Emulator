#ifndef _TIMER_HPP
#define _TIMER_HPP
#include <cstdint>

class Bus;

class Timer_Gameboy
{
private:
    Bus * bus;
    uint16_t div = 0;
    uint8_t tma = 0,tac = 0;//...
    uint8_t tima = 0;
    bool prev_output = false;
public:
    //Timer_Gameboy(); default is fine

    void attachBus(Bus * b)
    {
        bus = b;
    }

    void write(uint16_t adr, uint8_t data);
    uint8_t read(uint16_t adr);

    void tick();
};

#endif // _TIMER_HPP

