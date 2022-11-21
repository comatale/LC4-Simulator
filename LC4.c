/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>

/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState* CPU)
{
    CPU->PC = CPU->PSR = CPU->rsMux_CTL = CPU->rtMux_CTL = CPU->rdMux_CTL = CPU->regFile_WE = CPU->NZP_WE = CPU->DATA_WE = 0;
    CPU->regInputVal = CPU->NZPVal = CPU->dmemAddr = CPU->dmemValue = 0;
    
    for (int i = 0; i < 8; i++) {
        CPU->R[i] = 0;
    }
    
    for (int i = 0; i < 65536; i++) {
        CPU->memory[i] = 0;
    }
}


/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState* CPU)
{
    CPU->rsMux_CTL = CPU->rtMux_CTL =CPU->rdMux_CTL = CPU->regFile_WE = CPU->NZP_WE = CPU->DATA_WE = 0;
}


/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState* CPU, FILE* output)
{
    /* determines which register is being written to, if any */
    unsigned short int dest_reg = 0;
    if (CPU->regFile_WE == 1) {
        if (((CPU->memory[CPU->PC] >> 12) == 4) || ((CPU->memory[CPU->PC] >> 12) == 15)) {
            dest_reg = 7; /* means the instruction was a subroutine or TRAP */
        } else {
        dest_reg = (((CPU->memory[CPU->PC]) << 4) >> 13); /* results in 0000 0000 0000 0ddd */
        }
    }
    
    /* fills the output file with the current line as a string */
    fprintf(output, "%04x %016hu %x %x %04x %x %x %x %04x %04x\n", CPU->PC, CPU->memory[CPU->PC], CPU->regFile_WE, dest_reg,
            CPU->regInputVal, CPU->NZP_WE, CPU->NZPVal, CPU->DATA_WE, CPU->dmemAddr, CPU->dmemValue);
    
}


/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState* CPU, FILE* output) {
    int opcode = (CPU->memory[CPU->PC] >> 12);
    int d, s, t, imm;
    switch(opcode) {
        case 0:
            BranchOp(CPU, output);
            break;
            
        case 1:
            ArithmeticOp(CPU, output);
            break;
            
        case 2:
            ComparativeOp(CPU, output);
            break;
            
        case 4: /* JSR, JSRR */
            JSROp(CPU, output);
            break;
            
        case 5:
            LogicalOp(CPU, output);
            break;
            
        case 6: /* LDR */
            d = ((CPU->memory[CPU->PC] << 4) >> 13);
            s = ((CPU->memory[CPU->PC] << 7) >> 13);
            imm = (short) ((CPU->memory[CPU->PC] << 10) >> 10);
            
            CPU->rdMux_CTL = 0;
            CPU->rsMux_CTL = 0;
            CPU->rtMux_CTL = 0;
            CPU->regFile_WE = 1;
            CPU->regInputVal = CPU->memory[(CPU->R[s]) + imm];
            CPU->NZP_WE = 1;
            CPU->DATA_WE = 0;
            CPU->dmemAddr = 0;
            CPU->dmemValue = 0;
            
            CPU->R[d] = CPU->regInputVal;
            
            SetNZP(CPU, CPU->regInputVal);
            WriteOut(CPU, output);
            CPU->PC += 1;         
            break;
            
        case 7: /* STR */
            s = ((CPU->memory[CPU->PC] << 7) >> 13);
            t = ((CPU->memory[CPU->PC] << 4) >> 13);
            imm = (short) ((CPU->memory[CPU->PC] << 10) >> 10);
            
            CPU->rdMux_CTL = 0;
            CPU->rsMux_CTL = 0;
            CPU->rtMux_CTL = 1;
            CPU->regFile_WE = 0;
            CPU->regInputVal = 0;
            CPU->NZP_WE = 0;
            CPU->NZPVal = 0;
            CPU->DATA_WE = 1;
            CPU->dmemAddr = (CPU->R[s]) + imm;
            CPU->dmemValue = CPU->R[t];
            
            CPU->memory[CPU->dmemAddr] = CPU->dmemValue;
            WriteOut(CPU, output);
            CPU->PC += 1;            
            break;
            
        case 8: /* RTI */
            ClearSignals(CPU);
            CPU->rsMux_CTL = 1;
            CPU->regInputVal = CPU->NZPVal = CPU->dmemAddr = CPU->dmemValue = 0;
            
            CPU->PSR = (CPU->PSR << 1) >> 1;
            WriteOut(CPU, output);
            CPU->PC = CPU->R[7];
            break;
            
        case 9: /* CONST */
            d = ((CPU->memory[CPU->PC] << 4) >> 13);
            imm = (short) ((CPU->memory[CPU->PC] << 7) >> 7);
            
            CPU->rdMux_CTL = 0;
            CPU->rsMux_CTL = 0;
            CPU->rtMux_CTL = 0;
            CPU->regFile_WE = 1;
            CPU->regInputVal = imm;
            CPU->NZP_WE = 1;
            CPU->DATA_WE = 0;
            CPU->dmemAddr = 0;
            CPU->dmemValue = 0;
            
            CPU->R[d] = imm;
            SetNZP(CPU, imm);
            WriteOut(CPU, output);
            CPU->PC += 1;
            break;
            
        case 10:
            ShiftModOp(CPU, output);
            break;
            
        case 12:
            JumpOp(CPU, output);
            break;
            
        case 13: /* HICONST */
            d = ((CPU->memory[CPU->PC] << 4) >> 13);
            imm = ((CPU->memory[CPU->PC] << 8) >> 8);
            
            CPU->rdMux_CTL = 0;
            CPU->rsMux_CTL = 0;
            CPU->rtMux_CTL = 0;
            CPU->regFile_WE = 1;
            CPU->regInputVal = (CPU->R[d] & 0xFF) | (imm << 8);
            CPU->NZP_WE = 1;
            CPU->DATA_WE = 0;
            CPU->dmemAddr = 0;
            CPU->dmemValue = 0;
            
            CPU->R[d] = CPU->regInputVal;
            SetNZP(CPU, CPU->regInputVal);
            WriteOut(CPU, output);
            CPU->PC += 1;
            break;
            
        case 15: /* TRAP */
            imm = ((CPU->memory[CPU->PC] << 8) >> 8);
            
            CPU->rdMux_CTL = 1;
            CPU->rsMux_CTL = 0;
            CPU->rtMux_CTL = 0;
            CPU->regFile_WE = 1;
            CPU->regInputVal = (CPU->PC) + 1;
            CPU->NZP_WE = 1;
            CPU->DATA_WE = 0;
            CPU->dmemAddr = 0;
            CPU->dmemValue = 0;
            
            CPU->R[7] = CPU->regInputVal;
            CPU->PSR += 2^15;
            SetNZP(CPU, CPU->regInputVal);
            WriteOut(CPU, output);
            CPU->PC = (0x8000 | imm);
            break;
            
    }
    return 0;
}



//////////////// PARSING HELPER FUNCTIONS ///////////////////////////



/*
 * Parses rest of branch operation and updates state of machine.
 */
void BranchOp(MachineState* CPU, FILE* output)
{
    int increment = 0;
    int sub_opcode = (CPU->memory[CPU->PC] >> 9);
    int current_NZP = ((CPU->PSR << 1) >> 1);
    
    if ((current_NZP & sub_opcode) != 0) {
        increment = (short) ((CPU->memory[CPU->PC] << 7) >> 7);
    }

    ClearSignals(CPU);
    CPU->regInputVal = CPU->NZPVal = CPU->dmemAddr = CPU->dmemValue = 0;
    WriteOut(CPU, output);
    
    CPU->PC += increment + 1;
    
    if (((CPU->PC >= 0x2000) && (CPU->PC < 0x8000)) || (CPU->PC >= 0xA000)) {
        printf("ERROR: ILLEGAL MEM ACCESS\n");
        exit(1);
    }
}

/*
 * Parses rest of arithmetic operation and prints out.
 */
void ArithmeticOp(MachineState* CPU, FILE* output)
{
    int sub_opcode = ((CPU->memory[CPU->PC] << 10) >> 13);
    int d = ((CPU->memory[CPU->PC] << 4) >> 13);
    int s = ((CPU->memory[CPU->PC] << 7) >> 13);
    int t = ((CPU->memory[CPU->PC] << 13) >> 13);
       
    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    
    if ((sub_opcode >> 2) != 0) { /* case of an immediate add */
        int imm = (short) ((CPU->memory[CPU->PC] << 11) >> 11);
        CPU->R[d] = (short) CPU->R[s] + imm;
    } else if (sub_opcode == 0) {
        CPU->R[d] = (short) CPU->R[s] + (short) CPU->R[t];
    } else if (sub_opcode == 1) {
        CPU->R[d] = (short) CPU->R[s] * (short) CPU->R[t];
    } else if (sub_opcode == 2) {
        CPU->R[d] = (short) CPU->R[s] - (short) CPU->R[t];
    } else if (sub_opcode == 3) {
        CPU->R[d] = (short) CPU->R[s] / (short) CPU->R[t];
    }
    
    CPU->regInputVal = CPU->R[d];
    SetNZP(CPU, CPU->regInputVal);
    
    WriteOut(CPU, output);
    CPU->PC += 1;
    
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState* CPU, FILE* output)
{
    int sub_opcode = ((CPU->memory[CPU->PC] << 7) >> 14);
    int result, imm;
    int s = ((CPU->memory[CPU->PC] << 4) >> 13);
    int t = ((CPU->memory[CPU->PC] << 13) >> 13);
    
    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 2;
    CPU->rtMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->regInputVal = 0;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    
    if (sub_opcode == 0) {
        result = (short) CPU->R[s] - (short) CPU->R[t];        
    } else if (sub_opcode == 1) {
        result = CPU->R[s] - CPU->R[t];
    } else if (sub_opcode == 2) {
        imm = (short) ((CPU->memory[CPU->PC] << 9) >> 9);
        result = (short) CPU->R[s] - imm;
    } else if (sub_opcode == 3) {
        imm = ((CPU->memory[CPU->PC] << 9) >> 9);
        result = CPU->R[s] - imm;
    }
    
    SetNZP(CPU, result);
    WriteOut(CPU, output);
    CPU->PC += 1;
}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState* CPU, FILE* output)
{
    int sub_opcode = ((CPU->memory[CPU->PC] << 10) >> 13);
    int d = ((CPU->memory[CPU->PC] << 4) >> 13);
    int s = ((CPU->memory[CPU->PC] << 7) >> 13);
    int t = ((CPU->memory[CPU->PC] << 13) >> 13);
    
    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    
    if ((sub_opcode >> 2) != 0) { /* case of an immediate add */
        int imm = (short) ((CPU->memory[CPU->PC] << 11) >> 11);
        CPU->R[d] = (short) CPU->R[s] & imm;
    } else if (sub_opcode == 0) {
        CPU->R[d] = (short) CPU->R[s] & (short) CPU->R[t];
    } else if (sub_opcode == 1) {
        CPU->R[d] = ~((short) (CPU->R[s]));
    } else if (sub_opcode == 2) {
        CPU->R[d] = (short) CPU->R[s] | (short) CPU->R[t];
    } else if (sub_opcode == 3) {
        CPU->R[d] = (short) CPU->R[s] ^ (short) CPU->R[t];
    }
    
    CPU->regInputVal = CPU->R[d];
    SetNZP(CPU, CPU->regInputVal);
    
    WriteOut(CPU, output);
    CPU->PC += 1;
}

/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState* CPU, FILE* output)
{
    int sub_opcode = ((CPU->memory[CPU->PC] << 4) >> 15);
    int s = ((CPU->memory[CPU->PC] << 7) >> 13);
    int imm = (short) ((CPU->memory[CPU->PC] << 5) >> 5);
    
    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->regInputVal = 0;
    CPU->NZP_WE = 0;
    CPU->NZPVal = 0;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    
    WriteOut(CPU, output);
    
    if (sub_opcode == 0) {
        CPU->PC = CPU->R[s];
    } else {
        CPU->PC += (imm + 1);
    }
}

/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState* CPU, FILE* output)
{
    int sub_opcode = ((CPU->memory[CPU->PC] << 4) >> 15);
    int s = ((CPU->memory[CPU->PC] << 7) >> 13);
    if (((CPU->R[s] >= 0x2000) && (CPU->R[s] < 0x8000)) || (CPU->R[s] >= 0xA000) || ((CPU->R[s] >= 0x8000) && (((short) CPU->PSR) < 0))) {
        printf("ERROR: ILLEGAL PART OF MEMORY");
        exit(1);
    }
    
    CPU->rdMux_CTL = 1;
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->regInputVal = (CPU->PC) + 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    CPU->R[7] = (CPU->regInputVal);
    
    SetNZP(CPU, CPU->R[7]);
    WriteOut(CPU, output);
    
    if (sub_opcode == 0) {
        int s = ((CPU->memory[CPU->PC] << 7) >> 13);
        CPU->PC = CPU->R[s];
    } else {
        int imm = (short) ((CPU->memory[CPU->PC] << 5) >> 5);
        CPU->PC = (CPU->PC & 0x8000) | (imm << 4);
    }
}

/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState* CPU, FILE* output)
{
    int sub_opcode = ((CPU->memory[CPU->PC] << 4) >> 15);
    int s = ((CPU->memory[CPU->PC] << 7) >> 13);
    int t = ((CPU->memory[CPU->PC] << 13) >> 13);
    int d = ((CPU->memory[CPU->PC] << 4) >> 13);
    int imm = ((CPU->memory[CPU->PC] << 12) >> 12);
}

/*
 * Set the NZP bits in the PSR.
 */
void SetNZP(MachineState* CPU, short result)
{
    CPU->PSR = ((CPU->PSR >> 3) << 3);
    if (result > 0) {
        CPU->NZPVal = 1;        
    } else if (result == 0) {
        CPU->NZPVal = 2;
    } else {
        CPU->NZPVal = 4;
    }       
    CPU->PSR += CPU->NZPVal;
}
