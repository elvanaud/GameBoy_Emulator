#include "Z80_Gameboy.h"
#include "Bus.h"
#include <iomanip>

Z80_Gameboy::Z80_Gameboy()
{
    reset();
    masks[0] = 0b11'111'111; //No param
    masks[1] = 0b11'001'111; //Reg middle 2-bit left
    masks[2] = 0b11'111'000; //Reg right
    masks[3] = 0b11'000'111; //Reg middle
    masks[4] = 0b11'000'000; //Reg both

    instructions[0] = {
        {0b00'110'110,&LD_iHL_N},
        {0b00'001'010,&LD_A_iBC},
        {0b00'011'010,&LD_A_iDE},
        {0b11'110'010,&LDH_A_iC},
        {0b11'100'010,&LDH_iC_A},
        {0b11'110'000,&LDH_A_iN},
        {0b11'100'000,&LDH_iN_A},
        {0b11'111'010,&LD_A_iNN},
        {0b11'101'010,&LD_iNN_A},
        {0b00'101'010,&LDI_A_iHL},
        {0b00'111'010,&LDD_A_iHL},
        {0b00'000'010,&LD_iBC_A},
        {0b00'010'010,&LD_iDE_A},
        {0b00'100'010,&LDI_iHL_A},
        {0b00'110'010,&LDD_iHL_A},
        {0b11'111'001,&LD_SP_HL},
        {0b11'111'000,&LDHL_SP_E}, //carefull that one (test important)
        {0b00'001'000,&LD_iNN_SP},
        {0b11'000'110,&ADD_A_N},
        {0b10'000'110,&ADD_A_iHL},
        {0b11'001'110,&ADC_A_N},
        {0b10'001'110,&ADC_A_iHL},
        {0b11'010'110,&SUB_N},
        {0b10'010'110,&SUB_iHL},
        {0b11'011'110,&SBC_A_N},
        {0b10'011'110,&SBC_A_iHL},
        {0b11'100'110,&AND_N},
        {0b10'100'110,&AND_iHL},
        {0b11'110'110,&OR_N},
        {0b10'110'110,&OR_iHL},
        {0b11'101'110,&XOR_N},
        {0b10'101'110,&XOR_iHL},
        {0b11'111'110,&CP_N},
        {0b10'111'110,&CP_iHL},
        {0b00'110'100,&INC_iHL},
        {0b00'110'101,&DEC_iHL},
        {0b11'101'000,&ADD_SP_E},
        {0b00'000'111,&RLCA},
        {0b00'010'111,&RLA},
        {0b00'001'111,&RRCA},
        {0b00'011'111,&RRA},
        {0b11'001'011,&META_ROTATE},
        {0b11'000'011,&JP_NN},
        {0b00'011'000,&JR_E},
        {0b11'101'001,&JP_iHL},
        {0b11'001'101,&CALL_NN},
        {0b11'001'001,&RET},
        {0b11'011'001,&RETI},
        {0b00'100'111,&DAA},
        {0b00'101'111,&CPL},
        {0b00'000'000,&NOP},
        {0b00'111'111,&CCF},
        {0b00'110'111,&SCF},
        {0b11'110'011,&DI},
        {0b11'111'011,&EI},
        {0b01'110'110,&HALT}, //TODO
        {0b00'010'000,&STOP} //TODO (and careful opcode)
    };

    instructions[1] = {
        {0b00'000'001,&LD_DD_NN},
        {0b11'000'101,&PUSH_QQ},
        {0b11'000'001,&POP_QQ},
        {0b00'001'001,&ADD_HL_SS},
        {0b00'000'011,&INC_SS},
        {0b00'001'011,&DEC_SS}
    };

    instructions[2] = {
        {0b01'110'000,&LD_iHL_R},
        {0b10'000'000,&ADD_A_R},
        {0b10'001'000,&ADC_A_R},
        {0b10'010'000,&SUB_R},
        {0b10'011'000,&SBC_A_R},
        {0b10'100'000,&AND_R},
        {0b10'110'000,&OR_R},
        {0b10'101'000,&XOR_R},
        {0b10'111'000,&CP_R}
    };

    instructions[3] = {
        {0b00'000'110,&LD_R_N},
        {0b01'000'110,&LD_R_iHL},
        {0b00'000'100,&INC_R},
        {0b00'000'101,&DEC_R},
        {0b11'000'010,&JP_CC_NN},
        {0b00'000'000,&JR_CC_E},
        {0b11'000'100,&CALL_CC_NN},
        {0b11'000'000,&RET_CC},
        {0b11'000'111,&RST_T}
    };

    instructions[4] = {
        {0b01'000'000,&LD_R_Rp}
    };
}

uint8_t Z80_Gameboy::read(uint16_t adr)
{
    return bus->read(adr);
}

void Z80_Gameboy::write(uint16_t adr,uint8_t data)
{
    bus->write(adr,data);
}

uint16_t Z80_Gameboy::GetRegPair(uint8_t r,bool accFlagsMode)
{
    if(r==3)
    {
        if(accFlagsMode)
            return (((uint16_t)GetReg(Registers::A))<<8)+flags;
        return sp;
    }
    return (((uint16_t) regs[2*r]) << 8) + regs[2*r+1];
}

void Z80_Gameboy::SetRegPair(uint8_t r, uint16_t val,bool accFlagsMode)
{
    if(r == 3)
    {
        if(accFlagsMode)
        {
            SetReg(Registers::A,val>>8);
            flags = val & 0b1111'0000;
            return;
        }
        sp = val;
        return;
    }
    uint8_t low = val & 0b11'111'111;
    uint8_t high = val >> 8;
    regs[r*2] = high;
    regs[2*r+1] = low;
}

void Z80_Gameboy::triggerStopMode(bool stop)
{
    bus->triggerStopMode(stop);
}

std::string Z80_Gameboy::instDump(bool withPC)
{
    std::stringstream ss;
    ss<<std::hex;
    if(withPC)
        ss <<"0x"<<std::setfill ('0') << std::setw (4)<<fromPC<<":"<<std::noshowbase;
    for(uint16_t i = fromPC; i < fromPC+inst_length; i++)
        ss<< " "<<std::setfill ('0') << std::setw (2)<<(int)read(i);
    return ss.str();
}

uint8_t Z80_Gameboy::tick()
{
    if(cycles == 0)
    {
        oldFlags = flags;
        fromPC = pc;
        opcode = read(pc);
        //std::cout<<(int)opcode<<std::endl;
        if(disassemble)
        {
            pc = priv_pc;
            opcode = read(priv_pc);
        }

        for(int mask_type = 0; mask_type < MASK_TYPE_NB; mask_type++)
        {
            uint8_t masked_opcode = opcode & masks[mask_type];
            for(auto inst : instructions[mask_type])
            {
                if(inst.first == masked_opcode)
                {
                    inst.second(this);//executes the instruction
                    cyclesInst = cycles;
                    goto after_exec;
                }
            }
        }
        //If no instruction is found:
        pc++; //try next one //TODO: Generate error maybe ?
        priv_pc++;//Just in case
        std::cout <<"ERREUR MONUMENTALE:NO INST FOUND:"<<opcode<<"\n---------------------------------\n";
        after_exec:

        pc += inst_length;

        if(disassemble)
        {
            std::stringstream ss;
            if(disassembleAdr)
                ss<<std::hex<<std::showbase<<priv_pc<<": ";
            ss<<assembled_line<<std::endl;
            assembled += ss.str();
            genAsm[priv_pc] = ss.str();

            priv_pc += inst_length;
            if(priv_pc >= 0x8000)
                disassembleOver = true;
        }
    }

    cycles--;
    return cycles;
}

void Z80_Gameboy::reset()
{
    pc = 0x100;  fromPC = pc;
    cycles = 0; cyclesInst = 0; //A cycle counter and a save for each inst
    opcode = 0;
    inst_length = 0;
    //sp = 0;
    ime = 0;
    flags = oldFlags= 0xb0; //High bits: Z-N-H-CY
    for(uint8_t i = 0; i < 8; i++) regs[i] = 0x01;//=0;//TODO:repare that (weird emulator comp)(value a for cgb)
    //init regs:
    SetRegPair(Registers_Pairs::BC,0x0013);
    SetRegPair(Registers_Pairs::DE,0x00D8);
    SetRegPair(Registers_Pairs::HL,0x014D);
    sp = 0xFFFE;
    //assembled = "";
    disassemble = false;
    disassembleOver = false;
    //std::string genAsm[0x8000];
    disassembleAdr = false;
}

uint8_t Z80_Gameboy::GetReg(uint8_t r)
{
    return regs[r];
}

void Z80_Gameboy::SetReg(uint8_t r, uint8_t val)
{
    regs[r] = val;
}
