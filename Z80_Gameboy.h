#ifndef Z80_GAMEBOY_H
#define Z80_GAMEBOY_H

#include <cstdint>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>
#include <bitset>
#include <iomanip>

class Bus;

class Z80_Gameboy
{
private:
    uint16_t pc = 0x100; uint16_t fromPC = pc;
    uint8_t cycles = 0; uint8_t cyclesInst = 0; //A cycle counter and a save for each inst
    Bus* bus;
    uint8_t opcode;
    uint8_t inst_length;
    enum Registers{B=0,C=1,D=2,E=3,H=4,L=5,A=7};
    enum Registers_Pairs{BC=0,DE=1,HL=2,SP=3};
    uint16_t sp;
    uint8_t ime;
    uint8_t flags=0xb0; uint8_t oldFlags = 0xb0;//High bits: Z-N-H-CY
    uint8_t regs[8];
    static const uint8_t MASK_TYPE_NB = 5;
    uint8_t masks[MASK_TYPE_NB];
    std::vector<std::pair<uint8_t,std::function<void(Z80_Gameboy*)>>> instructions[MASK_TYPE_NB];
    uint8_t GetOpcodeMiddle()
    {
        return (opcode >> 3) & 0b00'000'111;
    }
    uint8_t GetOpcodeRight()
    {
        return opcode & 0b00'000'111;
    }

    uint8_t CarryFromBit(uint16_t a,uint16_t b,uint16_t res,uint8_t bit)
    {
        uint16_t mask = 1 << bit;
        a &= mask;
        b &= mask;
        res &= mask;
        if(a & b)
            return 1;
        if(a^b)
            return ((~res)>>bit)&1;
        return 0;
    }

    //TODO:ABSOLUMENT TESTER CA JE SUIS PAS SUR DU TOUT
    uint8_t BorrowFromBit(uint16_t a, uint16_t b, uint16_t res, uint8_t bit)
    {
        //a-b=res
        uint16_t mask = 1 << bit;
        a &= mask;
        b &= mask;
        res &= mask;
        if(a==0 && b == 0)
            return res>>bit; //set it back to bit 0 pos
        if(a & b)
            return res>>bit;
        if(a==0 && b!=0)
            return 1;
        return 0;
    }

    uint8_t SetAddFlags(uint8_t a, uint8_t b, uint8_t res)
    {
        return (CarryFromBit(a,b,res,7) << 4)+
            (CarryFromBit(a,b,res,3)<<5)+(((uint8_t)(res==0))<<7);
    }

    uint8_t SetSubFlags(uint8_t a, uint8_t b, uint8_t res)
    {
        //TODO:verify that we test for bit 4 or 3 for flag H IMPORTANT
        return (BorrowFromBit(a,b,res,7)<<4)+
            (BorrowFromBit(a,b,res,3)<<5)+(1<<6)+(((uint8_t)(res==0))<<7);
    }

    void triggerStopMode(bool stop);

public:
    Z80_Gameboy();
    void attachBus(Bus* b)
    {
        bus = b;
    }

    uint8_t read(uint16_t adr);

    void write(uint16_t adr,uint8_t data);

    uint8_t tick();

    uint8_t GetReg(uint8_t r);
    void SetReg(uint8_t r, uint8_t val);

    uint16_t GetRegPair(uint8_t r,bool accFlagsMode=false);
    void SetRegPair(uint8_t r,uint16_t val,bool accFlagsMode=false);

    void reset();

    uint16_t getPC()
    {
        return pc;
    }
    void setPC(uint16_t adr)
    {
        pc = priv_pc = adr;
    }

    std::string instDump(bool withPC = true);

    std::string trace(bool csvHeader = true,bool selectOldFlags = false,bool noShowBse = false)
    {
        uint8_t flags = this->flags;
        if(selectOldFlags)//useless now
        {
            flags = oldFlags;
        }

        std::stringstream ss;
        ss << std::hex << std::showbase;
        if(noShowBse)
            ss<<std::noshowbase;
        if(csvHeader)
            ss << "PC;A;B;C;D;E;H;L;BC;DE;HL;SP;flags;ime;inst_length;cycles;fromPC;dump;assembly\n";//assembly de l'inst executé et l'état APRES Exec
        ss  << pc
            <<";" <<std::setfill ('0') << std::setw (2)<<(int)GetReg(Registers::A)<<";"<<(int)GetReg(Registers::B)
            <<";"<<(int)GetReg(Registers::C)<<";"<<(int)GetReg(Registers::D)
            <<";"<<(int)GetReg(Registers::E)<<";"<<(int)GetReg(Registers::H)
            <<";"<<(int)GetReg(Registers::L)<<";"<<std::setfill ('0') << std::setw (4)<<GetRegPair(Registers_Pairs::BC)
            <<";"<<std::setfill ('0') << std::setw (4)<<GetRegPair(Registers_Pairs::DE)<<";"<<std::setfill ('0') << std::setw (4)<<GetRegPair(Registers_Pairs::HL)
            <<";"<<std::setfill ('0') << std::setw (4)<<GetRegPair(Registers_Pairs::SP)<<";"<<((flags&0x80)?'Z':'-')<<((flags&0x40)?'N':'-')<<((flags&0x20)?'H':'-')<<((flags&0x10)?'C':'-')<<std::dec
            <<";"<<(int)ime<<";"<<(int)inst_length<<";"<<(int)cyclesInst<<";"<<std::hex<<fromPC<<";"<<instDump(/*false*/) <<";"<<assembled_line<<std::endl;
        //ss << ((flags&0x80)?'Z':'-')<<((flags&0x40)?'N':'-')<<((flags&0x20)?'H':'-')<<((flags&0x10)?'C':'-');
        return ss.str();
    }
    void binaryDump(std::vector<uint8_t>&dump)
    {
        //dump.push_back((GetRegPair(Registers_Pairs::HL)<<16)+(GetRegPair(Registers_Pairs::DE)));
        //dump.push_back((GetRegPair(Registers_Pairs::BC)<<16)+sp);
        //dump.push_back((pc<<16)+(GetReg(Registers::A)<<8)+flags);

        int regs[6] = {H,L,D,E,B,C};
        for(int i = 0; i < 6; i++)
        {
            dump.push_back(GetReg(regs[i]));
        }
        dump.push_back((uint8_t)(sp>>8));
        dump.push_back((uint8_t)(sp&0b1111'1111));
        dump.push_back((uint8_t)(pc>>8));
        dump.push_back((uint8_t)(pc&0b1111'1111));
        dump.push_back(GetReg(Registers::A));
        dump.push_back(flags);
    }

    std::string assembled;
    bool disassemble = false;
    bool disassembleOver = false;
    std::string genAsm[0x8000];
    bool disassembleAdr = false;
private:
    std::string assembled_line;

    uint16_t priv_pc = 0x100;
    std::string Gen_Value(uint8_t val,bool indirect=false,bool signedval = false)
    {
        std::stringstream ss;
        ss << std::hex << std::showbase;
        if(signedval)
        {
            ss << std::dec;
            ss << ((int)((int8_t)val)); //TODO:Might no work
        }
        else
        {
            ss << (int)val;
        }
        if(indirect)
            return "("+ss.str()+")";
        return ss.str();
    }

    std::string Gen_Reg(uint8_t r,bool indirect=false)
    {
        //enum Registers{B=0,C=1,D=2,E=3,H=4,L=5,A=7};
        char rnames[] = {'B','C','D','E','H','L','?','A'};
        if(indirect)
            return std::string("(")+rnames[r]+")";
        return std::string("")+rnames[r];
    }
    std::string Gen_RegPair(uint8_t r, bool indirect=false, bool accFlagsMode=false)
    {
        //enum Registers_Pairs{BC=0,DE=1,HL=2,SP=3};
        std::string rnames[] = {"BC","DE","HL","SP"};
        if(accFlagsMode)
            rnames[3] = "AF";
        if(indirect)
            return "("+rnames[r]+")";
        return rnames[r];
    }
    std::string Gen_Address(uint16_t adr,bool indirect=false)
    {
        std::stringstream ss;
        ss << std::showbase << std::hex;
        ss << adr;
        if(indirect)
            return "("+ss.str()+")";
        return ss.str();
    }
    std::string Gen_Condition(uint8_t cc)
    {
        std::string conds[] = {"NZ","Z","NC","C"};
        return conds[cc];
    }
    void Gen_Assembly(std::string base)
    {
        assembled_line = base;
    }
    void Gen_Assembly(std::string base,std::string p1)
    {
        assembled_line = base + " " + p1;
    }
    void Gen_Assembly(std::string base,std::string p1,std::string p2)
    {
        assembled_line = base + " " + p1 + ", " + p2;
    }
public:
    void Trigger_Interupt(uint8_t interupt)
    {
        uint8_t if_reg = read(0xFF0F); uint8_t ie_reg = read(0xFFFF);
        if(ime == 1 && (if_reg & ie_reg)!=0)//TODO: check that (with manually causing interupts)
        {
            ime = 0;
            write(sp-1,pc>>8);
            write(sp-2,pc&0xFF);
            sp = sp-2;
            uint16_t adr[] = {0x40,0x48,0x50,0x58,0x60};
            pc = adr[interupt-1];//TODO:does it work with the cpu automatically incrementing it? yes
        }
    }

private:
    //Z80 Instructions
    void LD_R_Rp()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeMiddle(); uint8_t rp = GetOpcodeRight();
        Gen_Assembly("LD",Gen_Reg(r),Gen_Reg(rp));
        SetReg(r,GetReg(rp));
    }

    void LD_R_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1); uint8_t r = GetOpcodeMiddle();
        Gen_Assembly("LD",Gen_Reg(r),Gen_Value(n));
        SetReg(r,n);
    }

    void LD_R_iHL()
    {
        cycles = 2; inst_length = 1;
        uint8_t r = GetOpcodeMiddle();
        Gen_Assembly("LD",Gen_Reg(r),"(HL)");
        SetReg(r,read(GetRegPair(Registers_Pairs::HL)));
    }

    void LD_iHL_R()
    {
        cycles = 2; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("LD (HL),",Gen_Reg(r));
        write(GetRegPair(Registers_Pairs::HL),GetReg(r));
    }

    void LD_iHL_N()
    {
        cycles = 3; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("LD (HL),",Gen_Value(n));
        write(GetRegPair(Registers_Pairs::HL),n);
    }

    void LD_A_iBC()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LD A, (BC)");
        SetReg(Registers::A,read(GetRegPair(Registers_Pairs::BC)));
    }

    void LD_A_iDE()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LD A, (DE)");
        SetReg(Registers::A,read(GetRegPair(Registers_Pairs::DE)));
    }

    void LDH_A_iC()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LDH A, (C)");
        SetReg(Registers::A,read(0xFF00 + GetReg(Registers::C)));
    }

    void LDH_iC_A()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LDH (C), A");
        write(0xFF00 + GetReg(Registers::C),GetReg(Registers::A));
    }

    void LDH_A_iN()
    {
        cycles = 3; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("LDH A,",Gen_Address(0xFF00 + n,true));
        SetReg(Registers::A,read(0xFF00 + n));
    }

    void LDH_iN_A()
    {
        cycles = 3; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("LDH",Gen_Address(0xFF00 + n,true),"A");
        write(0xFF00 + n,GetReg(Registers::A));
    }

    void LD_A_iNN()
    {
        cycles = 4; inst_length = 3;
        uint16_t nn = read(pc+1);nn+= ((uint16_t)read(pc+2))<<8; //I'm lucky this operator converts to int
        Gen_Assembly("LD A,",Gen_Address(nn,true));
        SetReg(Registers::A,read(nn));
    }

    void LD_iNN_A()
    {
        cycles = 4; inst_length = 3;
        uint16_t nn = read(pc+1); nn+= ((uint16_t)read(pc+2))<<8;
        Gen_Assembly("LD",Gen_Address(nn,true),"A");
        write(nn,GetReg(Registers::A));
    }

    void LDI_A_iHL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LDI A, (HL)");
        SetReg(Registers::A,read(GetRegPair(Registers_Pairs::HL)));
        SetRegPair(Registers_Pairs::HL,GetRegPair(Registers_Pairs::HL)+1);
    }

    void LDD_A_iHL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LDD A, (HL)");
        SetReg(Registers::A,read(GetRegPair(Registers_Pairs::HL)));
        SetRegPair(Registers_Pairs::HL,GetRegPair(Registers_Pairs::HL)-1);
    }

    void LD_iBC_A()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LD (BC), A");
        write(GetRegPair(Registers_Pairs::BC),GetReg(Registers::A));
    }

    void LD_iDE_A()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LD (DE), A");
        write(GetRegPair(Registers_Pairs::DE),GetReg(Registers::A));
    }

    void LDI_iHL_A()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LDI (HL), A");
        write(GetRegPair(Registers_Pairs::HL),GetReg(Registers::A));
        SetRegPair(Registers_Pairs::HL,GetRegPair(Registers_Pairs::HL)+1);
    }

    void LDD_iHL_A()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LDD (HL), A");
        write(GetRegPair(Registers_Pairs::HL),GetReg(Registers::A));
        SetRegPair(Registers_Pairs::HL,GetRegPair(Registers_Pairs::HL)-1);
    }

    void LD_DD_NN()
    {
        cycles = 3; inst_length = 3;
        uint16_t nn = read(pc+1); nn+= (((uint16_t)read(pc+2))<<8);
        uint8_t dd = GetOpcodeMiddle()>>1;
        Gen_Assembly("LD",Gen_RegPair(dd),Gen_Address(nn));
        SetRegPair(dd,nn);
    }

    void LD_SP_HL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("LD SP, HL");
        sp = GetRegPair(Registers_Pairs::HL);
    }

    void PUSH_QQ()
    {
        cycles = 4; inst_length = 1;
        uint8_t qq = GetOpcodeMiddle()>>1;
        uint8_t low = GetRegPair(qq,true) & 0b11'111'111; uint8_t high = GetRegPair(qq,true) >> 8;
        Gen_Assembly("PUSH",Gen_RegPair(qq,false,true));
        write(sp-1,high);
        write(sp-2,low);
        sp = sp-2;
    }

    void POP_QQ()
    {
        cycles = 3; inst_length = 1;
        uint8_t qq = GetOpcodeMiddle()>>1;
        Gen_Assembly("POP",Gen_RegPair(qq,false,true));
        uint16_t val = read(sp);
        val += ((uint16_t)read(sp+1))<<8;
        SetRegPair(qq,val,true);
        sp = sp+2;
    }

    void LDHL_SP_E()
    {
        cycles = 3; inst_length = 2;
        uint8_t e = read(pc+1);
        Gen_Assembly("LDHL SP,",Gen_Value(e,false,true));
        //TODO: test
        uint16_t res = sp + e;
        //flags = (CarryFromBit(sp,e,res,11) << 5) + (CarryFromBit(sp,e,res,15) << 4);
        flags = (CarryFromBit(sp,e,res,3)<<5)+(CarryFromBit(sp,e,res,7)<<4);
        SetRegPair(Registers_Pairs::HL, res);//CAREFUL
    }

    void LD_iNN_SP()
    {
        cycles = 5; inst_length = 3;
        uint16_t nn = read(pc+1); nn += ((uint16_t)read(pc+2))<<8;
        Gen_Assembly("LD",Gen_Address(nn,true),"SP");
        uint8_t low = sp & 0xFF; uint8_t high = sp >> 8;
        write(nn,low);
        write(nn+1,high);
    }

    void ADD_A_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("ADD A,",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) + GetReg(r);
        flags = SetAddFlags(GetReg(Registers::A),GetReg(r),res);
        SetReg(Registers::A,res);
    }

    void ADD_A_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("ADD A,",Gen_Value(n,false,true)); //might want to change that
        uint8_t res = GetReg(Registers::A) + n;
        flags = SetAddFlags(GetReg(Registers::A),n,res);
        SetReg(Registers::A,res);
    }

    void ADD_A_iHL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("ADD A, (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t res = GetReg(Registers::A) + val;
        flags = SetAddFlags(GetReg(Registers::A),val,res);
        SetReg(Registers::A,res);
    }

    void ADC_A_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("ADC A,",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) + GetReg(r) + ((flags>>4)&0x01);
        flags = SetAddFlags(GetReg(Registers::A),GetReg(r),res);
        SetReg(Registers::A,res);
    }

    void ADC_A_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("ADC A,",Gen_Value(n,false,true)); //might want to change that
        uint8_t res = GetReg(Registers::A) + n + ((flags>>4)&0x01);
        flags = SetAddFlags(GetReg(Registers::A),n,res);
        SetReg(Registers::A,res);
    }

    void ADC_A_iHL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("ADC A, (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t res = GetReg(Registers::A) + val + ((flags>>4)&0x01);
        flags = SetAddFlags(GetReg(Registers::A),val,res);
        SetReg(Registers::A,res);
    }

    void SUB_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("SUB",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) - GetReg(r);
        flags = SetSubFlags(GetReg(Registers::A),GetReg(r),res);
        SetReg(Registers::A,res);
    }

    void SUB_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("SUB",Gen_Value(n,false,true));
        uint8_t res = GetReg(Registers::A) - n;
        flags = SetSubFlags(GetReg(Registers::A),n,res);
        SetReg(Registers::A,res);
    }

    void SUB_iHL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("SUB (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t res = GetReg(Registers::A) - val;
        flags = SetSubFlags(GetReg(Registers::A),val,res);
        SetReg(Registers::A,res);
    }

    void SBC_A_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("SBC A,",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) - GetReg(r) - ((flags >> 4)&1);
        flags = SetSubFlags(GetReg(Registers::A),GetReg(r),res); //Does the borrow/carry bit change the flags?--> I think not TODO:TEST THIS
        SetReg(Registers::A,res);
    }

    void SBC_A_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("SBC A,",Gen_Value(n,false,true));
        uint8_t res = GetReg(Registers::A) - n - ((flags >> 4)&1);
        flags = SetSubFlags(GetReg(Registers::A),n,res);
        SetReg(Registers::A,res);
    }

    void SBC_A_iHL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("SBC A, (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t res = GetReg(Registers::A) - val - ((flags >> 4)&1);
        flags = SetSubFlags(GetReg(Registers::A),val,res);
        SetReg(Registers::A,res);
    }

    void AND_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("AND",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) & GetReg(r);
        flags = 0b0010'0000 + (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void AND_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("AND",Gen_Value(n,false,true));
        uint8_t res = GetReg(Registers::A) & n;
        flags = 0b0010'0000 + (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void AND_iHL()
    {
        cycles = 2; inst_length = 1;
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        Gen_Assembly("AND (HL)");
        uint8_t res = GetReg(Registers::A) & val;
        flags = 0b0010'0000 + (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void OR_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("OR",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) | GetReg(r);
        flags = (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void OR_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("OR",Gen_Value(n,false,true));
        uint8_t res = GetReg(Registers::A) | n;
        flags = (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void OR_iHL()
    {
        cycles = 2; inst_length = 1;
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        Gen_Assembly("OR (HL)");
        uint8_t res = GetReg(Registers::A) | val;
        flags = (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void XOR_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("XOR",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) ^ GetReg(r);
        flags = (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void XOR_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("XOR",Gen_Value(n,false,true));
        uint8_t res = GetReg(Registers::A) ^ n;
        flags = (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void XOR_iHL()
    {
        cycles = 2; inst_length = 1;
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        Gen_Assembly("XOR (HL)");
        uint8_t res = GetReg(Registers::A) ^ val;
        flags = (res==0?1<<7:0);
        SetReg(Registers::A,res);
    }

    void CP_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("CP",Gen_Reg(r));
        uint8_t res = GetReg(Registers::A) - GetReg(r);
        flags = SetSubFlags(GetReg(Registers::A),GetReg(r),res);
    }

    void CP_N()
    {
        cycles = 2; inst_length = 2;
        uint8_t n = read(pc+1);
        Gen_Assembly("CP",Gen_Value(n,false,true));
        uint8_t res = GetReg(Registers::A) - n;
        flags = SetSubFlags(GetReg(Registers::A),n,res);
    }

    void CP_iHL()
    {
        cycles = 2; inst_length = 1;
        Gen_Assembly("CP (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t res = GetReg(Registers::A) - val;
        flags = SetSubFlags(GetReg(Registers::A),val,res);
    }

    void INC_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeMiddle();
        Gen_Assembly("INC",Gen_Reg(r));
        uint8_t val = GetReg(r);
        val++;
        flags = (flags & 0b0001'0000)+(CarryFromBit(val-1,1,val,3)<<5)+(val==0?1<<7:0);
        SetReg(r,val);
    }

    void INC_iHL()
    {
        cycles = 3; inst_length = 1;
        Gen_Assembly("INC (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        val++;
        flags = (flags & 0b0001'0000)+(CarryFromBit(val-1,1,val,3)<<5)+(val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void DEC_R()
    {
        cycles = 1; inst_length = 1;
        uint8_t r = GetOpcodeMiddle();
        Gen_Assembly("DEC",Gen_Reg(r));
        uint8_t val = GetReg(r);
        val--;
        flags = (flags & 0b0001'0000)+(BorrowFromBit(val+1,1,val,3)<<5)+(val==0?1<<7:0) +(1<<6);
        SetReg(r,val);
    }

    void DEC_iHL()
    {
        cycles = 3; inst_length = 1;
        Gen_Assembly("DEC (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        val--;
        flags = (flags & 0b0001'0000)+(BorrowFromBit(val+1,1,val,3)<<5)+(val==0?1<<7:0) +(1<<6); //TODO:Not sure for borrow bit (changed when comparing to other emulator)
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void ADD_HL_SS()
    {
        cycles = 2; inst_length = 1;
        uint8_t ss = GetOpcodeMiddle()>>1;
        Gen_Assembly("ADD HL,",Gen_RegPair(ss));
        uint16_t res = GetRegPair(ss) + GetRegPair(Registers_Pairs::HL);
        flags = (flags & 0b1000'0000) + (CarryFromBit(GetRegPair(ss),GetRegPair(Registers_Pairs::HL),res,11)<<5)
                    +(CarryFromBit(GetRegPair(ss),GetRegPair(Registers_Pairs::HL),res,15)<<4);
        SetRegPair(Registers_Pairs::HL,res);
    }

    void ADD_SP_E()
    {
        cycles = 4; inst_length = 2;
        int8_t e = read(pc+1);
        Gen_Assembly("ADD SP,",Gen_Value(e,false,true));
        uint16_t res = sp + (int16_t)e;
        flags = (CarryFromBit(sp,e,res,3)<<5)+(CarryFromBit(sp,e,res,7)<<4);
        sp = res;
    }

    void INC_SS()
    {
        cycles = 2; inst_length = 1;
        uint8_t ss = GetOpcodeMiddle()>>1;
        Gen_Assembly("INC",Gen_RegPair(ss));
        SetRegPair(ss,GetRegPair(ss)+1);
    }

    void DEC_SS()
    {
        cycles = 2; inst_length = 1;
        uint8_t ss = GetOpcodeMiddle()>>1;
        Gen_Assembly("DEC",Gen_RegPair(ss));
        SetRegPair(ss,GetRegPair(ss)-1);
    }

    void RLCA()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("RLCA");
        uint8_t val = GetReg(Registers::A);
        uint8_t bit = (val >> 7) & 1;
        val = (val << 1) + bit;
        flags = bit << 4;
        SetReg(Registers::A,val);
    }

    void RLA()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("RLA");
        uint8_t val = GetReg(Registers::A);
        uint8_t bit = (val >> 7) & 1;
        val = (val << 1) + ((flags >> 4)&1);
        flags = bit << 4;
        SetReg(Registers::A,val);
    }

    void RRCA()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("RRCA");
        uint8_t val = GetReg(Registers::A);
        uint8_t bit = val & 1;
        val = (val >> 1) + (bit<<7);
        flags = bit << 4;
        SetReg(Registers::A,val);
    }

    void RRA()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("RRA");
        uint8_t val = GetReg(Registers::A);
        uint8_t bit = val & 1;
        val = (val >> 1) + ((flags<<3)&0b1000'0000);
        flags = bit << 4;
        SetReg(Registers::A,val);
    }

    void META_ROTATE()
    {
        //SPECIAL INSTRUCTION: OPCODE ON MORE THAN 8 BITS:
        inst_length = 2;
        opcode = read(pc+1);//maybe use other variable
        uint8_t masked_opcode = opcode & 0b11'111'000;
        if(masked_opcode == 0b00'000'000)
        {
            //RLC r OR RLC (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                RLC_iHL();
            else
                RLC_R();
        }
        else if(masked_opcode == 0b00'010'000)
        {
            //RL r OR RL (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                RL_iHL();
            else
                RL_R();
        }
        else if(masked_opcode == 0b00'001'000)
        {
            //RRC r OR RRC (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                RRC_iHL();
            else
                RRC_R();
        }
        else if(masked_opcode == 0b00'011'000)
        {
            //RR r OR RR (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                RR_iHL();
            else
                RR_R();
        }
        else if(masked_opcode == 0b00'100'000)
        {
            //SLA r OR SLA (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                SLA_iHL();
            else
                SLA_R();
        }
        else if(masked_opcode == 0b00'101'000)
        {
            //SLA r OR SLA (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                SRA_iHL();
            else
                SRA_R();
        }
        else if(masked_opcode == 0b00'111'000)
        {
            //SLL r OR SLL (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                SRL_iHL();
            else
                SRL_R();
        }
        else if(masked_opcode == 0b00'110'000)
        {
            //SLL r OR SLL (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                SWAP_iHL();
            else
                SWAP_R();
        }
        else
        {
            META_BIT();
        }
    }

    void RLC_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("RLC",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t bit = (val >> 7) & 1;
        val = (val << 1) + bit;
        flags = (bit << 4) + (val==0?1<<7:0);
        SetReg(r,val);
    }

    void RLC_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("RLC (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t bit = (val >> 7) & 1;
        val = (val << 1) + bit;
        flags = (bit << 4) + (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void RL_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("RL",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t bit = (val >> 7) & 1;
        val = (val << 1) + ((flags>>4)&1);
        flags = (bit << 4) + (val==0?1<<7:0);
        SetReg(r,val);
    }

    void RL_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("RL (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t bit = (val >> 7) & 1;
        val = (val << 1) + ((flags>>4)&1);
        flags = (bit << 4) + (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void RRC_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("RRC",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t bit = val & 1;
        val = (val >> 1) + (bit<<7);
        flags = (bit << 4) + (val==0?1<<7:0);
        SetReg(r,val);
    }

    void RRC_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("RRC (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t bit = val & 1;
        val = (val >> 1) + (bit<<7);
        flags = (bit << 4) + (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void RR_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("RR",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t bit = val & 1;
        val = (val >> 1) + ((flags<<3)&0b1000'0000);
        flags = (bit << 4) + (val==0?1<<7:0);
        SetReg(r,val);
    }

    void RR_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("RR (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t bit = val & 1;
        val = (val >> 1) + ((flags<<3)&0b1000'0000);
        flags = (bit << 4) + (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void SLA_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("SLA",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t bit = (val >> 7)&1;
        val = val << 1;
        flags = (bit << 4) + (val==0?1<<7:0);
        SetReg(r,val);
    }

    void SLA_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("SLA (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t bit = (val >> 7)&1;
        val = val << 1;
        flags = (bit << 4) + (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void SRA_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("SRA",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t bit = val&1;
        val = (val >> 1) + ((val<<1)&0b1000'0000);
        flags = (bit << 4) + (val==0?1<<7:0);
        SetReg(r,val);
    }

    void SRA_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("SRA (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t bit = val&1;
        val = (val >> 1) + ((val<<1)&0b1000'0000);
        flags = (bit << 4) + (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void SRL_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("SRL",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t bit = val&1;
        val = (val >> 1);
        flags = (bit << 4) + (val==0?1<<7:0);
        SetReg(r,val);
    }

    void SRL_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("SRL (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t bit = val&1;
        val = (val >> 1);
        flags = (bit << 4) + (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void SWAP_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t r = GetOpcodeRight();
        Gen_Assembly("SWAP",Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t low = val&0b0000'1111;
        val = (val >> 4) + (low<<4);
        flags = (val==0?1<<7:0);
        SetReg(r,val);
    }

    void SWAP_iHL()
    {
        cycles = 4; inst_length = 2;
        Gen_Assembly("SWAP (HL)");
        uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        uint8_t low = val&0b0000'1111;
        val = (val >> 4) + (low<<4);
        flags = (val==0?1<<7:0);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void META_BIT()
    {
        //SPECIAL INSTRUCTION: OPCODE ON MORE THAN 8 BITS:
        inst_length = 2;
        opcode = read(pc+1);//maybe use other variable
        uint8_t masked_opcode = opcode & 0b11'000'000;
        if(masked_opcode == 0b01'000'000)
        {
            //BIT b,r OR BIT b, (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                BIT_B_iHL();
            else
                BIT_B_R();
        }
        else if(masked_opcode == 0b10'000'000)
        {
            //SET b,r OR SET b, (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                SET_B_iHL();
            else
                SET_B_R();
        }
        else if(masked_opcode == 0b10'000'000)
        {
            //RES b,r OR RES b, (HL):
            uint8_t r = GetOpcodeRight();
            if(r == 0b110)
                RES_B_iHL();
            else
                RES_B_R();
        }
    }

    void BIT_B_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t b = GetOpcodeMiddle(); uint8_t r = GetOpcodeRight();
        Gen_Assembly("BIT",Gen_Value(b,false,true),Gen_Reg(r));
        uint8_t val = GetReg(r);
        uint8_t complement_bit = !((val>>b)&1);
        flags = (flags&0b0001'0000)+(1<<5)+(complement_bit<<7);
    }

    void BIT_B_iHL()
    {
        cycles = 3; inst_length = 2;
        uint8_t b = GetOpcodeMiddle(); uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        Gen_Assembly("BIT",Gen_Value(b,false,true),"(HL)");
        uint8_t complement_bit = !((val>>b)&1);
        flags = (flags&0b0001'0000)+(1<<5)+(complement_bit<<7);
    }

    void SET_B_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t b = GetOpcodeMiddle(); uint8_t r = GetOpcodeRight();
        Gen_Assembly("SET",Gen_Value(b,false,true),Gen_Reg(r));
        uint8_t val = GetReg(r);
        val = val | (1<<b);
        SetReg(r,val);
    }

    void SET_B_iHL()
    {
        cycles = 4; inst_length = 2;
        uint8_t b = GetOpcodeMiddle(); uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        Gen_Assembly("SET",Gen_Value(b,false,true),"(HL)");
        val = val | (1<<b);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void RES_B_R()
    {
        cycles = 2; inst_length = 2;
        uint8_t b = GetOpcodeMiddle(); uint8_t r = GetOpcodeRight();
        Gen_Assembly("RES",Gen_Value(b,false,true),Gen_Reg(r));
        uint8_t val = GetReg(r);
        val = val & ~(1<<b);
        SetReg(r,val);
    }

    void RES_B_iHL()
    {
        cycles = 4; inst_length = 2;
        uint8_t b = GetOpcodeMiddle(); uint8_t val = read(GetRegPair(Registers_Pairs::HL));
        Gen_Assembly("RES",Gen_Value(b,false,true),"(HL)");
        val = val & ~(1<<b);
        write(GetRegPair(Registers_Pairs::HL),val);
    }

    void JP_NN()
    {
        cycles = 4; inst_length = 3;
        uint16_t nn = read(pc+1)+(((uint16_t)read(pc+2))<<8);
        Gen_Assembly("JP",Gen_Address(nn));
        pc = nn - inst_length; //The inst_length is always added after the exec of instruction, just setting the length to 0 won't work in disassemble mode
    }

    void JP_CC_NN()
    {
        cycles = 4; inst_length = 3;
        uint8_t cc = GetOpcodeMiddle(); uint16_t nn = read(pc+1)+(((uint16_t)read(pc+2))<<8);
        Gen_Assembly("JP",Gen_Condition(cc),Gen_Address(nn));
        if(((cc == 0) && (flags>>7)==0) || ((cc == 1) && (flags>>7)==1)
            || ((cc == 2) && ((flags>>4)&1)==0) || ((cc == 3) && ((flags>>4)&1)==1))
        {
            pc = nn - inst_length;
        }
        else
        {
            //No jump:
            cycles = 3;
        }
    }

    void JR_E()
    {
        cycles = 3; inst_length = 2;
        uint8_t e = read(pc+1)+inst_length;//TODO TEST THAT ...signed or unsigned ?
        Gen_Assembly("JR",Gen_Value(e,false,true));
        pc = (pc+(int8_t)e)-inst_length;
    }

    void JR_CC_E()
    {
        cycles = 3; inst_length = 2;
        uint8_t cc = GetOpcodeMiddle()&0b011; uint8_t e = read(pc+1)+2;
        Gen_Assembly("JR",Gen_Condition(cc),Gen_Value(e,false,true));
        if(((cc == 0) && (flags>>7)==0) || ((cc == 1) && (flags>>7)==1)
            || ((cc == 2) && ((flags>>4)&1)==0) || ((cc == 3) && ((flags>>4)&1)==1))
        {
            pc = pc + ((int8_t)e) - inst_length;
        }
        else
        {
            //No jump:
            cycles = 2;
        }
    }

    void JP_iHL()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("JP (HL)");
        //pc = read(GetRegPair(Registers_Pairs::HL))-inst_length;
        pc = GetRegPair(Registers_Pairs::HL)-inst_length;
    }

    void CALL_NN()
    {
        cycles = 6; inst_length = 3;
        uint16_t nn = read(pc+1)+(((uint16_t)read(pc+2))<<8);
        Gen_Assembly("CALL",Gen_Address(nn));
        write(sp-1,(pc+inst_length)>>8);
        write(sp-2,(pc+inst_length)&0xFF);
        pc = nn - inst_length;
        sp=sp-2;
    }

    void CALL_CC_NN()
    {
        cycles = 6; inst_length = 3;
        uint8_t cc = GetOpcodeMiddle()&3; uint16_t nn = read(pc+1)+(((uint16_t)read(pc+2))<<8);
        //std::cout<<std::bitset<8>(opcode)<<std::endl; //TODO: MANAGE ILLEGAL OPCODES
        Gen_Assembly("CALL",Gen_Condition(cc),Gen_Address(nn));

        if(((cc == 0) && (flags>>7)==0) || ((cc == 1) && (flags>>7)==1)
            || ((cc == 2) && ((flags>>4)&1)==0) || ((cc == 3) && ((flags>>4)&1)==1))
        {
            write(sp-1,(pc+inst_length)>>8);
            write(sp-2,(pc+inst_length)&0xFF);
            pc = nn - inst_length;
            sp=sp-2;
        }
        else
        {
            cycles = 3;
        }
    }

    void RET()
    {
        cycles = 4; inst_length = 1;
        Gen_Assembly("RET");
        pc = read(sp) + (((uint16_t)read(sp+1))<<8) - inst_length; //Take into account that we add inst_length in the decode step
        sp += 2;
    }

    void RETI()
    {
        cycles = 4; inst_length = 1;
        Gen_Assembly("RET");
        pc = read(sp) + (((uint16_t)read(sp+1))<<8)-inst_length;
        sp += 2;
        ime = 1;
        //TODO:since if is hardware reset, maybe i should do it here:
        write(0xFF0F,0);
    }

    void RET_CC()
    {
        cycles = 5; inst_length = 1;
        uint8_t cc = GetOpcodeMiddle();
        Gen_Assembly("RET",Gen_Condition(cc));
        if(((cc == 0) && (flags>>7)==0) || ((cc == 1) && (flags>>7)==1)
            || ((cc == 2) && ((flags>>4)&1)==0) || ((cc == 3) && ((flags>>4)&1)==1))
        {
            pc = read(sp) + (((uint16_t)read(sp+1))<<8) - inst_length;
            sp += 2;
        }
        else
        {
            cycles = 2;
        }

    }

    void RST_T()
    {
        cycles = 4; inst_length = 1;
        uint8_t t = GetOpcodeMiddle();
        Gen_Assembly("RST",Gen_Value(t));
        write(sp-1,(pc)>>8); //write current value of pc
        write(sp-2,(pc)&0xFF);
        sp=sp-2;
        uint16_t rst_adr[] = {0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38};
        pc = rst_adr[t]-inst_length;
    }

    void DAA()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("DAA");
        uint8_t N = (flags >> 6)&1; uint8_t H = (flags >> 5)&1; uint8_t CY = (flags>>4)&1;
        uint8_t offset = 0; uint8_t val = GetReg(Registers::A);
        uint8_t left = (val >> 4)&0b0000'1111; uint8_t right = val&0b0000'1111;
        if(N==0)
        {
            if(H==1 || (right>=0xA && right<=0xF))
            {
                offset += 0x06;
            }
            if(CY==1 || (left>=0xA && left <= 0xF) || ((right>=0xA && right<=0xF)&&left==9))
            {
                offset += 0x60;
                flags |= 0b0001'0000; //cy=1
            }
        }
        else
        {
            if(H==1)
            {
                offset += 0x0A;
            }
            uint8_t left_offset[] = {0x00,0xF0,0xA0,0x90};
            offset += left_offset[(CY<<1)|H];
            if(CY==1)
            {
                flags |= 0b0001'0000; //cy=1
            }
        }
        SetReg(Registers::A,val+offset);
        flags = (flags&0b0101'0000) | (((uint8_t)(val+offset)==0)?(1<<7):0);
    }

    void CPL()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("CPL");
        SetReg(Registers::A,~GetReg(Registers::A));
        flags = flags | 0b0110'0000;
    }

    void NOP()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("NOP");
        //Does nothing
    }

    void CCF()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("CCF");
        uint8_t inverted_cy = (~flags >> 4)&1;
        flags = (flags&0b1000'0000)+(inverted_cy<<4);
    }

    void SCF()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("SCF");
        flags = (flags&0b1000'0000)+(1<<4);
    }

    void DI()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("DI");
        ime = 0;
    }

    void EI()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("EI");
        ime = 1;
        //TODO: maybe, since if is hardware reset, i should do it here
        write(0xFF0F,0);
    }

    void HALT()
    {
        cycles = 1; inst_length = 1;
        Gen_Assembly("HALT");
        //todo
    }

    void STOP()
    {
        cycles = 1; inst_length = 2; //careful opcode on two bytes it seems
        Gen_Assembly("STOP");
        triggerStopMode(true);
    }
};

#endif // Z80_GAMEBOY_H
