#ifndef _TIMER_HPP
#define _TIMER_HPP

class Bus;

class Timer_Gameboy
{
private:
    Bus * bus;
public:
    Timer_Gameboy();

    void attachBus(Bus * b)
    {
        bus = b;
    }
};

#endif // _TIMER_HPP
