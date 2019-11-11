#ifndef PPU_GAMEBOY_H
#define PPU_GAMEBOY_H

#include <cstdint>
#include <SFML/Graphics.hpp>

class Bus;

class PPU_Gameboy
{
private:
    Bus * bus;
    uint8_t scanline = 0;
    uint16_t dots = 16;
    enum Mode {HBLANK = 0, VBLANK = 1, OAMSCAN = 2, PICGEN = 3};
    uint8_t mode = 2;
    int8_t cycles = 0;
    uint8_t pixelcount = 0;
    uint8_t scx = 0,scy = 0;
    uint8_t vram[0x2000];
    uint8_t lcdc = 0x91;
    uint8_t bgp = 0b11'10'01'00,obp0=0b00'01'10'11,obp1;
    uint8_t lyc;
    uint8_t wy,wx;
    uint8_t stat = 0;
    uint8_t screen[160*144*4];
    void DrawScreen();
    sf::Texture texture;
    sf::Sprite sprite;

    void generatePixel(uint8_t y, uint8_t x);
    uint16_t GetBGMapOffset();
    uint16_t GetTileLine(uint8_t tileCode,uint8_t line);
    void UpdateScreen(uint8_t pixelData,uint8_t y, uint8_t x, uint8_t palette);
public:
    PPU_Gameboy();

    void attachBus(Bus * b)
    {
        bus = b;
    }

    void tick();

    void write(uint16_t adr, uint8_t data);
    uint8_t read(uint16_t adr);

    void test()
    {
        uint8_t pic[16] = {
            0b00'011'000,
            0b00'011'000,

            0b00'111'100,
            0b00'100'100,

            0b00'111'100,
            0b00'100'100,

            0b00'111'100,
            0b00'100'100,

            0b01'111'110,
            0b01'100'110,

            0b11'111'111,
            0b10'011'001,

            0b11'111'111,
            0b10'011'001,

            0b01'100'110,
            0b01'100'110
        };

        for(uint16_t i = 0; i < 16; i++)
        {
            //vram[i] = pic[i];
            vram[i+16*32] = pic[i];
        }

        for(uint16_t i = 0x9800; i < 0x9FFF; i++)
        {
            //vram[i-0x8000] = 36;
        }
    }
};

#endif // PPU_GAMEBOY_HPP