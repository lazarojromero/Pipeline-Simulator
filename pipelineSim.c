#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMMEMORY 16 /* Maximum number of data words in memory */
#define NUMREGS 8    /* Number of registers */

/* Opcode values for instructions */
#define R 0
#define LW 35
#define SW 43
#define BNE 4
#define HALT 63

/* Funct values for R-type instructions */
#define ADD 32
#define SUB 34

/* Branch Prediction Buffer Values */
#define STRONGLYTAKEN 3
#define WEAKLYTAKEN 2
#define WEAKLYNOTTAKEN 1
#define STRONGLYNOTTAKEN 0

typedef struct IFIDStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
} IFIDType;

typedef struct IDEXStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
  int readData1;                   /* Contents of rs register */
  int readData2;                   /* Contents of rt register */
  int immed;                       /* Immediate field */
  int rsReg;                       /* Number of rs register */
  int rtReg;                       /* Number of rt register */
  int rdReg;                       /* Number of rd register */
  int branchTarget;                /* Branch target, obtained from immediate field */
} IDEXType;

typedef struct EXMEMStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int aluResult;                   /* Result of ALU operation */
  int writeDataReg;                /* Contents of the rt register, used for store word */
  int writeReg;                    /* The destination register */
} EXMEMType;

typedef struct MEMWBStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int writeDataMem;                /* Data read from memory */
  int writeDataALU;                /* Result from ALU operation */
  int writeReg;                    /* The destination register */
} MEMWBType;

typedef struct stateStruct {
  int PC;                                 /* Program Counter */
  unsigned int instrMem[NUMMEMORY];       /* Instruction memory */
  int dataMem[NUMMEMORY];                 /* Data memory */
  int regFile[NUMREGS];                   /* Register file */
  IFIDType IFID;                          /* Current IFID pipeline register */
  IDEXType IDEX;                          /* Current IDEX pipeline register */
  EXMEMType EXMEM;                        /* Current EXMEM pipeline register */
  MEMWBType MEMWB;                        /* Current MEMWB pipeline register */
  int cycles;                             /* Number of cycles executed so far */
} stateType;


void run();
void printState(stateType*);
void initState(stateType*);
unsigned int instrToInt(char*, char*);
int get_opcode(unsigned int);
void printInstruction(unsigned int);

int main(){
    run();
    return(0); 
}

int get_rs(unsigned int instruction){
    return( (instruction>>21) & 0x1F);
}

int get_rt(unsigned int instruction){
    return( (instruction>>16) & 0x1F);
}

int get_rd(unsigned int instruction){
    return( (instruction>>11) & 0x1F);
}

int get_funct(unsigned int instruction){
    return(instruction & 0x3F);
}

int get_immed(unsigned int instruction){
    return(instruction & 0xFFFF);
}

int get_opcode(unsigned int instruction){
    return(instruction>>26);
}

void run(){

  stateType state;           /* Contains the state of the entire pipeline before the cycle executes */ 
  stateType newState;        /* Contains the state of the entire pipeline after the cycle executes */
  initState(&state);         /* Initialize the state of the pipeline */
	int numStalls = 0;//count number of stalls needed throughout program
	int numBranches = 0;
	int numMisPred = 0;
    
    while (1) {

        printState(&state);

	/* If a halt instruction is entering its WB stage, then all of the legitimate */
	/* instruction have completed. Print the statistics and exit the program. */
        if (get_opcode(state.MEMWB.instr) == HALT) {
            printf("Total number of cycles executed: %d\n", state.cycles);
            printf("Total number of stalls: %d\n", numStalls);
            printf("Total number of branches: %d\n", numBranches);
            printf("Total number of mispredicted branches: %d\n", numMisPred);
            /* Remember to print the number of stalls, branches, and mispredictions! */
            exit(0);
        }

        newState = state;     /* Start by making newState a copy of the state before the cycle */
        newState.cycles++;

	/* Modify newState stage-by-stage below to reflect the state of the pipeline after the cycle has executed */

        /* --------------------- IF stage --------------------- */

        newState.PC = ((state.PC) + 4);//set newState's PC to oldState's PC plus 4 for next instr 
        
        newState.IFID.instr = state.instrMem[((state.PC) / 4)];//PC divide by 4 since data memory is sequential
        
        newState.IFID.PCPlus4 = ((state.PC) + 4);

        /* --------------------- ID stage --------------------- */   

        int memRead = 0;

        if(get_opcode(state.IDEX.instr) == LW) {

        	memRead = 1;

        }

        if((memRead  != 0) && ((state.IDEX.rtReg == get_rs(state.IFID.instr)) || (state.IDEX.rtReg == get_rt(state.IFID.instr)))) {

        	printf("\nStall Pipeline\n");

            newState.IDEX.instr = 0; //flush cycle

                newState.IFID.instr = state.IFID.instr;

                newState.IFID.PCPlus4 = state.IFID.PCPlus4;\

                newState.PC = state.PC;

            numStalls++;

        } else {

                newState.IDEX.instr = state.IFID.instr; 
                
        }

        if(get_opcode(newState.EXMEM.instr) != HALT) {
        
        	newState.IDEX.PCPlus4 = state.IFID.PCPlus4;  

        	newState.IDEX.readData1 = newState.regFile[get_rs(newState.IDEX.instr)];
                       
        	newState.IDEX.readData2 = newState.regFile[get_rt(newState.IDEX.instr)];

        	newState.IDEX.rsReg = get_rs(newState.IDEX.instr);
                       
        	newState.IDEX.rtReg = get_rt(newState.IDEX.instr);

            	if(get_opcode(newState.IDEX.instr) == R) {

            		newState.IDEX.immed = ((newState.IDEX.instr)& 0x0000FFFF);

               		newState.IDEX.rdReg = get_rd(newState.IDEX.instr);
                       
                	newState.IDEX.branchTarget = ((newState.IDEX.instr)& 0x0000FFFF);

				} else {

					newState.IDEX.immed = get_immed(newState.IDEX.instr);
                       
                	newState.IDEX.rdReg = 0;
                       
                	newState.IDEX.branchTarget = get_immed(newState.IDEX.instr);

				}

        }

        /* --------------------- EX stage --------------------- */

        int regWrite = 0;

        if(get_opcode(state.MEMWB.instr) == LW || R) {

        	regWrite = 1;

        }
//&& (state.EXMEM.writeReg != 0)
        if((regWrite != 0)  && (state.EXMEM.writeReg == state.IDEX.rsReg)) {
        	
        	printf("\n(1a) ForwardA = 10\n");
        	
        	if(get_opcode(state.MEMWB.instr) == R) {
        	
        		state.IDEX.readData1 = state.EXMEM.aluResult;
        		
        	} else {
       
        		if(state.EXMEM.writeReg == state.IDEX.rtReg) {
       
        			state.IDEX.readData2 = state.MEMWB.writeDataMem;
       
        			state.IDEX.readData1 = state.MEMWB.writeDataMem;
       
        		} else {
       
        			state.IDEX.readData1 = state.MEMWB.writeDataMem;
       
        	}
       
        }
        //&& (state.EXMEM.writeReg != 0)
        } else if((regWrite != 0)  && (state.EXMEM.writeReg == state.IDEX.rtReg)) {
        
        	printf("\n(1b) ForwardB = 10\n");

        	if(get_opcode(state.MEMWB.instr == R)) {
        
        		state.IDEX.readData2 = state.EXMEM.aluResult;
        	
        	} else {

        		if(state.EXMEM.writeReg == state.IDEX.rsReg) {

        			state.IDEX.readData1 = state.MEMWB.writeDataMem;

        			state.IDEX.readData2 = state.MEMWB.writeDataMem;

        		} else {

        			state.IDEX.readData2 = state.MEMWB.writeDataMem;

        		}

        	}

        } else if((regWrite != 0)  && (state.MEMWB.writeReg == state.IDEX.rsReg)
        	&& !((regWrite != 0)  && (state.EXMEM.writeReg == state.IDEX.rsReg))) {
        
            printf("\n(2a) ForwardA = 01\n");

            state.IDEX.readData1 = state.MEMWB.writeDataMem;
        //&& (state.MEMWB.writeReg != 0)
         //   && (state.EXMEM.writeReg != 0)
        } else if((regWrite != 0)  && (state.MEMWB.writeReg == state.IDEX.rtReg)
        	&& !((regWrite != 0)  && (state.EXMEM.writeReg == state.IDEX.rtReg))) {
        
            printf("\n(2b) ForwardB = 01\n");

            state.IDEX.readData2 = state.MEMWB.writeDataMem;
        
        }

        newState.EXMEM.instr = state.IDEX.instr;

            switch(get_opcode(newState.EXMEM.instr)) {

                case LW:

                    newState.EXMEM.writeReg = get_rt(newState.EXMEM.instr);

                    newState.EXMEM.writeDataReg = state.regFile[get_rt(newState.EXMEM.instr)];

                    newState.EXMEM.aluResult = ((state.IDEX.readData1) + (state.IDEX.immed));

                    break;

                case SW:
                    
                    newState.EXMEM.writeReg = get_rt(newState.EXMEM.instr);

                    newState.EXMEM.writeDataReg = get_rt(newState.EXMEM.instr);//state.regFile[get_rt(newState.EXMEM.instr)];

                    newState.EXMEM.aluResult = ((state.IDEX.readData1) + (state.IDEX.immed));

                    break;

                case BNE:

                    newState.EXMEM.writeReg = get_rt(newState.EXMEM.instr);

                    newState.EXMEM.writeDataReg = state.regFile[get_rt(newState.EXMEM.instr)];

                    newState.EXMEM.aluResult = (state.regFile[get_rs(newState.EXMEM.instr)] - 
                    		state.regFile[get_rt(newState.EXMEM.instr)]);

                    break;

                case HALT:

                	newState.EXMEM.writeReg = 0;

                	newState.EXMEM.writeDataReg = 0;

                	newState.EXMEM.aluResult = 0;

                	break;

                case R:

                    	newState.EXMEM.writeReg = get_rd(newState.EXMEM.instr);

                    	newState.EXMEM.writeDataReg = get_rt(newState.EXMEM.instr);
       
                    switch(get_funct(newState.EXMEM.instr)) {
       
                        case ADD:

                            newState.EXMEM.aluResult = (state.IDEX.readData1) + (state.IDEX.readData2);

                            break;

                        case SUB:

                        newState.EXMEM.aluResult = 
((state.IDEX.readData2) - (state.IDEX.readData1));

                            break;

                    }

            }

        /* --------------------- MEM stage --------------------- */

        newState.MEMWB.instr = state.EXMEM.instr;
 
        switch(get_opcode(newState.MEMWB.instr)) {
        	
        	case LW:
        	    
        	    newState.MEMWB.writeDataALU = state.EXMEM.aluResult;

        		newState.MEMWB.writeReg = state.EXMEM.writeReg;
        	
        		newState.MEMWB.writeDataMem = state.dataMem[((state.EXMEM.aluResult) / 4)];
        	
        		break;
        	
        	case SW:
        	    
        	    newState.MEMWB.writeDataALU = state.EXMEM.aluResult;

        		newState.MEMWB.writeReg = state.EXMEM.writeReg;

				newState.dataMem[(((state.EXMEM.aluResult) / 4))] = state.regFile[state.EXMEM.writeReg];
                
                break;

            case HALT:

            	newState.MEMWB.writeDataALU = 0;

            	newState.MEMWB.writeReg = 0;
	
				break;

			case BNE:

				newState.MEMWB.writeDataALU = state.EXMEM.aluResult;

				newState.MEMWB.writeReg = state.EXMEM.writeReg;

				newState.MEMWB.writeDataMem = state.EXMEM.writeDataReg;

				if(newState.MEMWB.writeDataALU != 0) {

					// newState.IFID.instr = state.IFID.instr;

                	// newState.PC = newState.IDEX.immed;

                	// newState.IFID.PCPlus4 = newState.PC + 4;

                	numBranches++;

				}
        	
        	default:

        	    int temp = ((newState.MEMWB.instr)& 0x0000FFFF);

        		if(temp != 0) {

        			newState.MEMWB.writeDataALU = state.EXMEM.aluResult;

        			newState.MEMWB.writeReg = state.EXMEM.writeReg;

        		}

        		break;
        
        }

        /* --------------------- WB stage --------------------- */

        switch(get_opcode(newState.MEMWB.instr)) {

        	case LW:

        		newState.regFile[newState.MEMWB.writeReg] = newState.MEMWB.writeDataMem;

        		break;

        	case HALT:

        		newState.MEMWB.writeDataALU = 0;
    			
    			newState.MEMWB.writeReg = 0;

        		break;

        	case R:

        		int temp = ((newState.MEMWB.instr)& 0x0000FFFF);

        		if(temp != 0) {

        			newState.MEMWB.writeReg = get_rd(newState.MEMWB.instr); 

        			newState.regFile[newState.MEMWB.writeReg] = newState.MEMWB.writeDataALU;

        			break;

        		} else {

        			newState.MEMWB.writeDataALU = 0;

        			newState.MEMWB.writeReg = 0;

        			break;

        		}      		

        }



        state = newState;    /* The newState now becomes the old state before we execute the next cycle */
  
    	}

}

/******************************************************************/
/* The initState function accepts a pointer to the current        */ 
/* state as an argument, initializing the state to pre-execution  */
/* state. In particular, all registers are zero'd out. All        */
/* instructions in the pipeline are NOOPS. Data and instruction   */
/* memory are initialized with the contents of the assembly       */
/* input file.                                                    */
/*****************************************************************/
void initState(stateType *statePtr)
{
    unsigned int dec_inst;
    int data_index = 0;
    int inst_index = 0;
    char line[130];
    char instr[5];
    char args[130];
    char* arg; 

    statePtr->PC = 0;
    statePtr->cycles = 0;

    /* Zero out data, instructions, and registers */
    memset(statePtr->dataMem, 0, 4*NUMMEMORY);
    memset(statePtr->instrMem, 0, 4*NUMMEMORY);
    memset(statePtr->regFile, 0, 4*NUMREGS);

    /* Parse assembly file and initialize data/instruction memory */
    while(fgets(line, 130, stdin)){
        if(sscanf(line, "\t.%s %s", instr, args) == 2){
            arg = strtok(args, ",");
            while(arg != NULL){
                statePtr->dataMem[data_index] = atoi(arg);
                data_index += 1;
                arg = strtok(NULL, ","); 
            }  
        }
        else if(sscanf(line, "\t%s %s", instr, args) == 2){
            dec_inst = instrToInt(instr, args);
            statePtr->instrMem[inst_index] = dec_inst;
            inst_index += 1;
        }
    } 

    /* Zero-out all registers in pipeline to start */
    statePtr->IFID.instr = 0;
    statePtr->IFID.PCPlus4 = 0;
    statePtr->IDEX.instr = 0;
    statePtr->IDEX.PCPlus4 = 0;
    statePtr->IDEX.branchTarget = 0;
    statePtr->IDEX.readData1 = 0;
    statePtr->IDEX.readData2 = 0;
    statePtr->IDEX.immed = 0;
    statePtr->IDEX.rsReg = 0;
    statePtr->IDEX.rtReg = 0;
    statePtr->IDEX.rdReg = 0;
 
    statePtr->EXMEM.instr = 0;
    statePtr->EXMEM.aluResult = 0;
    statePtr->EXMEM.writeDataReg = 0;
    statePtr->EXMEM.writeReg = 0;

    statePtr->MEMWB.instr = 0;
    statePtr->MEMWB.writeDataMem = 0;
    statePtr->MEMWB.writeDataALU = 0;
    statePtr->MEMWB.writeReg = 0;
 }


 /***************************************************************************************/
 /*              You do not need to modify the functions below.                         */
 /*                They are provided for your convenience.                              */
 /***************************************************************************************/


/*************************************************************/
/* The printState function accepts a pointer to a state as   */
/* an argument and prints the formatted contents of          */
/* pipeline register.                                        */
/* You should not modify this function.                      */
/*************************************************************/
void printState(stateType *statePtr)
{
    int i;
    printf("\n********************\nState at the beginning of cycle %d:\n", statePtr->cycles+1);
    printf("\tPC = %d\n", statePtr->PC);
    printf("\tData Memory:\n");
    for (i=0; i<(NUMMEMORY/2); i++) {
        printf("\t\tdataMem[%d] = %d\t\tdataMem[%d] = %d\n", 
            i, statePtr->dataMem[i], i+(NUMMEMORY/2), statePtr->dataMem[i+(NUMMEMORY/2)]);
    }
    printf("\tRegisters:\n");
    for (i=0; i<(NUMREGS/2); i++) {
        printf("\t\tregFile[%d] = %d\t\tregFile[%d] = %d\n", 
            i, statePtr->regFile[i], i+(NUMREGS/2), statePtr->regFile[i+(NUMREGS/2)]);
    }
    printf("\tIF/ID:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IFID.PCPlus4);
    printf("\tID/EX:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IDEX.PCPlus4);
    printf("\t\tbranchTarget: %d\n", statePtr->IDEX.branchTarget);
    printf("\t\treadData1: %d\n", statePtr->IDEX.readData1);
    printf("\t\treadData2: %d\n", statePtr->IDEX.readData2);
    printf("\t\timmed: %d\n", statePtr->IDEX.immed);
    printf("\t\trs: %d\n", statePtr->IDEX.rsReg);
    printf("\t\trt: %d\n", statePtr->IDEX.rtReg);
    printf("\t\trd: %d\n", statePtr->IDEX.rdReg);
    printf("\tEX/MEM:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\taluResult: %d\n", statePtr->EXMEM.aluResult);
    printf("\t\twriteDataReg: %d\n", statePtr->EXMEM.writeDataReg);
    printf("\t\twriteReg:%d\n", statePtr->EXMEM.writeReg);
    printf("\tMEM/WB:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteDataMem: %d\n", statePtr->MEMWB.writeDataMem);
    printf("\t\twriteDataALU: %d\n", statePtr->MEMWB.writeDataALU);
    printf("\t\twriteReg: %d\n", statePtr->MEMWB.writeReg);
}

/*************************************************************/
/*  The instrToInt function converts an instruction from the */
/*  assembly file into an unsigned integer representation.   */
/*  For example, consider the add $0,$1,$2 instruction.      */
/*  In binary, this instruction is:                          */
/*   000000 00001 00010 00000 00000 100000                   */
/*  The unsigned representation in decimal is therefore:     */
/*   2228256                                                 */
/*************************************************************/
unsigned int instrToInt(char* inst, char* args){

    int opcode, rs, rt, rd, shamt, funct, immed;
    unsigned int dec_inst;
    
    if((strcmp(inst, "add") == 0) || (strcmp(inst, "sub") == 0)){
        opcode = 0;
        if(strcmp(inst, "add") == 0)
            funct = ADD;
        else
            funct = SUB; 
        shamt = 0; 
        rd = atoi(strtok(args, ",$"));
        rs = atoi(strtok(NULL, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + (rd << 11) + (shamt << 6) + funct;
    } else if((strcmp(inst, "lw") == 0) || (strcmp(inst, "sw") == 0)){
        if(strcmp(inst, "lw") == 0)
            opcode = LW;
        else
            opcode = SW;
        rt = atoi(strtok(args, ",$"));
        immed = atoi(strtok(NULL, ",("));
        rs = atoi(strtok(NULL, "($)"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;
    } else if(strcmp(inst, "bne") == 0){
        opcode = 4;
        rs = atoi(strtok(args, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        immed = atoi(strtok(NULL, ","));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;   
    } else if(strcmp(inst, "halt") == 0){
        opcode = 63; 
        dec_inst = (opcode << 26);
    } else if(strcmp(inst, "noop") == 0){
        dec_inst = 0;
    }
    return dec_inst;
}



/*************************************************/
/*  The printInstruction decodes an unsigned     */
/*  integer representation of an instruction     */
/*  into its string representation and prints    */
/*  the result to stdout.                        */
/*************************************************/
void printInstruction(unsigned int instr)
{
    char opcodeString[10];
    if (instr == 0){
      printf("NOOP\n");
    } else if (get_opcode(instr) == R) {
        if(get_funct(instr)!=0){
            if(get_funct(instr) == ADD)
                strcpy(opcodeString, "add");
            else
                strcpy(opcodeString, "sub");
            printf("%s $%d,$%d,$%d\n", opcodeString, get_rd(instr), get_rs(instr), get_rt(instr));
        }
        else{
            printf("NOOP\n");
        }
    } else if (get_opcode(instr) == LW) {
        printf("%s $%d,%d($%d)\n", "lw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == SW) {
        printf("%s $%d,%d($%d)\n", "sw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == BNE) {
        printf("%s $%d,$%d,%d\n", "bne", get_rs(instr), get_rt(instr), get_immed(instr));
    } else if (get_opcode(instr) == HALT) {
        printf("%s\n", "halt");
    }
}
