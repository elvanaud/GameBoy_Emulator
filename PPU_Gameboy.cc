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

bool PPU_Gameboy::tick()
{
    //test();
    bool newFrame = false;
    if(((lcdc>>7)&1)==0) //TODO: manage this properly, and make it cycle accurate
    {
        stat = 0;
        scanline = 0;
        dots = 4;
        mode = 2;
        cycles = 0;
        pixelcount = 0;
    }
    else
    {
        if(mode != VBLANK)
        {
            if(mode == 2 || dots == 0)
            {
                mode = 2;
                if(mode == 2 && dots == 0) //Enter OAM scan
                {
                    if((stat >> 5)&1)
                        bus->write(0xFF0F,bus->read(0xFF0F)|2);
                    tileY = (((scy + scanline)) >> 3)%32; //TODO: debug this
                    tileYOffset = ((scy + scanline)) & 0x07;
                    tileX = scx >> 3;
                    tileXOffset = scx & 0x07;
                    ComputeTileLine();

                    //Preprocess sprites:
                    uint8_t oamLine[10][4] = {0};
                    int nbSpritesOnLine = 0;
                    uint8_t spriteHeight = 8;
                    bool y16Mode = ((lcdc>>2)&1) == 1;
                    if(y16Mode) //If 8*16 mode
                        spriteHeight = 16;
                    for(uint16_t objectIndex = 0xFE00; objectIndex < 0xFE9F; objectIndex+= 4) //Parcours l'oam
                    {
                        if(read(objectIndex) <= scanline+16 && scanline+16 < read(objectIndex)+spriteHeight)
                        {
                            for(uint16_t j = 0; j < 4; j++)
                                oamLine[nbSpritesOnLine][j] = read(objectIndex+j); //Load 10 first objects of the line
                            nbSpritesOnLine++;
                            if(nbSpritesOnLine == 10)
                                break;
                        }
                    }

                    memset(screenLine,4,160);
                    for(int i = 0; i < nbSpritesOnLine; i++)
                    {
                        uint8_t spriteY = scanline+16 - oamLine[i][0];
                        uint8_t charCode = oamLine[i][2];
                        if(y16Mode)
                        {
                            charCode &= (~1);
                            if(spriteY >= 8)
                            {
                                charCode |= 1;
                                spriteY -= 8;
                            }
                        }
                        if((oamLine[i][3]>>6)&1) //Y-flip ( TODO: verify 8*16 mode )
                        {
                            spriteY = 7 - spriteY;
                        }
                        uint16_t sprite = GetTileLineFromCode(charCode,spriteY,true);
                        bool drawAllSprite = false;
                        for(int j = (oamLine[i][1] < 8 ? (8-oamLine[i][1]) : 0); j < 8; j++) //Ternary here to have x sprite scroling
                        {
                            uint8_t spriteXDot= oamLine[i][1]+j-8; //-8 here for x scrolling
                            if(spriteXDot >= 160) break;
                            if(!drawAllSprite && screenLine[spriteXDot] == 4)
                            {
                                drawAllSprite = true;
                            }
                            if(drawAllSprite)
                            {
                                int spriteIndex = j;
                                if((oamLine[i][3] >> 5)&1) //X-flip
                                {
                                    spriteIndex = 7 - spriteIndex;
                                }
                                screenLine[spriteXDot] = ((sprite >> (15-spriteIndex))&1) | (((sprite >> (7-spriteIndex))<<1)&2);
                            }
                        }
                    }
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
                if(cycles == 0) //Mode 3 n'est pas régulier, ce if sert à temporiser
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
            //std::cout<<"VBLANK\n";
            mode = VBLANK;
            //if((stat >> 4)&1) //Trigger VBLANK Interupt
            {
                bus->write(0xFF0F,bus->read(0xFF0F)|1);
            }
            DrawScreen();
            newFrame = true;
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
    return newFrame;
}

void PPU_Gameboy::generatePixel(uint8_t y, uint8_t x)
{
    uint8_t pixelData = 0;
    //uint8_t palette = 0; //DEFAULT PALETTE: pixelData=0 => blank screen
    uint8_t palette = obp0;
    if(lcdc & 1) //Draw the background if enabled
    {
        pixelData = (lower_tile >> 7) | ((higher_tile >> 6)&2);
        lower_tile <<=1; higher_tile <<=1;
        tileXOffset++;
        if(tileXOffset == 8)
        {
            tileXOffset = 0;
            tileX++;
            tileX &= 0x3F; //Modulo for scrolling
            ComputeTileLine();
        }

        palette = bgp;

    }
    if(lcdc & 2) //Display Sprites
    {
        if(screenLine[x] != 4)
        {
            //std::cout << (int) screenLine[x] << std::endl;
            pixelData = screenLine[x];
            //palette = //BIG PROBLEM: TODO: encode palette info in upper bits
        }
    }
    UpdateScreen(pixelData,y,x,palette);
}

void PPU_Gameboy::ComputeTileLine()
{
    uint8_t tileCode = read((tileY*32+tileX) + GetBGMapOffset());
    uint16_t line = GetTileLineFromCode(tileCode,tileYOffset);
    lower_tile = line>>8;
    higher_tile = line & 0xFF;
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
    SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE );
    SDL_RenderClear(ren);
    /*for(int i = 0; i < 2; i++)
    {
        for(int j=0; j < 160; j++)
        {
            screen[(i*160+j)*4+0]=255;
        }
    }*/

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

uint16_t PPU_Gameboy::GetTileLineFromCode(uint8_t tileCode,uint8_t line, bool spriteMode)
{
    uint16_t index = tileCode*16 + 2*line;
    if(((lcdc >> 4)&1)==1 || spriteMode)
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
            index += 0x9000;
        }
    }
    return read(index) + (read(index+1)<<8);
}

uint16_t PPU_Gameboy::GetBGMapOffset()
{
    return (((lcdc >> 3)&1) ? 0x9C00 : 0x9800);//- 0x8000;
}

void PPU_Gameboy::DMA_Transfer() //TODO: wait 160 cycles
{
    uint16_t starting_adress = ((uint16_t)dma_starting_adress)<<8;
    for(uint16_t i = 0xFE00; i <= 0xFE9F;i++) //Exactly 160 iterations... => manage cycle count with this..
    {
        write(i,bus->read(starting_adress));
        starting_adress++;
    }
}

void PPU_Gameboy::write(uint16_t adr, uint8_t data)
{
    if(adr >= 0x8000 && adr <= 0x9FFF)
    {
        vram[adr-0x8000] = data;
        return;
    }
    if(adr >= 0xFE00 && adr <= 0xFE9F)
    {
        oam[adr-0xFE00] = data;
        return;
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
    case 0xFF46: //DMA Transfer
        dma_starting_adress = data;
        DMA_Transfer(); break;
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
    if(adr >= 0xFE00 && adr <= 0xFE9F)
        return oam[adr-0xFE00];
    switch(adr)
    {
    case 0xFF40:
        return lcdc;
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
    case 0xFF46:
        return dma_starting_adress;
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
