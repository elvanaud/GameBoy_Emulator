#include "PPU_Gameboy.h"
#include "Bus.h"

#include <algorithm>
#include <iostream>

#include <SDL2/SDL.h>

PPU_Gameboy::PPU_Gameboy(){}

PPU_Gameboy::~PPU_Gameboy()
{
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void PPU_Gameboy::tick()
{
    //test();
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
                if(mode == 2 && dots == 0)
                {
                    if((stat >> 5)&1)
                        bus->write(0xFF0F,bus->read(0xFF0F)|2);
                }
                //Probably implement OAM Bug or something
                //For now, nothing happens
                if(dots == 79)
                {
                    mode = 3; //When we reach dots == 79, we ticked 80 times so we switch to mode 3(for next tick)
                }
            }
            else if(mode == 3)
            {
                if(cycles == 0) //Mode 3 n'est pas r�gulier, ce if sert � temporiser
                {
                    if(dots == 80) //pixelcount == 0) //TODO: Add sprites, bg scrolling and window as causes of cycles
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
            if(pixelcount == 159)//ENTER HBLANCK:
            {
                mode = HBLANK;
                if((stat>>3)&1)
                    bus->write(0xFF0F,bus->read(0xFF0F)|2);
            }
        }

        dots++;
        if(dots == 456) //Can't change mode here (may still be in VBLANK)
        {
            dots = 0;
            scanline++;
            pixelcount = 0;
        }
        if(scanline == 144 && dots == 0) //ENTER VBLANK
        {
            mode = VBLANK;
            if((stat >> 4)&1) //Trigger VBLANK Interupt
            {
                bus->write(0xFF0F,bus->read(0xFF0F)|1);
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
            if(dots == 0 && (stat >> 6)&1) //Trigger LYC Interupt
            {
                bus->write(0xFF0F,bus->read(0xFF0F)|2);
            }
        }
        stat = (stat&0b1'1111'1'00) + mode;
    }
}

void PPU_Gameboy::generatePixel(uint8_t y, uint8_t x)
{
    if(lcdc & 1) //Draw the background if enabled
    {
        uint8_t pixelX = x + scx;
        uint8_t pixelY = y + scy;
        if(pixelX > 32*8) pixelX-= 32*8;
        if(pixelY > 32*8) pixelY-=32*8;
        uint8_t tileX = pixelX / 8;
        uint8_t tileY = pixelY / 8;
        uint8_t tileCode = read((tileY*32+tileX) + GetBGMapOffset());
        uint16_t line = GetTileLine(tileCode,(y+scy)%8);

        uint8_t inTileX = (x + scx) % 8;
        uint8_t upper = (line >> (7-inTileX))&1;
        uint8_t lower = ((line>>8) >> (7-inTileX))&1;
        uint8_t pixelData = lower + (upper<<1);
        uint8_t palette = bgp;
        UpdateScreen(pixelData,y,x,palette);
    }
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
    SDL_SetRenderDrawColor( ren, 0, 0, 0, SDL_ALPHA_OPAQUE );
    SDL_RenderClear(ren);

    /*for( unsigned int i = 0; i < 160*144; i++ )
        {
            const unsigned int x = rand() % 160;
            const unsigned int y = rand() % 144;

            const unsigned int offset = ( 160 * 4 * y ) + x * 4;
            screen[ offset + 0 ] = rand() % 256;        // b
            screen[ offset + 1 ] = rand() % 256;        // g
            screen[ offset + 2 ] = rand() % 256;        // r
            screen[ offset + 3 ] = SDL_ALPHA_OPAQUE;    // a
        }*/
    SDL_UpdateTexture(tex,NULL,&screen[0],160*4);

	SDL_RenderCopy(ren, tex, NULL, NULL);//&dst);

    SDL_RenderPresent(ren);
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
    return read(index) + (read(index+1)<<8);
}

uint16_t PPU_Gameboy::GetBGMapOffset()
{
    return (((lcdc >> 3)&1) ? 0x9C00 : 0x9800);//- 0x8000;
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
        lcdc = data; //std::cout<<"lcdc:"<<(int)lcdc<<"|"<<((lcdc>>7)&1)<<std::endl;break;
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
        return scy;
    case 0xFF43:
        return scx;
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
