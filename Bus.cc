#include "Bus.h"
#include "Z80_Gameboy.h"
#include "PPU_Gameboy.h"
#include "Timer_Gameboy.h"
#include <iostream>
#include <fstream>
//#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <list>
#include "NoMBC.h"
#include "MBC1.h"

Bus::Bus(Z80_Gameboy & c, PPU_Gameboy & p,Timer_Gameboy & t)
    :cpu(c),ppu(p),tim(t)
{
    cpu.attachBus(this);
    tim.attachBus(this);

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "BIGPROBLEM\n";
    }
    win = SDL_CreateWindow("GameBoy Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 576, SDL_WINDOW_SHOWN);
    if (win == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED );
    if (ren == nullptr)
    {
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }
    ppu.attachBus(this,ren);

    for(int i = 0; i < Cont_KEYNB; i++) controller_keys_state[i] = Sig_UP;
}

Bus::~Bus()
{
    if(cartridge)
        delete cartridge;
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

void Bus::loadCartridge(std::string path)
{
    std::ifstream input(path, std::ios::binary );
    if(!input) std::cout << "ERREUR: Fichier ROM introuvable\n";

    char romHeader[0x150];
    input.read(romHeader,0x150);
    uint8_t romType = romHeader[0x0147];
    std::cout << "Type: " << (int)romType << " ROM: " << (int)romHeader[0x148] << " RAM: " << (int)romHeader[0x149] << std::endl;
    input.seekg(0,std::ios::beg);
    switch(romType)
    {
    case 0:
        cartridge = new NoMBC(input);
        break;
    case 1: case 2: case 3:
        cartridge = new MBC1(input);
        break;
    default:
        std::cout << "ERROR: MBC Type "<<(int)romType<<" not handled !\n";
        cartridge = new NoMBC(input); //By default, TODO: stop execution
    }
}

void Bus::write(uint16_t adr, uint8_t data)
{
    if(blockMemoryWrite) return;
    if(adr >= 0x0000 && adr <= 0x7FFF) //ROM
    {
        cartridge->write(adr,data);
    }
    else if(adr >= 0x8000 && adr <= 0x9FFF) //VRAM
    {
        ppu.write(adr,data);
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        cartridge->write(adr,data);
    }
    else if(adr >= 0xC000 && adr <= 0xDFFF) //WRAM
    {
        ram[adr] = data;
    }
    else if(adr >= 0xE000 && adr <= 0xFDFF) //Mirror WRAM
    {
        ram[adr-0xE000+0xC000] = data;
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
        if(adr == 0xFF00)
        {
            //controller_keys_state[Cont_START] = Sig_DOWN;
            uint8_t base = 0b10'00'0000;
            uint8_t keys = 0b0000'0000;
            int mask = 1;
            Controller_Key starting_key = Cont_A;
            if(((data >> 5)&1) == 0);//P15 selected: General Control Buttons
            else if(((data >> 4)&1) == 0) //P14 selected: Cross Dir
            {
                starting_key = Cont_RIGHT;
            }
            for(int i = starting_key; i < starting_key+4; i++)
            {
                if(controller_keys_state[i] == Sig_UP)
                    keys |= mask;
                mask <<= 1;
            }
            controller_reg = base | (data & 0b00'11'0000) | keys;
        }
        if(adr == 0xFF01)
        {
            sb = data;
        }
        if(adr == 0xFF02)
        {
            sc = data;
        }
        if(adr >= 0xFF04 && adr <= 0xFF07)
        {
            tim.write(adr,data);
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
    if(adr <= 0x7FFF) //ROM
    {
        return cartridge->read(adr);
    }
    else if(adr >= 0x8000 && adr <= 0x9FFF) //VRAM
    {
        //std::cout << "youpi" << std::endl;
        return ppu.read(adr);
    }
    else if(adr >= 0xA000 && adr <= 0xBFFF) //ExtRAM
    {
        return cartridge->read(adr);
    }
    else if(adr >= 0xC000 && adr <= 0xDFFF) //WRAM
    {

    }
    else if(adr >= 0xE000 && adr <= 0xFDFF) //Mirror WRAM
    {
        return ram[adr-0xE000+0xC000];
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
        if(adr == 0xFF00)
        {
            return controller_reg;
        }
        if(adr == 0xFF01)
        {
           return sb;
        }
        if(adr == 0xFF02)
        {
            return sc;
        }
        if(adr >= 0xFF04 && adr <= 0xFF07)
        {
            return tim.read(adr);
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
        //asmArray[i] = cpu.genAsm[i];//useless right now(below code directly uses cpu.genAsm)
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
    uint8_t cpt = 4;
    bool stepping = true;
    bool step = true;
    bool debug = stepping;
    uint8_t instcycles = 0;
    std::vector<uint16_t> watchAdr;
    bool breakpointEnable = false;
    uint16_t breakpoint = 0; //adr 0 never executed (at least i hope)
    bool memoryBreakPointEnable = false;
    uint16_t memBpAdr = 0;
    uint8_t memBpVal = 0;
    bool triggerAsm = false;
    std::string dump = "";
    std::vector<uint8_t> dmp;

    bool over = false;
    uint64_t nbInst = 0;
    uint64_t nbCycles = 0;
    clock_t startTime = clock();
    SDL_Event e;
    bool newFrame = true;
    int fixedDumpEnabled = 0;
    std::list<std::string> fixedDump;
    bool  dump_membp = false;

    while (!over)
    {
        //nbCycles++;
        if(newFrame && !debug) //TODO: change the condition to also check this when screen is disabled
        {
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    over = true;
                }
                else if(e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
                {
                    //Controls:
                    //std::vector<SDL_Keycode> controller_keys = {SDLK_DOWN,SDLK_UP,SDLK_RIGHT,SDLK_LEFT,SDLK_SPACE,SDLK_RETURN,SDLK_a,SDLK_z,SDLK_a};
                    std::map<SDL_EventType,Controller_Signal> sdl_signal;
                    sdl_signal[SDL_KEYDOWN] = Sig_DOWN;
                    sdl_signal[SDL_KEYUP] = Sig_UP;
                    std::map<SDL_Keycode,Controller_Key> controller_keys = {{SDLK_DOWN,Cont_DOWN},{SDLK_UP,Cont_UP},{SDLK_RIGHT,Cont_RIGHT},
                        {SDLK_LEFT,Cont_LEFT},{SDLK_SPACE,Cont_SELECT},{SDLK_RETURN,Cont_START},{SDLK_a,Cont_A},{SDLK_z,Cont_B}};
                    if(controller_keys.count(e.key.keysym.sym) == 1)
                    {
                        Controller_Signal signal = sdl_signal[(SDL_EventType)e.type];
                        if(controller_keys_state[controller_keys[e.key.keysym.sym]] == Sig_UP && signal == Sig_DOWN)
                        {
                            write(0xFF0F,read(0xFF0F)|16);
                            stopMode = false;
                        }
                        controller_keys_state[controller_keys[e.key.keysym.sym]] = signal;
                        //controller_keys_state[Cont_START] = Sig_DOWN;
                    }
                }
                if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_d) //enable debug mode
                {
                    debug = !debug;
                }

            }
        }
        else if(debug || stepping) //TODO: verify all that behavior
        {
            while(SDL_PollEvent(&e)) //Debug events
            {
                if(e.type == SDL_QUIT)
                {
                    over = true;
                }
                else if (e.type == SDL_KEYDOWN)// || e.type == SDL_KEYUP)
                {
                    if(e.key.keysym.sym == SDLK_d) //debug
                    {
                        debug = !debug;
                    }
                    if(e.key.keysym.sym == SDLK_s)//enable stepping
                    {
                        stepping = true;
                        step = true;
                    }
                    if(e.key.keysym.sym == SDLK_c) //continue
                    {
                        stepping = false;
                    }

                    if(e.key.keysym.sym == SDLK_w)
                    {
                        //Watch memory
                        std::cout << "Watch Memory address:";
                        uint16_t user_entry;
                        std::cin >> std::hex >> user_entry;
                        std::cin.ignore();
                        watchAdr.push_back(user_entry);
                        std::cout << std::hex << user_entry << ": " << (int)read(user_entry) << std::endl;
                    }
                    if(e.key.keysym.sym == SDLK_b)
                    {

                        breakpointEnable = true;//Toggle breakpoint
                        std::cout << "Toggle breakpoint:";
                        uint16_t user_entry;
                        std::cin >> std::hex >> user_entry;
                        std::cin.ignore();
                        //Single breakpoint for now:
                        if(breakpoint == user_entry)
                        {
                            breakpoint = 0; //disable (never used for code)
                            stepping = false; //todo: change behavior
                            breakpointEnable = false;
                        }
                        else
                            breakpoint = user_entry;
                    }

                    if(e.key.keysym.sym == SDLK_m)
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
                        std::cin.ignore();
                        memBpVal = user_entry;
                    }
                    if(debug && e.key.keysym.sym == SDLK_a)
                    {
                        //trigger asm [-10;+10]
                        //use array 32kb: adr = index
                        triggerAsm = !triggerAsm;
                        //std::cout << showMemory(0x100,0x110);

                        //genAsm(asmArray); //already done(in construction)
                    }
                    if(debug && e.key.keysym.sym == SDLK_v)
                    {
                        std::cout << "Type your command: ";
                        std::string user_entry;
                        //std::cin >> user_entry;
                        std::getline(std::cin, user_entry);
                        auto command = split(user_entry, ' ');
                        if(command[0] == "dump")
                        {
                            if(command[1] == "add")
                            {
                                if(command[2] == "memory_breakpoint")
                                {
                                    dump_membp = true;
                                }
                            }
                            else
                            {
                                fixedDumpEnabled = std::stoi(command[1]);
                                fixedDump.clear();
                            }

                        }
                        else if(command[0] == "show")
                        {
                            if(command[1] == "dump")
                            {
                                for(auto line : fixedDump)
                                {
                                    std::cout << line << std::endl;
                                }
                            }
                        }
                    }
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
                    //dump+=cpu.trace(false);
                    //cpu.binaryDump(dmp);
                    nbInst++;
                    if(fixedDumpEnabled != 0)
                    {
                        std::stringstream line; line << cpu.trace(false);
                        if(dump_membp)
                        {
                            line << ";  [" << memBpAdr << "] = " << (int)read(memBpAdr);
                        }
                        fixedDump.push_back(line.str());

                        if(fixedDumpEnabled == fixedDump.size())
                            fixedDump.pop_front();
                    }
                    //if(nbInst >= 0x3e0000) over = true;
                    /*if(ram[0x40] == 0x2F)
                    {
                        std::cout << "wtf ------"<<nbInst<<"----------\n";
                        stepping = true;
                    }*/
                }

                instcycles = cpu.tick();

                /*if(nbInst == 0x477ed0) //973dc0 //depends on speed to press buttons...
                {
                    std::cout<<"sah\n";
                    stepping = true;
                }*/

                step = false;
                if(breakpointEnable&&instcycles==0&&cpu.getPC() == breakpoint)
                    {stepping = true; std::cout << "Breakpoint hit !\n";}
                if(memoryBreakPointEnable&&instcycles==0 && read(memBpAdr)==memBpVal)
                    {stepping = true; std::cout << "Memory Breakpoint hit !\n";}
                if(debug&&instcycles==0)
                {
                    std::cout << cpu.trace() << std::endl<<cpu.instDump()<<std::endl;
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
                        }
                    }
                }

                if(sc == 0x81) //read serial cable msg from test rom
                {
                    msg+=(char) sb;
                    if(msg=="Passed" || (char)sb == 'P' || (char)sb == 'F')
                        ;//app.close();
                    std::cout << "Serial Cable Char:"<<(int)sb<<" ("<<(char)sb<<")"<<std::endl;
                    sb = sc = 0;
                }
                SDL_Delay(0.001);

            }
            newFrame = ppu.tick();
            tim.tick();
            cpt++;
        }
        if(stopMode)
        {
            std::cout<<"STOP";
            stopMode = false;
            stepping = true;
            debug = true;
            //over = true; //todo: trigger on interupt from p10-...
        }
    }
    clock_t endTime = clock();
    double duration = ((double)(endTime-startTime)) / CLOCKS_PER_SEC;
    std::cout << "\nTime:" << duration << " Nb inst: " << nbInst << " Nb cycles:" << nbCycles << "\n";

    std::cout <<"Result:"<< msg << std::endl;
    //std::cout <<dump;
    /*std::ofstream of;
    of.open("tracetxt.log");
    of<<dump;
    of.close();
*/
    //auto myfile = std::fstream("trace.log", std::ios::out | std::ios::binary);
    //myfile.write((char*)dmp.data(),dmp.size()*sizeof(uint8_t));
}
