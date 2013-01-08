#include <stdio.h>
#include <GLUT/glut.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "main.h"
#include "mmc1.h"
//#define DEBUG_NESTEST
//#define DEBUG_INSTR_TEST
//#define GRAPHICS_OFF
#define WINDOW_SIZE_X 256
#define WINDOW_SIZE_Y 240
#define TIMER_FUNC_FREQ 10

int cycles = 0;
int DMA_WAIT = 0;

cpu CPU;
ram * RAM ;
rom * ROM = new rom() ;
ppu * PPU  = new ppu();
void push(uint8_t value)
{
    //STACK is $0100 - $01FF
    RAM->store(  value , ((uint16_t)CPU.S | 0x0100));
    CPU.S -= 1;
}

uint8_t pop()
{
    CPU.S +=1;
    uint8_t value  = RAM->read( ((uint16_t)CPU.S | 0x0100));
    return value;
}

void checkOverflowAdd(uint16_t sum, uint8_t op1, uint8_t op2)
{
//!((AC ^ src) & 0x80) && ((AC ^ temp) & 0x80)
    if (!((op1 ^ op2) & 0x80) && ((op1 ^ sum ) & 0x80)) 
    {
        CPU.setFlag('V', true);
    } 
    else 
    {
        CPU.setFlag('V', false);
    }

}
void checkOverflowSub(uint16_t sum, uint8_t op1, uint8_t op2)
{
    if (((op1 ^ op2) & 0x80) && ((op1 ^ sum ) & 0x80)) 
    {
        CPU.setFlag('V', true);
    } 
    else 
    {
        CPU.setFlag('V', false);
    }

}

void checkZero(uint16_t value)
{
    value = value & 0xff;
    if( value ==0 )
    {
        CPU.setFlag('Z', true);
    }
    else
    {   
        CPU.setFlag('Z', false);
    }

}

void checkNegative(uint8_t value)
{
    if ( ( value& 0x80) == 0x80 ) 
    {
        CPU.setFlag('N', true);

   } 
    else
    {
        CPU.setFlag('N', false);
    }
}

void checkCarryAdd(uint16_t value)
{
    //ONLY TESTED FOR ADD
    if ( value > 0xff) 
    {
        CPU.setFlag('C', true);

    }
    else
    {
        CPU.setFlag('C', false);
    }
}
void checkCarrySubtraction(bool carryNeeded)
{
    CPU.setFlag('C', !carryNeeded);
}
    
void checkBRK(uint16_t value);
void checkDEC(uint16_t value);
void checkIRQ(uint16_t value);


uint16_t immediateAddress()
{
    CPU.pc++;
    return CPU.pc -1;
}

uint16_t zPage()
{   
    CPU.pc++;
    uint8_t addy = RAM->read(CPU.pc -1);
    //take msb as address with zeros in front
    
    return (uint16_t)addy;
}


uint16_t absolute()
{
    CPU.pc += 2;
    uint8_t high = RAM->read(CPU.pc-1);
    uint8_t low = RAM->read(CPU.pc-2);
    uint16_t addy = ((uint16_t) high << 8) + low;
    return addy;
}
    
uint16_t zPageIndexed(uint8_t value)
{
    uint16_t addy = zPage();
    addy += value;
    return (addy & 0xff);
}

    
uint16_t absoluteIndexed( uint8_t value)
{
    uint16_t addy = absolute();
    addy += value;
    return addy;
}

uint16_t indexedIndirect( uint8_t value)
{
    CPU.pc++;
    uint16_t current =(uint16_t) RAM->read(CPU.pc-1);
    current += value;
    current &= 0xff;
    uint8_t high = RAM->read( (current+1) & 0xff);
    uint8_t low = RAM->read(current);

    return ((((uint16_t) high) << 8) + low);

}

uint16_t indirectIndexed(uint8_t value)
{
    CPU.pc++;
    uint16_t zeroPageAddy = RAM->read(CPU.pc-1);
    uint8_t low = RAM->read(zeroPageAddy);
    uint8_t high = RAM->read((zeroPageAddy+1) & 0xff);
    uint16_t combined = ((uint16_t)high <<8) + low;

    combined += value;
    return combined;

}


//ADD with carry
void ADC( uint16_t memloc )
{
    //read ram from loc value
    uint8_t value = RAM->read(memloc);
    bool carry = CPU.getFlag('C');
    uint8_t carry_bit  = carry ? 0x01 : 0x00;
    uint16_t intermediate = CPU.A +  value + carry_bit;
    //check flags
    //CPU.clearFlags();
    checkOverflowAdd(intermediate, CPU.A, value);
    checkCarryAdd(intermediate);
    CPU.A = intermediate & 0xff;
    checkZero(intermediate);
    checkNegative(CPU.A);
}


//logical AND
void AND(uint16_t memloc)
{
    uint8_t value = RAM->read(memloc);
#ifdef DEBUG_NESTEST
    printf(" AND %X  ", value);
#endif
    CPU.A &= value;
    //CPU.clearFlags();
    checkNegative(CPU.A);
    checkZero(CPU.A);
}

//Arithmetic shift left with memloc
void ASL( uint16_t memloc)
{
    //CPU.clearFlags();
    uint16_t value = RAM->read(memloc);
    char highest_bit = value & 0x80;
    CPU.setFlag('C', (bool)highest_bit);
    value = value << 1;
    RAM->store(value & 0xff , memloc);
    checkZero(value & 0xff);
    checkNegative(value & 0xff);
}

//Arithmetic shift left with accumulator as the value to shift
void ASL_accumulator()
{
    //CPU.clearFlags();
    uint16_t value = CPU.A;
    char highest_bit = value & 0x80;
    CPU.setFlag('C', (bool)highest_bit);
    value = value << 1;
    CPU.A = value & 0xff;
    checkZero(CPU.A);
    checkNegative(CPU.A);
}
void Bcycles_helper(uint16_t oldcpu)
{

    if ( (oldcpu & 0xff00) == (CPU.pc & 0xff00))
    {

        cycles +=1;
    } else {
        cycles +=2;

    }
}
int relativeAddress(uint8_t value)
{
    if ( value & 0x80)
    { 
        //it is negative!
        int sign =  -(0x100 - (int)value);
        //printf("value: %x , signed : %d \n", value, sign);
        return sign;
    }
    else
    {
        return value;
    }
}
//branch on Carry Clear
void BCC()
{
    CPU.pc +=1;
   uint16_t oldcpu = CPU.pc; 
    
    if ( !CPU.getFlag('C'))
    { //branch!
        int offset = relativeAddress(RAM->read(CPU.pc-1));
        CPU.pc += offset;
        Bcycles_helper(oldcpu);
    }
}
    
//branch on carry set
void BCS()
{
    CPU.pc ++;
    uint16_t oldcpu = CPU.pc;
    if( CPU.getFlag('C'))
    { //BRANCH!
        int offset = relativeAddress(RAM->read(CPU.pc-1));
        CPU.pc += (offset);
        Bcycles_helper(oldcpu);
    }
#ifdef DEBUG_NESTEST
    printf("  BCS $%X  ", CPU.pc);
#endif
    //CPU.clearFlags();
}

void BEQ()
{
    CPU.pc++;
    uint16_t oldcpu = CPU.pc;
    if( CPU.getFlag('Z'))
    {
        int offset = relativeAddress(RAM->read(CPU.pc-1));
        CPU.pc += (offset);
        Bcycles_helper(oldcpu);
    }
#ifdef DEBUG_NESTEST
    printf("  BEQ $%X  ", CPU.pc);
#endif
}

void BIT(uint16_t memloc)
{
    //CPU.clearFlags();
    uint8_t value = RAM->read(memloc);
    uint8_t andResult = value & CPU.A;
    uint8_t N = value & 0x80;
    uint8_t V = value & 0x40;
    checkZero(andResult);
    if ( N == 0x80 )
    {
        CPU.setFlag('N', true);

    }
    else
    {
        CPU.setFlag('N', false);
    }

    if ( V == 0x40 )
    {
        CPU.setFlag ( 'V', true);
    }
    else
    {
        CPU.setFlag('V', false);
    }
}


void BMI()
{
    CPU.pc++;
    if( CPU.getFlag('N'))
    {
        int offset = relativeAddress(RAM->read(CPU.pc-1));
        CPU.pc += (offset);
        cycles ++;
    }
#ifdef DEBUG_NESTEST
    printf("  BMI $%X  ", CPU.pc);
#endif
}
void BNE()
{
    CPU.pc++;
    uint16_t oldcpu = CPU.pc;
    if( !CPU.getFlag('Z'))
    {
        int offset = relativeAddress(RAM->read(CPU.pc-1));
        CPU.pc += (offset);
        Bcycles_helper(oldcpu);
    }
#ifdef DEBUG_NESTEST
    printf("  BNE $%X  ", CPU.pc);
#endif
    //CPU.clearFlags();
}

void BPL()
{
    CPU.pc++;
    uint16_t oldcpu = CPU.pc;
    int offset;
    if(! CPU.getFlag('N'))
    {
    offset = relativeAddress(RAM->read(CPU.pc-1));
        CPU.pc += (offset);
        Bcycles_helper(oldcpu);
    }
#ifdef DEBUG_NESTEST
    printf(" offset: %d , %X  BPL $%X  ", offset, RAM->read(CPU.pc-1), CPU.pc);
#endif
    //CPU.clearFlags();
}



void BVC()
{
    CPU.pc++;
    uint16_t oldcpu = CPU.pc;
    uint8_t offset;
    if(! CPU.getFlag('V'))
    {
        offset = RAM->read(CPU.pc-1);
        CPU.pc += (offset);
        Bcycles_helper(oldcpu);
    }
#ifdef DEBUG_NESTEST
    printf(" %X ", offset);
    printf("  BVC $%X  ", CPU.pc);
#endif
    //CPU.clearFlags();
}
void BVS()
{
    CPU.pc++;
    uint16_t oldcpu = CPU.pc;
    uint8_t offset;
    if( CPU.getFlag('V'))
    {
        offset = RAM->read(CPU.pc-1);
        CPU.pc += (offset);
        Bcycles_helper(oldcpu);
    }
#ifdef DEBUG_NESTEST
    printf(" %X ", offset);
    printf("  BVS $%X  ", CPU.pc);
#endif
}
void CLC()
{
    //CPU.clearFlags();
    CPU.setFlag('C', false);
#ifdef DEBUG_NESTEST
    printf("     CLC     ");
#endif
}
void CLD()
{
    //CPU.clearFlags();
    CPU.setFlag('D', false);
}
void CLI()
{
    //CPU.clearFlags();
    CPU.setFlag('I', false);
}
void CLV()
{
    //CPU.clearFlags();
    CPU.setFlag('V', false);
}

void CMP_helper(uint8_t reg, uint16_t memloc)
{

    uint8_t value = RAM->read(memloc);
    int intermediate  = reg - value;
    //CPU.clearFlags();
    checkNegative(intermediate);
    checkZero(intermediate);
    checkCarrySubtraction( (value > reg) );
}

void CMP(uint16_t memloc)
{
    CMP_helper(CPU.A, memloc);
}

void CPX(uint16_t memloc)
{
    CMP_helper(CPU.X, memloc);
}

void CPY(uint16_t memloc)
{
    CMP_helper(CPU.Y, memloc);
}

void DEC(uint16_t memloc)
{
    uint8_t value = RAM->read(memloc) - 1;
    RAM->store(value, memloc);
    checkNegative(value);
    checkZero(value);

}

void DEX()
{
    CPU.X -= 1;
    checkNegative(CPU.X);
    checkZero(CPU.X);
}

void DEY()
{
    CPU.Y-=1;
    checkNegative(CPU.Y);
    checkZero(CPU.Y);
}

void EOR(uint16_t memloc)
{
    uint8_t value = RAM->read(memloc);
    CPU.A = CPU.A ^ value;
    //CPU.clearFlags();
    checkNegative(CPU.A);
    checkZero(CPU.A);
}

void INC(uint16_t memloc)
{
    uint8_t value = RAM->read(memloc)+1;
    RAM->store(value, memloc);
    //CPU.clearFlags();
    checkNegative(value);
    checkZero(value);
}

void INX()
{
    CPU.X +=1;
    //CPU.clearFlags();
    checkNegative(CPU.X);
    checkZero(CPU.X);
}

void INY()
{
    CPU.Y +=1;
    //CPU.clearFlags();
    checkNegative(CPU.Y);
    checkZero(CPU.Y);
}

void JMP()
{

    CPU.pc +=2;
    uint8_t low = RAM->read(CPU.pc-2);
    uint8_t high = RAM->read(CPU.pc-1);
    CPU.pc = ((uint16_t)high << 8 ) + low;
    #ifdef DEBUG_NESTEST
    printf("%X %X  ", low, high);
    printf("JMP $%X%X", high, low);
    #endif
}

void JMP_indirect()
{
    CPU.pc +=2;
    uint8_t low = RAM->read(CPU.pc-2);
    uint8_t high = RAM->read(CPU.pc-1);
    uint16_t addy = ((uint16_t)high << 8 ) + low;
/*
Indirect jump is bugged on the 6502, it doesn't add 1 to the full 16-bit value when it reads the second byte, it adds 1 to the low byte only. 
So JMP (03FF) reads from 3FF and 300, not 3FF and 400.

http://forums.nesdev.com/viewtopic.php?t=6621&start=15&sid=f9240730048f60854dcdfca400ba246f
*/

    uint16_t addy_plus_one =  (( addy & 0xff00) | ((( addy & 0x00ff) + 1 ) & 0x00ff));
    uint8_t high_1 = RAM->read(addy_plus_one); 
    uint8_t low_1 = RAM->read(addy);

    CPU.pc = ((uint16_t) high_1 << 8) + low_1;
#ifdef DEBUG_NESTEST
    printf("addy = %X , pc = %X ", addy, CPU.pc);
#endif
}

void JSR()
{

    CPU.pc +=1;
    push( (CPU.pc>>8) ); //HIGH FIRST THEN LOW??
    push( (CPU.pc & 0x00ff)); //
    uint8_t low = RAM->read(CPU.pc-1);
    uint8_t high = RAM->read(CPU.pc);
    CPU.pc = ((uint16_t)high << 8 ) + low;
#ifdef DEBUG_NESTEST
    printf(" %X %X  ", low, high);
    printf("JSR $%X%X", high, low);
#endif

} 

void LDA(uint16_t memloc )
{
   CPU.A = RAM->read(memloc);
#ifdef DEBUG_NESTEST
    printf(" LDA %04X ", memloc);
    printf("A = %X", CPU.A);
#endif
   checkNegative(CPU.A);
   checkZero(CPU.A);
}

void LDX(uint16_t memloc )
{
   CPU.X = RAM->read(memloc);
#ifdef DEBUG_NESTEST
    //printf("%2X", memloc);
    printf("     LDX  $%02X ", CPU.X);
#endif
   checkNegative(CPU.X);
   checkZero(CPU.X);
}


void LDY(uint16_t memloc )
{
   CPU.Y = RAM->read(memloc);
   //CPU.clearFlags();
   checkNegative(CPU.Y);
   checkZero(CPU.Y);
}

void LSR(uint16_t memloc)
{

   uint8_t value = RAM->read(memloc);

   char first_bit = value & 1;
   CPU.setFlag('C' , (bool)first_bit);
    value  = value >> 1;
    RAM->store(value, memloc);
    checkNegative(value);
    checkZero(value);

}
void LSR_accumulator()
{

   uint8_t value = CPU.A;

   char first_bit = value & 1;
   CPU.setFlag('C' , (bool)first_bit);
    value  = value >> 1;
    CPU.A = value;
    checkNegative(value);
    checkZero(value);

}

void ORA(uint16_t memloc)
{
    uint8_t value = RAM->read(memloc);
    CPU.A = CPU.A | value;
   //CPU.clearFlags();
    checkNegative(CPU.A);
    checkZero(CPU.A);
}

void PHA()
{
    
    push(CPU.A);
}

void PHP()
{
    push( (CPU.P | 0x10) ); // So it passses nestest test
}


void PLA()
{
    CPU.A =  pop();
    checkNegative(CPU.A);
    checkZero(CPU.A);
}

void PLP()
{
    CPU.P = pop();
    CPU.setFlag('B', false);
    CPU.P = CPU.P | 0x20; // set unused flag to always be one
    
}

void ROL(uint16_t memloc)
{
    //CPU.clearFlags();
    uint16_t value = RAM->read(memloc);
    value <<= 1;
    if ( CPU.getFlag('C'))
    {
        value |= 0x1;
    }
    CPU.setFlag('C', value > 0xff);
    value = value & 0xff;
    RAM->store(value, memloc);
    checkNegative(value);
    checkZero(value);
}

void ROL_accumulator() //accumuator
{
    //CPU.clearFlags();
    uint16_t value = CPU.A;
    value <<= 1;
    if ( CPU.getFlag('C'))
    {
        value |= 0x1;
    }
    CPU.setFlag('C', value > 0xff);
    value = value & 0xff;
    CPU.A = value;
    checkNegative(value);
    checkZero(value);
}
void ROR(uint16_t memloc)
{
    //CPU.clearFlags();
    uint16_t value = RAM->read(memloc);
    if ( CPU.getFlag('C'))
    {
        value |= 0x100;
    }
    uint8_t lowest_bit = value & 0x01;
    CPU.setFlag('C', (bool)lowest_bit);
    value = value >> 1;
    value &= 0xff;
    RAM->store( value, memloc);
    checkNegative(value );
    checkZero(value);
}

void ROR_accumulator()
{
    uint16_t value =  CPU.A;
    if ( CPU.getFlag('C'))
    {
        value |= 0x100;
    }
    char lowest_bit = value & 0x01;
    CPU.setFlag('C', (bool)lowest_bit);
    value = value >> 1;
    CPU.A = value & 0xff;
    checkNegative(value);
    checkZero(value);
}




void RTS()
{
    uint8_t low = pop();
    uint8_t high = pop();
    CPU.pc = ((uint16_t) high << 8) + low;
    CPU.pc++;
}
void RTI()
{
    CPU.P = pop();
    CPU.setFlag('B', false);
    CPU.P = CPU.P | 0x20; // set unused flag to always be one
    RTS();
    CPU.pc--;
}

void SBC(uint16_t memloc)
{
    uint8_t mem = RAM->read(memloc);
    uint8_t carry = (CPU.getFlag('C') ? 0 : 1);
    uint16_t  value =  CPU.A - mem - carry;
    //CPU.clearFlags();
    checkNegative(value);
    checkZero(value);
    checkOverflowSub(value, CPU.A, mem);
    checkCarrySubtraction((mem-carry) > CPU.A);
    CPU.A = value & 0xFF;
}

void SEC()
{
    CPU.setFlag('C', true);
#ifdef DEBUG_NESTEST
    printf("     SEC          ");
#endif
}

void SED()
{
    //CPU.clearFlags();
    CPU.setFlag('D', true);
}

void SEI()
{
    //CPU.clearFlags();
    CPU.setFlag('I', true);
}

void STA(uint16_t memloc)
{
#ifdef DEBUG_NESTEST
    printf("       STA  %X   ", memloc);
#endif

    RAM->store(CPU.A, memloc);
}

void STX(uint16_t memloc)
{
#ifdef DEBUG_NESTEST
    printf("       STX  %X   ", memloc);
#endif
    RAM->store(CPU.X, memloc);
}

void STY(uint16_t memloc)
{
    RAM->store(CPU.Y, memloc);
}

void TAX()
{
    CPU.X = CPU.A;
    //CPU.clearFlags();
    checkNegative(CPU.A);
    checkZero(CPU.A);

}
void TAY()
{
    CPU.Y = CPU.A;
    //CPU.clearFlags();
    checkNegative(CPU.A);
    checkZero(CPU.A);
}

void TSX()
{
    CPU.X = CPU.S;
    //CPU.clearFlags();
    checkNegative(CPU.X);
    checkZero(CPU.X);
}

void TXA()
{
    CPU.A = CPU.X;
    //CPU.clearFlags();
    checkNegative(CPU.A);
    checkZero(CPU.A);
}


void TXS()
{
    CPU.S = CPU.X;
}


void TYA()
{
    CPU.A = CPU.Y;
    //CPU.clearFlags();
    checkNegative(CPU.A);
    checkZero(CPU.A);
}

void nmi()
{
    uint8_t high = (CPU.pc >> 8) & 0xFF;
    uint8_t low = CPU.pc & 0xFF;

    push(high);
    push(low);
    push(CPU.P);

    high  = RAM->read(0xFFFB);
    low = RAM->read(0xFFFA);

    CPU.pc = ((uint16_t)high<<8) + low;
    CPU.request_nmi = false;
}

void PHP();
void SEI();
void BRK()
{
    CPU.pc ++;
    push(CPU.pc>>8);
    push(CPU.pc&0xff);
    //CPU.clearFlags();
    PHP();
    SEI();
    
    
    uint8_t high = RAM->read(0xffff);
    uint8_t low = RAM->read(0xfffe);
    CPU.pc = ((uint16_t)high << 8) + (uint16_t)low;
}
void irq()
{
    uint8_t high = CPU.pc >> 8;
    uint8_t low = CPU.pc & 0xFF;

    push(high);
    push(low);
    push(CPU.P);

    high  = RAM->read(0xFFFF);
    low = RAM->read(0xFFFE);

    CPU.pc = (uint16_t)high<<8 + low;
}

int step( uint8_t opcode ) 
{
    //printf(" N == %d\n\n", CPU.getFlag('N'));
    cycles = 0;
    if (DMA_WAIT >=0 ) {
        DMA_WAIT --;
        cycles = 1;
        return 0;
    }

    if ( CPU.request_nmi )
    {
        nmi();
        return 0 ;
    }
    else if ( CPU.irq_enable())
    {
        if(CPU.request_irq)
        {
            irq();
            return 0;
        }
    }

    CPU.pc++;

    switch (opcode)
    {
        //ADC - add to accululator with carry
        case 0x69: //immediate
            cycles += 2;
            ADC(immediateAddress());
            break;
        case 0x65: //z page
            cycles += 3;
            ADC(zPage());
            break;
        case 0x75: //zpage , X
            cycles += 4;
            ADC(zPageIndexed(CPU.X));
            break;
        case 0x6d: //absolute
            cycles += 4;
            ADC(absolute());
            break;
        case 0x7d: //Absolute,X
            cycles += 4;
            ADC(absoluteIndexed(CPU.X));
            break;
        case 0x79: //absolute y
            cycles += 4;
            ADC(absoluteIndexed(CPU.Y));
            break;
        case 0x61: //indexedIndirect
            cycles += 6;
            ADC(indexedIndirect(CPU.X));
            break;
        case 0x71:
            cycles += 5;
            ADC(indirectIndexed(CPU.Y));
            break;

        //AND memory with Accumulator
        case 0x29: //immediate
            cycles += 2;
            AND(immediateAddress());
            break;
        case 0x25:
            cycles += 3;
            AND(zPage());
            break;
        case 0x35:
            cycles += 4;
            AND(zPageIndexed(CPU.X));
            break;
        case 0x2d:
            cycles += 4;
            AND(absolute());
            break;
        case 0x3d:
            cycles += 4;
            AND(absoluteIndexed(CPU.X));
            break;
        case 0x39:
            cycles += 4;
            AND(absoluteIndexed(CPU.Y));
            break;
        case 0x21:
            cycles += 6;
            AND(indexedIndirect(CPU.X));
            break;
        case 0x31:
            cycles += 5;
            AND(indirectIndexed(CPU.Y));
            break;

        //ASL - arithmetic shift left
        case 0x0a: //accumulator
            cycles += 2;
            ASL_accumulator();
            break;
        case 0x06:
            cycles += 5;
            ASL(zPage());
            break;
        case 0x16:
            cycles += 6;
            ASL(zPageIndexed(CPU.X));
            break;
        case 0x0e:
            cycles += 6;
            ASL(absolute());
            break;
        case 0x1e:
            cycles += 7;
            ASL(absoluteIndexed(CPU.X));
            break;

        // BCC - branch on carry clear

        case 0x90:
            cycles += 2;
            BCC();
            break;

        //BCS - branch on Carry Set

        case 0xB0:
            cycles += 2;
            BCS();
            break;

        //BEQ Branch Zero Set

        case 0xf0:
            cycles += 2;
            BEQ();
            break;

        //BIT test bits in memory with accumulator
        case 0x24:
            cycles += 3;
            BIT(zPage());
            break;
        case 0x2c:
            cycles += 4;
            BIT(absolute());
            break;

        //BMI branch on result minus
        case 0x30:
            cycles += 2;
            BMI();
            break;

        //BNE - branch on Z reset
        case 0xD0:
            cycles += 2;
            BNE();
            break;

        //BPL - branch on result plus (or positive)
        case 0x10:
            cycles += 2;
            BPL();
            break;

        //BRK - forced break
        case  0x00:
            cycles += 7;
            BRK();
            break;

        //BVC - Branch on overflow clear
        case 0x50:
            cycles += 2;
            BVC();
            break;

        //BVS
        case 0x70:
            cycles += 2;
            BVS();
            break;

        //CLC
        case 0x18:
            cycles += 2;
            CLC();
            break;

        //CLI
        case 0x58:
            cycles +=2;
            CLI();
            break;

        //CLD
        case 0xD8:
            cycles += 2;
            CLD();
            break;

        //CLV
        case 0xB8:
            cycles += 2;
            CLV();
            break;
        
        // CMP - memory and accum
        case 0xC9:
            cycles += 2;
            CMP(immediateAddress());
            break;
        case 0xc5:
            cycles += 3;
            CMP(zPage());
            break;
        case 0xd5:
            cycles += 4;
            CMP(zPageIndexed(CPU.X));
            break;
        case 0xcd:
            cycles += 4;
            CMP(absolute());
            break;
        case 0xdd:
            cycles += 4;
            CMP(absoluteIndexed(CPU.X));
            break;
        case 0xd9:
            cycles += 4;
            CMP(absoluteIndexed(CPU.Y));
            break;
        case 0xc1:
            cycles += 6;
            CMP(indexedIndirect(CPU.X));
            break;
        case 0xd1:
            cycles += 5;
            CMP(indirectIndexed(CPU.Y));
            break;

        //CPX - memory and X
        case 0xe0:
            cycles += 2;
            CPX(immediateAddress());
            break;
        case 0xe4:
            cycles += 3;
            CPX(zPage());
            break;
        case 0xec:
            cycles += 4;
            CPX(absolute());
            break;

        //CPY - memory and Y
        case 0xC0:
            cycles += 2;
            CPY(immediateAddress());
            break;
        case 0xC4:
            cycles += 3;
            CPY(zPage());
            break;
        case 0xCC:
            cycles += 4;
            CPY(absolute());
            break;

        //DEC

        case 0xC6:
            cycles += 5;
            DEC(zPage());
            break;
        case 0xD6:
            cycles += 6;
            DEC(zPageIndexed(CPU.X));
            break;
        case 0xCE:
            cycles += 6;
            DEC(absolute());
            break;
        case 0xDE:
            cycles += 7;
            DEC(absoluteIndexed(CPU.X));
            break;

        //DEX - decrement X
        case 0xCA:
            cycles += 2;
            DEX();
            break;

        //DEY 
        case 0x88:
            cycles += 2;
            DEY();
            break;

        //EOR - XOR mem and Accum
        case 0x49:
            cycles += 2;
            EOR(immediateAddress());
            break;
        case 0x45:
            cycles += 3;
            EOR(zPage());
            break;
        case 0x55:
            cycles += 4;
            EOR(zPageIndexed(CPU.X));
            break;
        case 0x4D:
            cycles += 4;
            EOR(absolute());
            break;
        case 0x5D:
            cycles += 4;
            EOR(absoluteIndexed(CPU.X));
            break;
        case 0x59:
            cycles += 4;
            EOR(absoluteIndexed(CPU.Y));
            break;
        case 0x41:
            cycles += 6;
            EOR(indexedIndirect(CPU.X));
            break;
        case 0x51:
            cycles += 5;
            EOR(indirectIndexed(CPU.Y));
            break;

        //INC
        case 0xE6:
            cycles += 5;
            INC(zPage());
            break;
        case 0xF6:
            cycles += 6;
            INC(zPageIndexed(CPU.X));
            break;
        case 0xEE:
            cycles += 6;
            INC(absolute());
            break;
        case 0xFE:
            cycles += 7;
            INC(absoluteIndexed(CPU.X));
            break;

        //INX
        case 0xE8:
            cycles += 2;
            INX();
            break;

        //INY
        case 0xC8:
            cycles += 2;
            INY();
            break;

        //JMP
        case 0x4C:
            cycles += 3;
            JMP();
            break;
        case 0x6C:
            cycles += 5;
            JMP_indirect();
            break;
            

        //JSR
        case 0x20:
            cycles += 6;
            JSR();
            break;


        //LDA
        case 0xA9:
            cycles += 2;
            LDA(immediateAddress());
            break;
        case 0xA5:
            cycles += 3;
            LDA(zPage());
            break;
        case 0xB5:
            cycles += 4;
            LDA(zPageIndexed(CPU.X));
            break;
        case 0xAD:
            cycles += 4;
            LDA(absolute());
            break;
        case 0xBD:
            cycles += 4;
            LDA(absoluteIndexed(CPU.X));
            break;
        case 0xB9:
            cycles += 4;
            LDA(absoluteIndexed(CPU.Y));
            break;
        case 0xA1:
            cycles += 6;
            LDA(indexedIndirect(CPU.X));
            break;
        case 0xB1:
            cycles += 5;
            LDA(indirectIndexed(CPU.Y));
            break;
    
    

        //LDX


        case 0xA2:
            cycles += 2;
            LDX(immediateAddress());
            break;
        case 0xA6:
            cycles += 3;
            LDX(zPage());
            break;
        case 0xB6:
            cycles += 4;
            LDX(zPageIndexed(CPU.Y));
            break;
        case 0xAE:
            cycles += 4;
            LDX(absolute());
            break;
        case 0xBE:
            cycles += 4;
            LDX(absoluteIndexed(CPU.Y));
            break;

        
        //LDY

        case 0xA0:
            cycles += 2;
            LDY(immediateAddress());
            break;
        case 0xA4:
            cycles += 3;
            LDY(zPage());
            break;
        case 0xB4:
            cycles += 4;
            LDY(zPageIndexed(CPU.X));
            break;
        case 0xAC:
            cycles += 4;
            LDY(absolute());
            break;
        case 0xBC:
            cycles += 4;
            LDY(absoluteIndexed(CPU.X));
            break;



        //LSR
        case 0x4A:
            cycles += 2;
            LSR_accumulator();
            break;
        case 0x46:
            cycles += 5;
            LSR(zPage());
            break;
        case 0x56:
            cycles += 6;
            LSR(zPageIndexed(CPU.X));
            break;
        case 0x4E:
            cycles += 6;
            LSR(absolute());
            break;
        case 0x5e:
            cycles += 7;
            LSR(absoluteIndexed(CPU.X));
            break;


        //NOP
        case 0xEA:
            cycles += 2;
#ifdef DEBUG_NESTEST
            printf("    NOP           ");        
#endif
            break;

        //ORA

        case 0x09:
            cycles += 2;
            ORA(immediateAddress());
            break;
        case 0x05:
            cycles += 3;
            ORA(zPage());
            break;
        case 0x15:
            cycles += 4;
            ORA(zPageIndexed(CPU.X));
            break;
        case 0x0D:
            cycles += 4;
            ORA(absolute());
            break;
        case 0x1D:
            cycles += 4;
            ORA(absoluteIndexed(CPU.X));
            break;
        case 0x19:
            cycles += 4;
            ORA(absoluteIndexed(CPU.Y));
            break;
        case 0x01:
            cycles += 6;
            ORA(indexedIndirect(CPU.X));
            break;
        case 0x11:
            cycles += 5;
            ORA(indirectIndexed(CPU.Y));
            break;

        //PHP
        case 0x08:
            cycles += 3;
            PHP();
            break;

        //PLA
        case 0x68:
            cycles += 4;
            PLA();
            break;
        
        //PHA
        case 0x48:
            cycles +=3;
            PHA();
            break;

        //PLP
        case 0x28:
            cycles += 4;
            PLP();
            break;


        //ROL
        
        case 0x2A:
            cycles += 2;
            ROL_accumulator();
            break;
        case 0x26:
            cycles += 5;
            ROL(zPage());
            break;
        case 0x36:
            cycles += 6;
            ROL(zPageIndexed(CPU.X));
            break;
        case 0x2E:
            cycles += 6;
            ROL(absolute());
            break;
        case 0x3e:
            cycles += 7;
            ROL(absoluteIndexed(CPU.X));
            break;


        //ROR
        case 0x6A:
            cycles += 2;
            ROR_accumulator();
            break;
        case 0x66:
            cycles += 5;
            ROR(zPage());
            break;
        case 0x76:
            cycles += 6;
            ROR(zPageIndexed(CPU.X));
            break;
        case 0x6E:
            cycles += 6;
            ROR(absolute());
            break;
        case 0x7e:
            cycles += 7;
            ROR(absoluteIndexed(CPU.X));
            break;


        //RTI
        case 0x40:
            cycles += 6;
            RTI();
            break;

        //RTS
        case 0x60:
            cycles += 6;
            RTS();
            break;

        //SBC
        case 0xE9: //immediate
            cycles += 2;
            SBC(immediateAddress());
            break;
        case 0xE5: //z page
            cycles += 3;
            SBC(zPage());
            break;
        case 0xF5: //zpage , X
            cycles += 4;
            SBC(zPageIndexed(CPU.X));
            break;
        case 0xEd: //absolute
            cycles += 4;
            SBC(absolute());
            break;
        case 0xfd: //Absolute,X
            cycles += 4;
            SBC(absoluteIndexed(CPU.X));
            break;
        case 0xf9: //absolute y
            cycles += 4;
            SBC(absoluteIndexed(CPU.Y));
            break;
        case 0xE1: //indexedIndirect
            cycles += 6;
            SBC(indexedIndirect(CPU.X));
            break;
        case 0xF1:
            cycles += 5;
            SBC(indirectIndexed(CPU.Y));
            break;


        //SEC
        case 0x38:
            cycles += 2;
            SEC();
            break;


        //SED
        case 0xF8:
            cycles += 2;
            SED();
            break;

        //SEI
        case 0x78:
            cycles +=2;
            SEI();
            break;

        //STA

        case 0x85:
            cycles += 3;
            STA(zPage());
            break;
        case 0x95:
            cycles += 4;
            STA(zPageIndexed(CPU.X));
            break;
        case 0x8D:
            cycles += 4;
            STA(absolute());
            break;
        case 0x9D:
            cycles += 5;
            STA(absoluteIndexed(CPU.X));
            break;
        case 0x99:
            cycles += 5;
            STA(absoluteIndexed(CPU.Y));
            break;
        case 0x81:
            cycles += 6;
            STA(indexedIndirect(CPU.X));
            break;
        case 0x91:
            cycles += 6;
            STA(indirectIndexed(CPU.Y));
            break;

        //STX
        case 0x86:
            cycles += 3;
            STX(zPage());
            break;
        case 0x96:
            cycles += 4;
            STX(zPageIndexed(CPU.Y));
            break;
        case 0x8E:
            cycles += 4;
            STX(absolute());
            break;

        //STY
        case 0x84:
            cycles += 3;
            STY(zPage());
            break;
        case 0x94:
            cycles += 4;
            STY(zPageIndexed(CPU.X));
            break;
        case 0x8c:
            cycles += 4;
            STY(absolute());
            break;

        //TAX
        case 0xAA:
            cycles += 2;
            TAX();
            break;

        //TAY
        case 0xA8:
            cycles += 2;
            TAY();
            break;

        //TSX
        case 0xBA:
            cycles += 2;
            TSX();
            break;

        //TXA
        case 0x8A:
            cycles += 2;
            TXA();
            break;

        //TXS
        case 0x9A:
            cycles += 2;
            TXS();
            break;
        //TYA
        case 0x98:
            cycles += 2;
            TYA();
            break;



        default:
                printf( "INVALID OPCODE 0x%x \n", opcode);
                return 1;
                
                 break;
        }

        //sleep 
        return 0;
}


void * run(void* ptr){
#ifdef DEBUG_NESTEST
    int total_cycles=0;
    int sl = 241;
#endif
#ifdef DEBUG_INSTR_TEST
        int total_cycles = 0;
#endif
        int total_cycles = 0;
    while ( true)
    {
#ifdef DEBUG_NESTEST
        printf("%04X  %02X", CPU.pc, RAM->read(CPU.pc));
        uint8_t old_CPU_A = CPU.A;
        uint8_t old_CPU_X = CPU.X;
        uint8_t old_CPU_Y = CPU.Y;
        uint8_t old_CPU_P = CPU.P;
        uint8_t old_CPU_S = CPU.S;
#endif

        int error = step(RAM->read(CPU.pc));
        //one cpu cycles is three ppu cycles
        if(error)
            return (void*)1;
        for ( int i = 0; i < 3*cycles; i++)
        {
            PPU->step();
        }


#ifdef DEBUG_INSTR_TEST
        total_cycles += cycles ;
        if (total_cycles > 50000)
        {
            char c;
            for(int i =0; i < 0x2000;i++)
            {   
                
                printf("%X :  %X    ", i, RAM->read(i));
                if( i % 10 == 0) printf("\n");
            }
            exit(1);
        }
#endif

#ifdef DEBUG_NESTEST
        printf("         A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d SL:%d \n", old_CPU_A, old_CPU_X, old_CPU_Y, old_CPU_P, old_CPU_S, total_cycles, sl);
        total_cycles += 3*cycles;
        if( total_cycles >= 341)
        {
            total_cycles -= 341;
            sl++;
        }
        if ( sl >= 261) sl = -1;

#endif
    }
}
/*
unsigned int data[] = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E,
    0x6E0040, 0x6C0600, 0x561D00, 0x333500, 0x0B4800,
    0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000,
    0x000000, 0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE,
    0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00, 0x6B6D00,
    0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000,
    0x000000, 0x000000, 0xFFFEFF, 0x64B0FF, 0x9290FF,
    0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
    0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE,
    0x4F4F4F, 0x000000, 0x000000, 0xFFFEFF, 0xC0DFFF,
    0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5,
    0xF7D8A5, 0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC,
    0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000,
};
*/

void DisplayFunc()
{


    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (0, 1, 1, 0, -1, 1);
    glMatrixMode (GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT);

    //printf("color = %X\n", PPU->rgb_framebuffer[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WINDOW_SIZE_X, WINDOW_SIZE_Y, 0,  GL_RGB,GL_UNSIGNED_BYTE, PPU->rgb_framebuffer);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
        glTexCoord2f(0.0, 1.0); glVertex2f(0.0, 1.0);
        glTexCoord2f(1.0, 1.0); glVertex2f(1.0, 1.0);
        glTexCoord2f(1.0, 0.0); glVertex2f(1.0, 0.0);
    glEnd();


    glFlush();
}
void timerFunc(int a)
{



    glutPostRedisplay();
    glutTimerFunc(TIMER_FUNC_FREQ,timerFunc,100);
}

int main(int argc, char * argv[])
{

    if (argc >= 2)
    {
        int FD = open(argv[1], O_RDONLY);
        if( FD == -1 ) 
        {
            printf("invalid File name : %s \n", argv[1]);
            exit(1);
        }
        ROM->loadROM(FD);
        switch (ROM->mapper_number)
        {
            case 0: 
                RAM = new ram();
                break;

            case 1:
                RAM = new mmc1();
                break;
            default:
                printf("error mapper %d not implemented yet\n\n", ROM->mapper_number);
                exit(42);
        }
        ROM->setupRam();

    }
    if (argc == 3)
    {

        //cpu starts executing at addy in $FFFC and $FFFD
        uint8_t low;
        uint8_t high;
        high = RAM->read(0xFFFD);
        low = RAM->read(0xFFFC);

        if( !strcmp(argv[1], "nestest.nes"))
        {
            CPU.pc = 0xC000; 
        }else{
            CPU.pc = ((uint16_t) high << 8) + (uint16_t)low; 
        }
        CPU.S = 0xFD;
        CPU.P = 0x34;
        pthread_t cpu_thread;

        pthread_create(&cpu_thread, NULL, run, NULL );;


        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH);
        glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-WINDOW_SIZE_X)/2,
                (glutGet(GLUT_SCREEN_HEIGHT)-WINDOW_SIZE_Y)/2);
        glutInitWindowSize(WINDOW_SIZE_X, WINDOW_SIZE_Y);
        glutCreateWindow("MyNes");
        glutDisplayFunc(DisplayFunc);
        //glutReshapeFunc(ReshapeFunc);
        //glutMouseFunc(mouse);
        //glutMotionFunc(motion);
        //glutIdleFunc(DisplayFunc);
        glDisable(GL_DEPTH_TEST);
            glutTimerFunc(TIMER_FUNC_FREQ,timerFunc,100);

        glEnable(GL_TEXTURE_2D);
#ifndef GRAPHICS_OFF
       glutMainLoop();
#endif
        pthread_join(cpu_thread, NULL);

    }


    return 0;

}
