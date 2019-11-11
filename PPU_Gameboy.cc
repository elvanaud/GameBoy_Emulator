#include "PPU_Gameboy.h"
#include "Bus.h"

#include <algorithm>
#include <iostream>

PPU_Gameboy::PPU_Gameboy()
{
    if(!texture.create(160,144))
    {
        std::cout << "ERREUR MONUMENTALE" << std::endl;
    }
    texture.setSmooth(false);
    sprite.setTexture(texture);
}

void PPU_Gameboy::tick()
{
    //test();
    scx = scy = 0;//TODO
    if(((lcdc>>7)&1)==0)
    {
        stat = 0;
        scanline = 0;
        dots = 4;
        mode = 2;
        cycles = 0;
        pixelcount = 0;
    }
    else{
        if(mode != VBLANK)
        {
            if(mode == 2 || dots == 0)
            {
                mode = 2;
                //Probably implement OAM Bug or something
                //For now, nothing happens
                if(dots == 79)
                {
                    mode = 3; //When we reach dots == 79, we ticked 80 times so we switch to mode 3(for next tick)
                }
            }
            else if(mode == 3)
            {
                if(cycles == 0) //Mode 3 n'est pas régulier, ce if sert à temporiser
                {
                    if(dots == 80) //pixelcount == 0) //Add sprites, bg scrolling and window as causes of cycles
                    {
                        cycles = 8; //Initial 8 pixels (just before the left edge of the screen)
                    }
                    else
                    {
                        //1 tick = 1 dot = 1 pixel:
                        generatePixel(scanline,pixelcount);
                        pixelcount++;
                    }
                }
                cycles = std::max(0,cycles-1);
            }
            if(pixelcount == 159)
            {
                //ENTER HBLANCK:
                mode = HBLANK;
            }
        }
        //if(scanline==0x8a)
            //std::cout<<"dots:"<<dots<<std::endl;
        dots++;
        if(dots == 456)
        {
            dots = 0;
            scanline++;
            pixelcount = 0;
        }
        if(scanline == 144) //ENTER VBLANK
        {
            mode = VBLANK;
            if((stat >> 4)&1) //Trigger VBLANK Interupt
            {
                bus->write(0xFF0F,bus->read(0xFF0F)|1);
                bus->triggerInterupt(1);
            }
            DrawScreen();
        }
        if(scanline == 153 + 1) //EXIT VBLANK
        {
            mode = 2;
            scanline = 0;
            dots = 0;
            pixelcount = 0;
        }
        stat &= 0b1'1111'0'11;//reset the lyc=ly flag
        if(scanline == lyc) //trigger it only once
        {
            stat |= 4;
            if(dots == 0 && (stat >> 6)&1) //Trigger VBLANK Interupt
            {
                bus->write(0xFF0F,bus->read(0xFF0F)|2);
                bus->triggerInterupt(2);
            }
        }
        stat = (stat&0b1'1111'1'00) + mode;
    }
}

void PPU_Gameboy::generatePixel(uint8_t y, uint8_t x)
{
    //TODO: No scrolling for now-->no wrapping
    uint8_t tileX = (x + scx) / 8;
    uint8_t tileY = (y + scy) / 8;
    uint8_t tileCode = vram[(tileY*32+tileX) + GetBGMapOffset()];
    uint16_t line = GetTileLine(tileCode,(y+scy)%8);

    uint8_t inTileX = (x + scx) % 8;
    //uint8_t inTileY = (y + scy) % 8;
    uint8_t upper = (line >> (7-inTileX))&1;
    uint8_t lower = ((line>>8) >> (7-inTileX))&1;
    uint8_t pixelData = lower + (upper<<1);
    uint8_t palette = bgp;
    UpdateScreen(pixelData,y,x,palette);
}

void PPU_Gameboy::UpdateScreen(uint8_t pixelData,uint8_t y, uint8_t x, uint8_t palette)
{
    int colors[4][4] = {{255,255,255,255},{180,180,180,255},{60,60,60,255},{0,0,0,255}};
    uint8_t colorPalette = (palette >> (pixelData*2))&3;
    screen[(y*160+x)*4+0] = colors[colorPalette][0];
    screen[(y*160+x)*4+1] = colors[colorPalette][1];
    screen[(y*160+x)*4+2] = colors[colorPalette][2];
    screen[(y*160+x)*4+3] = colors[colorPalette][3];
}

void PPU_Gameboy::DrawScreen()
{
    sf::RenderWindow & app = bus->getWindow();

    texture.update(screen);
    app.clear();
    app.draw(sprite);
    //app.display();

    //Display vram:
    sf::Image img;
    img.create(32*8,32*8);

    for(int tileNb = 0; tileNb < 0x1fff/16; tileNb++)//0x1800 pour tiles only (also show bgmap as tiles)
    {
        for(int i = 0; i < 8; i++)
        {
            uint8_t lower = vram[tileNb*16+2*i];
            uint8_t upper = vram[tileNb*16+2*i+1];

            for(int j = 0; j < 8; j++)
            {
                uint8_t dotData = (lower&1)+(upper&2);
                lower >>=1; upper>>=1;
                sf::Color color;
                switch(dotData)
                {
                case 0:
                    color = sf::Color(255,255,255);break;
                case 1:
                    color = sf::Color(255,0,0); break;
                case 2:
                    color = sf::Color(0,255,0); break;
                case 3:
                    color = sf::Color(0,0,0); break;
                }
                img.setPixel((tileNb%32)*8+j,(tileNb/32)*8+i,color);
            }
        }
    }

    sf::Texture t;
    t.loadFromImage(img);
    t.setSmooth(false);
    sf::Sprite s;
    s.setTexture(t);
    s.move(160,0);
    app.draw(s);
    app.display();
}

uint16_t PPU_Gameboy::GetTileLine(uint8_t tileCode,uint8_t line)
{
    uint16_t index = tileCode*16 + 2*line;
    if((lcdc >> 4)&1)
    {
        index += 0x8000;
    }
    else
    {
        if(tileCode >= 0x80)
        {
            index += 0x8000;
        }
        else
        {
            index += 0x8800;
        }
    }
    return vram[index-0x8000]+(vram[index-0x8000+1]<<8);
}

uint16_t PPU_Gameboy::GetBGMapOffset()
{
    return (((lcdc >> 3)&1) ? 0x9C00 : 0x9800)- 0x8000;
}

void PPU_Gameboy::write(uint16_t adr, uint8_t data)
{
    if(adr >= 0x8000 && adr <= 0x9FFF)
    {
        vram[adr-0x8000] = data;
    }
    switch(adr)
    {
    case 0xFF40:
        lcdc = data; std::cout<<"lcdc:"<<(int)lcdc<<"|"<<((lcdc>>7)&1)<<std::endl;break;
    case 0xFF41:
        stat = data; break;
    case 0xFF42:
        scy = data; break;
    case 0xFF43:
        scx = data; break;
    case 0xFF44: //LY
        //not writable
        break;
    case 0xFF45:
        lyc = data; break;
    case 0xFF47:
        std::cout << "WRITE TO BG PALETTE\n";
        bgp = data; break;
    case 0xFF48:
        obp0 = data; break;
    case 0xFF49:
        obp1 = data; break;
    case 0xFF4A:
        wy = data; break;
    case 0xFF4B:
        wx = data; break;
    }
}

uint8_t PPU_Gameboy::read(uint16_t adr)
{
    //TODO: implement blocking depending on current mode
    if(adr >= 0x8000 && adr <= 0x9FFF)
        return vram[adr-0x8000];
    switch(adr)
    {
    case 0xFF40:
        return lcdc; //TODO:should update value at some point
    case 0xFF41:
        return stat;
    case 0xFF42:
        return 0;//scy; //TODO:set that back
    case 0xFF43:
        return 0;//scx;
    case 0xFF44: //LY
        return scanline;
    case 0xFF45:
        return lyc;
    case 0xFF47:
        return bgp;
    case 0xFF48:
        return obp0;
    case 0xFF49:
        return obp1;
    case 0xFF4A:
        return wy;
    case 0xFF4B:
        return wx;
    }
    return 0xFF;
}
