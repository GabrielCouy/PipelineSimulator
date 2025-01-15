/*
 * EECS 370, University of Michigan, Fall 2023
 * Project 3: LC-2K Pipeline Simulator
 * Instructions are found in the project spec: https://eecs370.github.io/project_3_spec/
 * Make sure NOT to modify printState or any of the associated functions
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Machine Definitions
#define NUMMEMORY 65536 // maximum number of data words in memory
#define NUMREGS 8 // number of machine registers

#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 // will not implemented for Project 3
#define HALT 6
#define NOOP 7

const char* opcode_to_str_map[] = {
    "add",
    "nor",
    "lw",
    "sw",
    "beq",
    "jalr",
    "halt",
    "noop"
};

#define NOOPINSTR (NOOP << 22)

typedef struct IFIDStruct {
	int pcPlus1;
	int instr;
} IFIDType;

typedef struct IDEXStruct {
	int pcPlus1;
	int valA;
	int valB;
	int offset;
	int instr;
} IDEXType;

typedef struct EXMEMStruct {
	int branchTarget;
    int eq;
	int aluResult;
	int valB;
	int instr;
} EXMEMType;

typedef struct MEMWBStruct {
    int ALU;
	int writeData;
    int instr;
} MEMWBType;

typedef struct WBENDStruct {
	int writeData;
	int instr;
} WBENDType;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	unsigned int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	unsigned int cycles; // number of cycles run so far
} stateType;

static inline int opcode(int instruction) {
    return instruction>>22;
}

static inline int field0(int instruction) {
    return (instruction>>19) & 0x7;
}

static inline int field1(int instruction) {
    return (instruction>>16) & 0x7;
}

static inline int field2(int instruction) {
    return instruction & 0xFFFF;
}

// convert a 16-bit number into a 32-bit Linux integer
static inline int convertNum(int num) {
    return num - ( (num & (1<<15)) ? 1<<16 : 0 );
}

void printState(stateType*);
void printInstruction(int);
void readMachineCode(stateType*, char*);


int main(int argc, char *argv[]) {

    /* Declare state and newState.
       Note these have static lifetime so that instrMem and
       dataMem are not allocated on the stack. */

    static stateType state, newState;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    readMachineCode(&state, argv[1]);

    // Initialize state here
    state.IFID.instr = 0x1c00000;
    state.IDEX.instr = 0x1c00000;
    state.EXMEM.instr = 0x1c00000;
    state.MEMWB.instr = 0x1c00000;
    state.WBEND.instr = 0x1c00000;

    newState = state;

    
    int c =0;
    while (opcode(state.MEMWB.instr) != HALT) {
        printState(&state);

        newState.cycles += 1;

        /* ---------------------- IF stage --------------------- */
        
        if(opcode(state.EXMEM.instr) == 2)
        {
            if(field1(state.EXMEM.instr) == field0(state.IDEX.instr))
            {
                newState.IFID.instr = state.instrMem[state.pc -1 ];
                newState.IFID.pcPlus1 = state.pc;
            }else if(field1(state.EXMEM.instr) == field0(state.IDEX.instr) && opcode(state.IDEX.instr) != 2)
            {
                newState.IFID.instr = state.instrMem[state.pc -1];
                newState.IFID.pcPlus1 = state.pc;
            }
        }else{
            newState.IFID.instr = state.instrMem[state.pc];
            newState.IFID.pcPlus1 = state.pc + 1;
            if(newState.cycles ==1)
            {
            newState.pc++;
            }
        }
       
       

        /* ---------------------- ID stage --------------------- */

        if(newState.cycles > 1)
        {
           
            if(opcode(state.EXMEM.instr) == 2)
            {
                if(field1(state.EXMEM.instr) == field0(state.IDEX.instr))
                {
                    newState.IDEX.instr = state.IDEX.instr;
                    newState.pc = state.pc;
                }else if(field1(state.EXMEM.instr) == field0(state.IDEX.instr) && opcode(state.IDEX.instr) != 2)
                {
                    newState.IDEX.instr = state.IDEX.instr;
                    newState.pc = state.pc;
                }
            }else{
                newState.IDEX.instr = state.IFID.instr;
                newState.IDEX.valA = state.reg[field0(state.IFID.instr)];
                newState.IDEX.valB = state.reg[field1(state.IFID.instr)];
                int of = field2(state.IFID.instr);
                newState.IDEX.offset = convertNum(of);
                newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
                newState.pc++;
            }
            
        }


        /* ---------------------- EX stage --------------------- */
        if(newState.cycles >2)
        {
            //stall
            newState.EXMEM.instr = state.IDEX.instr;
            if(opcode(state.EXMEM.instr) == 2)
            {
                if(field1(state.EXMEM.instr) == field0(state.IDEX.instr))
                {
                    newState.EXMEM.instr = 0x1c00000;
                }else if(field1(state.EXMEM.instr) == field0(state.IDEX.instr) && opcode(state.IDEX.instr) != 2)
                {
                    newState.EXMEM.instr = 0x1c00000;
                }
            }
            else if(opcode(state.IDEX.instr) == 2 )  // lw
            {
                if(opcode(state.EXMEM.instr) == 2)
                {
                    if(field1(state.EXMEM.instr) == field0(state.IDEX.instr))
                    {
                        newState.EXMEM.instr = 0x1c00000;
                    }
                }else if(opcode(state.EXMEM.instr < 2))
                {
                    if(field0(state.IDEX.instr) == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.EXMEM.aluResult;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                    }
                }else if(opcode(state.MEMWB.instr) < 2)
                {
                    if(field0(state.IDEX.instr) == field2(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.MEMWB.writeData;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                    }
                }else if(opcode(state.MEMWB.instr) == 2){
                    if(field0(state.IDEX.instr) == field1(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.MEMWB.writeData;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                    }
                }else{
                    newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                }
            }else if(opcode(state.IDEX.instr) == 0) // add
            {
                if(opcode(state.EXMEM.instr) == 2)
                {
                    if(field1(state.EXMEM.instr) == field0(state.IDEX.instr) || field1(state.EXMEM.instr) == field1(state.IDEX.instr) )
                    {
                        newState.EXMEM.instr = 0x1c00000;
                    }
                }else if(opcode(state.EXMEM.instr < 2))
                {
                    if(field0(state.IDEX.instr) == field2(state.EXMEM.instr) && field1(state.IDEX.instr)  == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = state.EXMEM.aluResult + state.EXMEM.aluResult;
                    }
                    else if(field0(state.IDEX.instr) == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = state.EXMEM.aluResult + state.IDEX.valB;
                    }else if(field1(state.IDEX.instr)  == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = state.EXMEM.aluResult + state.IDEX.valA;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.valB;
                    }
                }else if(opcode(state.MEMWB.instr) < 2)
                {
                    if(field0(state.IDEX.instr) == field2(state.MEMWB.instr) && field1(state.IDEX.instr)  == field2(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = state.MEMWB.writeData + state.MEMWB.writeData ;
                    }
                    else if(field0(state.IDEX.instr) == field2(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = state.MEMWB.writeData + state.IDEX.valB;
                    }else if (field1(state.IDEX.instr)  == field2(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = state.MEMWB.writeData + state.IDEX.valA;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.valB;
                    }
                }else if(opcode(state.MEMWB.instr) ==2)
                {
                    if(field1(state.MEMWB.instr) == field0(state.IDEX.instr) && field1(state.MEMWB.instr) == field1(state.IDEX.instr) )
                    {
                        newState.EXMEM.aluResult = state.MEMWB.writeData + state.MEMWB.writeData ;
                    }
                    else if(field1(state.MEMWB.instr) == field0(state.IDEX.instr) )
                    {
                        newState.EXMEM.aluResult = state.MEMWB.writeData + state.IDEX.valB;
                    }else if(field1(state.MEMWB.instr) == field1(state.IDEX.instr))
                    {
                        newState.EXMEM.aluResult = state.MEMWB.writeData + state.IDEX.valA;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.valB;
                    }
                }
                else{
                    newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.valB;
                }
                
            }else if(opcode(state.IDEX.instr) == 1) // nor
            {
                if(opcode(state.EXMEM.instr) == 2)
                {
                    if(field1(state.EXMEM.instr) == field0(state.IDEX.instr) || field1(state.EXMEM.instr) == field1(state.IDEX.instr) )
                    {
                        newState.EXMEM.instr = 0x1c00000;
                    }
                }else if(opcode(state.EXMEM.instr < 2))
                {

                    if(field0(state.IDEX.instr) == field2(state.EXMEM.instr) && field1(state.IDEX.instr)  == field2(state.EXMEM.instr) )
                    {
                        newState.EXMEM.aluResult = ~(state.EXMEM.aluResult | state.EXMEM.aluResult);
                    }else if(field0(state.IDEX.instr) == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = ~(state.EXMEM.aluResult | state.IDEX.valB);
                    }else if(field1(state.IDEX.instr)  == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult =  ~(state.EXMEM.aluResult | state.IDEX.valA);
                    }else{
                       newState.EXMEM.aluResult = ~(state.IDEX.valA | state.IDEX.valB);
                    }
                }else if(opcode(state.MEMWB.instr) < 2)
                {
                    if(field0(state.IDEX.instr) == field2(state.MEMWB.instr) && field1(state.IDEX.instr)  == field2(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = ~(state.MEMWB.writeData | state.MEMWB.writeData);
                    }if(field0(state.IDEX.instr) == field2(state.MEMWB.instr))
                    {
                         newState.EXMEM.aluResult = ~(state.MEMWB.writeData | state.IDEX.valB);
                    }else if (field1(state.IDEX.instr)  == field2(state.MEMWB.instr))
                    {
                         newState.EXMEM.aluResult = ~(state.MEMWB.writeData | state.IDEX.valA);                        
                    }else{
                        newState.EXMEM.aluResult = ~(state.IDEX.valA | state.IDEX.valB);
                    }
                    
                }else if(opcode(state.MEMWB.instr) ==2)
                {
                    if(field1(state.MEMWB.instr) == field0(state.IDEX.instr) && field1(state.MEMWB.instr) == field1(state.IDEX.instr) )
                    {
                       newState.EXMEM.aluResult = ~(state.MEMWB.writeData | state.MEMWB.writeData) ;
                    }
                    else if(field1(state.MEMWB.instr) == field0(state.IDEX.instr) )
                    {
                        newState.EXMEM.aluResult = ~(state.MEMWB.writeData | state.IDEX.valB);
                    }else if(field1(state.MEMWB.instr) == field1(state.IDEX.instr))
                    {
                       newState.EXMEM.aluResult = ~(state.MEMWB.writeData | state.IDEX.valA);   
                    }else{
                        newState.EXMEM.aluResult = ~(state.IDEX.valA | state.IDEX.valB);
                    }
                }
                else{
                    newState.EXMEM.aluResult = ~(state.IDEX.valA | state.IDEX.valB);
                }
                
            }else if(opcode(state.IDEX.instr) == 3) // sw
            {
                if(opcode(state.EXMEM.instr) == 2)
                {
                    if(field1(state.EXMEM.instr) == field0(state.IDEX.instr) || field1(state.EXMEM.instr) == field1(state.IDEX.instr) )
                    {
                        newState.EXMEM.instr = 0x1c00000;
                    }
                }else if(opcode(state.EXMEM.instr < 2))
                {
                    if(field0(state.IDEX.instr) == field2(state.EXMEM.instr) && field1(state.IDEX.instr)  == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.EXMEM.aluResult;
                        newState.EXMEM.valB = state.EXMEM.aluResult;
                    }else if(field0(state.IDEX.instr) == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.EXMEM.aluResult;
                        newState.EXMEM.valB = state.IDEX.valB;
                    }else if(field1(state.IDEX.instr)  == field2(state.EXMEM.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                        newState.EXMEM.valB = state.EXMEM.aluResult;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                        newState.EXMEM.valB = state.IDEX.valB;
                    }
                }else if(opcode(state.MEMWB.instr) < 2)
                {
                    if(field0(state.IDEX.instr) == field2(state.MEMWB.instr) && field1(state.IDEX.instr)  == field2(state.MEMWB.instr) )
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.MEMWB.writeData;
                        newState.EXMEM.valB = state.MEMWB.writeData;
                    }
                    if(field0(state.IDEX.instr) == field2(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.MEMWB.writeData;
                        newState.EXMEM.valB = state.IDEX.valB;
                    }else if (field1(state.IDEX.instr)  == field2(state.MEMWB.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                        newState.EXMEM.valB = state.MEMWB.writeData;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                        newState.EXMEM.valB = state.IDEX.valB;
                    }
                    
                }
                else if (opcode(state.MEMWB.instr) ==2){
                    if(field1(state.MEMWB.instr) == field0(state.IDEX.instr) && field1(state.MEMWB.instr) == field1(state.IDEX.instr) )
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.MEMWB.writeData;
                        newState.EXMEM.valB = state.MEMWB.writeData;
                    }else if(field1(state.MEMWB.instr) == field0(state.IDEX.instr) )
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.MEMWB.writeData;
                        newState.EXMEM.valB = state.IDEX.valB;
                    }else if(field1(state.MEMWB.instr) == field1(state.IDEX.instr))
                    {
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                        newState.EXMEM.valB = state.MEMWB.writeData;
                    }else{
                        newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                        newState.EXMEM.valB = state.IDEX.valB;
                    }

                }else{
                    newState.EXMEM.aluResult = state.IDEX.offset +state.IDEX.valA;
                    newState.EXMEM.valB = state.IDEX.valB;
                }
            }else if(opcode(state.IDEX.instr) == 4)
            {
                newState.EXMEM.eq = 0;
                newState.EXMEM.branchTarget = state.IDEX.offset + state.IDEX.pcPlus1;
                //newState.EXMEM.instr = state.IDEX.instr;
                if(opcode(state.EXMEM.instr) == 2)
                {
                    if(field1(state.EXMEM.instr) == field0(state.IDEX.instr) || field1(state.EXMEM.instr) == field1(state.IDEX.instr) )
                    {
                        newState.EXMEM.instr = 0x1c00000;
                    }
                }else if(opcode(state.EXMEM.instr < 2))
                {
                    if(field0(state.IDEX.instr) == field2(state.EXMEM.instr))
                    {
                        if(state.EXMEM.aluResult == state.IDEX.valB)
                        {
                             newState.EXMEM.eq = 1;
                        }
                    }else if(field1(state.IDEX.instr) == field2(state.EXMEM.instr))
                    {
                        if(state.EXMEM.aluResult == state.IDEX.valA)
                        {
                             newState.EXMEM.eq = 1;
                        }
                    }else{
                        if(state.IDEX.valA == state.IDEX.valB)
                        {
                            newState.EXMEM.eq = 1;
                        }
                    }
                }else if((opcode(state.MEMWB.instr) < 2))
                {
                    if(field0(state.IDEX.instr) == field2(state.MEMWB.instr))
                    {
                        if(state.MEMWB.writeData == state.IDEX.valB)
                        {
                             newState.EXMEM.eq = 1;
                        }
                    }else if(field1(state.IDEX.instr)  == field2(state.MEMWB.instr))
                    {
                        if(state.MEMWB.writeData == state.IDEX.valA)
                        {
                            newState.EXMEM.eq = 1;
                        }
                    }else{
                        if(state.IDEX.valA == state.IDEX.valB)
                        {
                            newState.EXMEM.eq = 1;
                        }
                    }
                }else if(opcode(state.MEMWB.instr) == 2)
                {
                    if(field1(state.MEMWB.instr) == field0(state.IDEX.instr))
                    {
                        if(state.MEMWB.writeData == state.IDEX.valB)
                        {
                            newState.EXMEM.eq = 1;
                        }
                    }else if(field1(state.MEMWB.instr) == field1(state.IDEX.instr))
                    {
                        if(state.MEMWB.writeData == state.IDEX.valA)
                        {
                            newState.EXMEM.eq = 1;
                        }
                    }else{
                        if(state.IDEX.valA == state.IDEX.valB)
                        {
                            newState.EXMEM.eq = 1;
                        }
                    }
                }else{
                    if(state.IDEX.valA == state.IDEX.valB)
                    {
                        newState.EXMEM.eq = 1;
                    }
                }
                
            }
        }

        /* --------------------- MEM stage --------------------- */

        if(newState.cycles > 3)
        {
            
            newState.MEMWB.instr = state.EXMEM.instr;
            if(opcode(state.EXMEM.instr) == 4)
            {
                if(state.EXMEM.eq ==1)
                {
                    newState.pc = state.EXMEM.branchTarget;
                    newState.EXMEM.instr = 0x1c00000;
                    newState.IDEX.instr = 0x1c00000;
                    newState.IFID.instr = 0x1c00000;
                }
            }
            if(opcode(state.EXMEM.instr) == 0 || opcode(state.EXMEM.instr) == 1)
            {
                newState.MEMWB.writeData = state.EXMEM.aluResult;
            }
            if(opcode(state.EXMEM.instr) == 2)
            {
                newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
            }
            if(opcode(state.EXMEM.instr) ==3)
            {
                newState.dataMem[state.EXMEM.aluResult]=  state.EXMEM.valB;//state.reg[field1(state.EXMEM.instr)];
            }
            
        
        }
        /* ---------------------- WB stage --------------------- */

        if(newState.cycles > 4)
        {

            newState.WBEND.instr = state.MEMWB.instr;
            if(opcode(state.MEMWB.instr) ==0 ||opcode(state.MEMWB.instr) ==1 )
            {
                newState.WBEND.writeData = state.MEMWB.writeData;
                newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
            }else if(opcode(state.MEMWB.instr) == 2)
            {
                newState.WBEND.writeData = state.MEMWB.writeData;
                newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;
            }
        }
        c++;
        //newState.pc++;
        
        /* ------------------------ END ------------------------ */
        state = newState; /* this is the last statement before end of the loop. It marks the end
        of the cycle and updates the current state with the values calculated in this cycle */
    }
    printf("Machine halted\n");
    printf("Total of %d cycles executed\n", state.cycles);
    printf("Final state of machine:\n");
    printState(&state);
}

/*
* DO NOT MODIFY ANY OF THE CODE BELOW.
*/

void printInstruction(int instr) {
    const char* instr_opcode_str;
    int instr_opcode = opcode(instr);
    if(ADD <= instr_opcode && instr_opcode <= NOOP) {
        instr_opcode_str = opcode_to_str_map[instr_opcode];
    }

    switch (instr_opcode) {
        case ADD:
        case NOR:
        case LW:
        case SW:
        case BEQ:
            printf("%s %d %d %d", instr_opcode_str, field0(instr), field1(instr), convertNum(field2(instr)));
            break;
        case JALR:
            printf("%s %d %d", instr_opcode_str, field0(instr), field1(instr));
            break;
        case HALT:
        case NOOP:
            printf("%s", instr_opcode_str);
            break;
        default:
            printf(".fill %d", instr);
            return;
    }
}

void printState(stateType *statePtr) {
    printf("\n@@@\n");
    printf("state before cycle %d starts:\n", statePtr->cycles);
    printf("\tpc = %d\n", statePtr->pc);

    printf("\tdata memory:\n");
    for (int i=0; i<statePtr->numMemory; ++i) {
        printf("\t\tdataMem[ %d ] = %d\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (int i=0; i<NUMREGS; ++i) {
        printf("\t\treg[ %d ] = %d\n", i, statePtr->reg[i]);
    }

    // IF/ID
    printf("\tIF/ID pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IFID.instr);
    printInstruction(statePtr->IFID.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IFID.pcPlus1);
    if(opcode(statePtr->IFID.instr) == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");

    // ID/EX
    int idexOp = opcode(statePtr->IDEX.instr);
    printf("\tID/EX pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->IDEX.instr);
    printInstruction(statePtr->IDEX.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IDEX.pcPlus1);
    if(idexOp == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegA = %d", statePtr->IDEX.valA);
    if (idexOp >= HALT || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->IDEX.valB);
    if(idexOp == LW || idexOp > BEQ || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\toffset = %d", statePtr->IDEX.offset);
    if (idexOp != LW && idexOp != SW && idexOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // EX/MEM
    int exmemOp = opcode(statePtr->EXMEM.instr);
    printf("\tEX/MEM pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->EXMEM.instr);
    printInstruction(statePtr->EXMEM.instr);
    printf(" )\n");
    printf("\t\tbranchTarget %d", statePtr->EXMEM.branchTarget);
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\teq ? %s", (statePtr->EXMEM.eq ? "True" : "False"));
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\taluResult = %d", statePtr->EXMEM.aluResult);
    if (exmemOp > SW || exmemOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->EXMEM.valB);
    if (exmemOp != SW) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // MEM/WB
	int memwbOp = opcode(statePtr->MEMWB.instr);
    printf("\tMEM/WB pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->MEMWB.instr);
    printInstruction(statePtr->MEMWB.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->MEMWB.writeData);
    if (memwbOp >= SW || memwbOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // WB/END
	int wbendOp = opcode(statePtr->WBEND.instr);
    printf("\tWB/END pipeline register:\n");
    printf("\t\tinstruction = %d ( ", statePtr->WBEND.instr);
    printInstruction(statePtr->WBEND.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->WBEND.writeData);
    if (wbendOp >= SW || wbendOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    printf("end state\n");
    fflush(stdout);
}

// File
#define MAXLINELENGTH 1000 // MAXLINELENGTH is the max number of characters we read

void readMachineCode(stateType *state, char* filename) {
    char line[MAXLINELENGTH];
    FILE *filePtr = fopen(filename, "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", filename);
        exit(1);
    }

    printf("instruction memory:\n");
    for (state->numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; ++state->numMemory) {
        if (sscanf(line, "%d", state->instrMem+state->numMemory) != 1) {
            printf("error in reading address %d\n", state->numMemory);
            exit(1);
        }
        printf("\tinstrMem[ %d ]\t= 0x%08x\t= %d\t= ", state->numMemory, 
            state->instrMem[state->numMemory], state->instrMem[state->numMemory]);
        printInstruction(state->dataMem[state->numMemory] = state->instrMem[state->numMemory]);
        printf("\n");
    }
}
