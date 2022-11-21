/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"

// memory array location
unsigned short memoryAddress;

/*
 * Read an object file and modify the machine state as described in the writeup
 */
int ReadObjectFile(char* filename, MachineState* CPU) {  
  FILE* obj_file = fopen(filename, "rb");
  if (obj_file == NULL) {return 1;}
  
  /* make temporary array to store the binary, then read the file into it */
  unsigned short int binary_array[65536];
  fread(binary_array, 2, 65536, obj_file);
  
  int j,k;  
  /* account for endianness */
  for (int i=0; i < 65536; i++) {
      j = binary_array[i] >> 8;
      k = binary_array[i] << 8;
      binary_array[i] = j + k;
  }
  
  int n;  
  /* populate memory */
  for (int i = 0; i < 65536; i++) {
      if ((binary_array[i] == 0xCADE) || (binary_array[i] == 0xDADA)) {
          memoryAddress = binary_array[i+1];
          n = binary_array[i+2];
          i += 3;
          for (int j = 0; j < n; j++,i++) {
              CPU->memory[memoryAddress + j] = binary_array[i];
          }
          i--;
      } 
  }
  
  fclose(obj_file);
    
  return 0;
}
