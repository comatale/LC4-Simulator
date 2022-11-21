/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

int main(int argc, char** argv)
{
    /* create instance of the MachineState structure and reset it */
    MachineState my_cpu;
    Reset(&my_cpu);
    
    /* check for valid arguments to main */
    if (argc < 3) {
        printf("USAGE ERROR: NEED A RETURN FILE AND AN .OBJ FILE\n");
        return -1;
    }
    
    /* initialize memory from the command-line .obj file(s) */
    for (int i = 2; i < argc; i++) {
        int j = ReadObjectFile(argv[i], &my_cpu);
        if (j == 1) {
            printf ("USAGE ERROR: INPUT FILE(S) NOT FOUND\n");
            return 1; /* j=1 means ReadObjectFile could not open one of the files */
        }
    }
    
    /*
    for (int i = 0; i <= 0x400b; i++) {
        printf("Address: %x Contents: %x\n", i, my_cpu.memory[i]);
    }*/
    
    /* initialize program counter */
    my_cpu.PC = 0x8200;
    
    /* open output file */
    FILE* out_file = fopen(argv[1], "w");
    
    /* main loop*/
    do {
        UpdateMachineState(&my_cpu, out_file);
    } while (my_cpu.PC != 0x80FF);
    
    /* char s;
    while((s = fgetc(out_file)) != EOF) {
      printf("%c",s);
   } */
    
    fclose(out_file);
    
    return 0;
}
/*
       once you code main() to open up files, you can access the test cases by typing:
       ./trace p1_test_cases/divide.obj   - divide.obj is just an example
    
       please note part 1 test case files have an OBJ and a TXT file
       the .TXT shows you what's in the .OBJ files
    
       part 2 test case files have an OBJ, an ASM, and a TXT files
       the .OBJ is what you must read in
       the .ASM is the original assembly file that the OBJ was produced from
       the .TXT file is the expected output of your simulator for the given OBJ file
    */