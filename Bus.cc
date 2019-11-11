#include "Bus.h"
#include "Z80_Gameboy.h"
#include "PPU_Gameboy.h"
#include <iostream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <vector>


Bus::Bus(Z80_Gameboy & c, PPU_Gameboy & p):cpu(c),ppu(p)
{
    cpu.attachBus(this);
    ppu.attachBus(this);

    app.create(sf::VideoMode(640,480,32),"GameBoy Emulator");
    //for(uint16_t i = 0; i < 0x100;i++)
        //ram[i] = 0;
}

void Bus::loadCartridge(std::string path)
{
    std::ifstream input(path, std::ios::binary );
    if(!input) std::cout << "ERREUR: FIchier ROM introuvable\n";

    input.read((char*)ram,0x8000);
}

void Bus::write(uint16_t adr, uint8_t data)
{
    if(blockMemoryWrite) return;
    if(adr >= 0x0000 && adr <= 0x7FFF) //ROM
    {
        //cartridge.write(adr,data);
        ram[adr] = data;
    }
    else if(adr >= 0x8000 && adr <= 0x9FFF) //VRAM
    {
        ppu.write(adr,data);
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        //cartridge.write(adr,data);
        ram[adr] = data;
    }
    else if(adr >= 0xC000 && adr <= 0xDFFF) //WRAM
    {
        ram[adr] = data;
    }
    else if(adr >= 0xE000 && adr <= 0xFDFF) //Mirror WRAM
    {
        ram[adr-0xE000] = data;
    }
    else if(adr >= 0xFE00 && adr <= 0xFE9F) //OAM
    {
        ppu.write(adr,data);
    }
    else if(adr >= 0xFEA0 && adr <= 0xFEFF) //NOT USED
    {
        ram[adr] = data; //Quand meme au cas ou
    }
    else if(adr >= 0xFF00 && adr <= 0xFF7F) //IO REGISTERS
    {
        if(adr == 0xFF01)
        {
            sb = data;
        }
        if(adr == 0xFF02)
        {
            sc = data;
        }
        if(adr >= 0xFF40 && adr <= 0xFF4B)
            ppu.write(adr,data);
        if(adr == 0xFF0F)
            if_reg = data;
    }
    else if(adr >= 0xFF80 && adr <= 0xFFEF) //HRAM
    {
        ram[adr] = data;
    }
    else if(adr == 0xFFFF)
        ie = data;//Interrupts Enable Register
    else
    {
        ram[adr] = data;
    }
}

uint8_t Bus::read(uint16_t adr)
{
    //std::cout << "bus_read\n";
    if(adr <= 0x7FFF) //ROM
    {

    }
    else if(adr >= 0x8000 && adr <= 0x9FFF) //VRAM
    {
        //std::cout << "youpi" << std::endl;
        return ppu.read(adr);
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {

    }
    else if(adr >= 0xC000 && adr <= 0xDFFF) //WRAM
    {
        if(adr == 0xD800)//TODO:enlever ca (test)
        {
            //return 0xff;
        }
    }
    else if(adr >= 0xE000 && adr <= 0xFDFF) //Mirror WRAM
    {

    }
    else if(adr >= 0xFE00 && adr <= 0xFE9F) //OAM
    {
        return ppu.read(adr);
    }
    else if(adr >= 0xFEA0 && adr <= 0xFEFF) //NOT USED
    {

    }
    else if(adr >= 0xFF00 && adr <= 0xFF7F) //IO REGISTERS
    {
        if(adr == 0xFF01)
        {
           return sb;
        }
        if(adr == 0xFF02)
        {
            return sc;
        }
        if(adr >= 0xFF40 && adr <= 0xFF4B)
            return ppu.read(adr);
        if(adr == 0xFF0F)
            return if_reg;
    }
    else if(adr >= 0xFF80 && adr <= 0xFFEF) //HRAM
    {

    }
    if(adr == 0xFFFF)
        return ie;//Interrupts Enable Register
    return ram[adr];
}

void Bus::triggerInterupt(uint8_t interupt)
{
    cpu.Trigger_Interupt(interupt);
}

void Bus::triggerStopMode(bool stop)
{
    stopMode = stop;
}

void Bus::disassemble()
{
    cpu.disassemble = true;
    cpu.setPC(0x0);
    //Block access to memory:
    blockMemoryWrite = true;
    while(!cpu.disassembleOver)
        cpu.tick();
    for(int i = 0; i < 0x8000;i++)
    {
        asmArray[i] = cpu.genAsm[i];//useless right now(below code directly uses cpu.genAsm)
    }
    cpu.reset();
    blockMemoryWrite = false;
    stopMode = false;
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}

std::string csvGet(std::string src,std::string field)
{
    auto parts = split(src,'\n');
    int pos =0; int p = 0;
    for(f : split(parts[0],';'))
    {
        if(f==field)
            pos = p;
        p++;
    }
    return split(parts[1],';')[pos];
}

void Bus::run()
{
    bool notOver = true;
    uint8_t cpt = 4;
    bool stepping = true;
    bool step = true;
    bool debug = true;
    uint8_t instcycles = 0;
    bool watch = false;
    std::vector<uint16_t> watchAdr;
    bool breakpointEnable = false;
    uint16_t breakpoint = 0; //adr 0 never executed (at least i hope)
    bool memoryBreakPointEnable = false;
    uint16_t memBpAdr = 0;
    uint8_t memBpVal = 0;
    bool triggerAsm = false;
    std::string dump = "";
    int instNb = 0;
    int clkTotal=0;
    std::vector<uint8_t> dmp;
    //testProgram();
    while (/*notOver || */app.isOpen())
    {
        // on inspecte tous les évènements de la fenêtre qui ont été émis depuis la précédente itération
        sf::Event event;
        while (app.pollEvent(event))
        {
            // évènement "fermeture demandée" : on ferme la fenêtre
            if (event.type == sf::Event::Closed)
            {
                app.close();
                notOver = false;
            }
            if(event.type == sf::Event::KeyPressed)
            {
                if(event.key.code == sf::Keyboard::S)//enable stepping
                {
                    stepping = true;
                    step = true;
                }
                if(event.key.code == sf::Keyboard::C) //continue
                {
                    stepping = false;
                }
                if(event.key.code == sf::Keyboard::D) //debug
                {
                    debug = !debug;
                }
                if(event.key.code == sf::Keyboard::W)
                {
                    //Watch memory
                    watch = true;
                }
                if(event.key.code == sf::Keyboard::B)
                {
                    breakpointEnable = true;//Toggle breakpoint
                }
                if(event.key.code == sf::Keyboard::M)
                {
                    memoryBreakPointEnable = true;//Toggle breakpoint
                    std::cout << "Toggle memory breakpoint - adr:";
                    uint16_t user_entry;
                    std::cin >> std::hex >> user_entry;
                    //Single breakpoint for now:
                    if(memBpAdr == user_entry)
                    {
                        memBpAdr = 0; //disable (never used for code)
                        //stepping = false; //todo: change behavior
                        memoryBreakPointEnable = false;
                    }
                    else
                        memBpAdr = user_entry;
                    std::cout << "\nValue:";
                    //std::cin.ignore();
                    std::cin>>std::hex>>user_entry;//works on uint16_t
                    memBpVal = user_entry;
                }
                if(event.key.code == sf::Keyboard::A)
                {
                    //trigger asm [-10;+10]
                    //use array 32kb: adr = index
                    triggerAsm = !triggerAsm;
                    //std::cout << showMemory(0x100,0x110);

                    //genAsm(asmArray); //already done(in construction)
                }
            }
        }

        if(!stopMode && (!stepping||instcycles!=0||step))
        {
            if(cpt == 4)
            {
                cpt = 0;
                if(instcycles==0)
                {
                    /*dump+=csvGet(cpu.trace(),"flags")+";";
                    dump+=csvGet(cpu.trace(true,true,true),"A")+";";
                    dump+=csvGet(cpu.trace(true,true,true),"HL")+";";
                    dump+=csvGet(cpu.trace(true,true,true),"DE")+";";
                    dump+=csvGet(cpu.trace(true,true,true),"BC")+";";
                    dump+=csvGet(cpu.trace(true,true,true),"SP")+";";*/

                    cpu.binaryDump(dmp);
                    //auto myfile = std::fstream("trace.log", std::ios::out | std::ios::binary);
                    //myfile.write((char*)&data[0], bytes);

                }
                instcycles = cpu.tick();
                step = false;
                if(cpu.getPC() == breakpoint&&instcycles==0)
                    stepping = true; //step = true
                if(memoryBreakPointEnable&&instcycles==0 && read(memBpAdr)==memBpVal)
                    stepping = true;
                /*if(instcycles==0)
                {
                    //dump+=csvGet(cpu.trace(true,true),"flags")+";";
                    //dump+=csvGet(cpu.trace(true,true),"A")+";";
                    dump+=cpu.instDump()+"|"+csvGet(cpu.trace(),"assembly")+"\n";
                    instNb++;
                }*/

                if(debug&&instcycles==0)
                {
                    std::cout << cpu.trace() << std::endl<<cpu.instDump()<<std::endl;
                    //std::cout << showMemory(0x100-1,0x100+10) << std::endl;
                    //std::cout << showMemory(0x300-1,0x300 + 11) << std::endl;
                    std::cout << "--------------------------------------" << std::endl;
                    for(a : watchAdr)
                    {
                        std::cout << std::hex << std::showbase << a << ": " << (int)read(a) << std::endl;
                    }
                    std::cout << "Breakpoint:" << breakpoint << std::endl;
                    if(memoryBreakPointEnable)
                        std::cout << "Memory breakpoint enabled on ("<<memBpAdr<<")="<<std::hex<<(int)memBpVal<<std::endl;
                    if(triggerAsm)
                    {
                        if(cpu.getPC()+10<=0x8000)
                            for(uint16_t i = cpu.getPC()-6; i < cpu.getPC()+(uint16_t)10;i++)
                            {
                                if(cpu.genAsm[i] != "")
                                    std::cout<<std::hex<<std::showbase<< (i == cpu.getPC()?"> ":"  ") <<i<<": "<<cpu.genAsm[i]<<std::endl;
                            }
                        else
                        {
                            //Dynamically compute it:
                            //disassemble();
                            //patate();
                        }
                    }
                }

                if(sc == 0x81) //read serial cable msg from test rom
                {
                    msg+=(char) sb;
                    std::cout << "Serial Cable Char:"<<(int)sb<<" ("<<(char)sb<<")"<<std::endl;
                    sb = sc = 0;
                }
            }
            ppu.tick();
            cpt++;
            clkTotal++;
        }
        if(stopMode)
        {
            std::cout<<"STOP";app.close(); //todo: trigger on interupt from p10-...
        }
        if(watch)
        {
            std::cout << "Watch Memory address:";
            uint16_t user_entry;
            std::cin >> std::hex >> user_entry;
            watchAdr.push_back(user_entry);
            watch = false;
            std::cout << std::hex << user_entry << ": " << (int)read(user_entry) << std::endl;
        }
        if(breakpointEnable)
        {
            std::cout << "Toggle breakpoint:";
            uint16_t user_entry;
            std::cin >> std::hex >> user_entry;
            //Single breakpoint for now:
            if(breakpoint == user_entry)
            {
                breakpoint = 0; //disable (never used for code)
                stepping = false; //todo: change behavior
            }
            else
                breakpoint = user_entry;
            breakpointEnable = false;
        }
        /*if(memoryBreakPointEnable)
        {
            std::cout << "Toggle memory breakpoint - adr:";
            uint16_t user_entry;
            std::cin >> std::hex >> user_entry;
            //Single breakpoint for now:
            if(memBpAdr == user_entry)
            {
                memBpAdr = 0; //disable (never used for code)
                //stepping = false; //todo: change behavior
            }
            else
                memBpAdr = user_entry;
            std::cout << "\nValue:";
            std::cin>>std::hex>>memBpVal;
            memoryBreakPointEnable = false;
        }*/

    }
    std::cout <<"Result:"<< msg << std::endl;
    //std::cout <<dump;
    //std::ofstream of;
    //of.open("trace.log");
    //of<<dump;
    //of.close();

    auto myfile = std::fstream("trace.log", std::ios::out | std::ios::binary);
    myfile.write((char*)dmp.data(),dmp.size()*sizeof(uint8_t));
    /*for(auto b : dmp)
    {
        myfile.write(&b, 4);
    }*/

}
