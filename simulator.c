/*
  Author: Daniel Kopta and Hunter
  CS 4400, University of Utah

  * Simulator handout
  * A simple x86-like processor simulator.
  * Read in a binary file that encodes instructions to execute.
  * Simulate a processor by executing instructions one at a time and appropriately 
  * updating register and memory contents.

  * Some code and pseudo code has been provided as a starting point.

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "instruction.h"
#include "string.h"

// Forward declarations for helper functions
unsigned int get_file_size(int file_descriptor);
unsigned int* load_file(int file_descriptor, unsigned int size);
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions);
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, 
				 unsigned int* registers, unsigned char* memory);
void print_instructions(instruction_t* instructions, unsigned int num_instructions);
void error_exit(const char* message);

// 17 registers
#define NUM_REGS 17
// 1024-byte stack
#define STACK_SIZE 1024

int main(int argc, char** argv)
{
  //printf("++++++++++++++++++++++++++++++++++++++++++\n");

  // Make sure we have enough arguments
  if(argc < 2)
    error_exit("must provide an argument specifying a binary file to execute");

  // Open the binary file
  int file_descriptor = open(argv[1], O_RDONLY);
  if (file_descriptor == -1) 
    error_exit("unable to open input file");

  // Get the size of the file
  unsigned int file_size = get_file_size(file_descriptor);
  // Make sure the file size is a multiple of 4 bytes
  // since machine code instructions are 4 bytes each
  if(file_size % 4 != 0)
    error_exit("invalid input file");

  // Load the file into memory
  // We use an unsigned int array to represent the raw bytes
  // We could use any 4-byte integer type
  unsigned int* instruction_bytes = load_file(file_descriptor, file_size);
  close(file_descriptor);

  unsigned int num_instructions = file_size / 4;


  /****************************************/
  /**** Begin code to modify/implement ****/
  /****************************************/

  // Allocate and decode instructions (left for you to fill in)
  instruction_t* instructions = decode_instructions(instruction_bytes, num_instructions);

  // Optionally print the decoded instructions for debugging
  // Will not work until you implement decode_instructions
  // Do not call this function in your handed in final version
  //print_instructions(instructions, num_instructions);




  // Allocate and initialize registers
  unsigned int* registers = (unsigned int*)malloc(sizeof(unsigned int) * NUM_REGS);
  for (int i = 0; i < NUM_REGS; i++){
      registers[i] = 0;
  }
  registers[8] = 1024; //This is %esp

  // Stack memory is byte-addressed, so it must be a 1-byte type
  unsigned char* memory = (unsigned char*)malloc(1024);

  // program_counter is a byte address, so we must multiply num_instructions by 4 to get the address past the last instruction
  unsigned int program_counter = 0;
  while(program_counter != num_instructions * 4)
  {
    program_counter = execute_instruction(program_counter, instructions, registers, memory);
  }

  return 0;
}



/*
 * Decodes the array of raw instruction bytes into an array of instruction_t
 * Each raw instruction is encoded as a 4-byte unsigned int
*/
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions)
{
  //allocate memory
  instruction_t* retval = NULL; 
  int mem_size = sizeof(instruction_t)* num_instructions;
  retval = malloc(mem_size);
  for(int i = 0; i < num_instructions; i++){
    int instr = bytes[i];
    instruction_t x; //struct from instruction.h
    //Obtain the instructions from the byte list
    x.opcode = (instr >> 27) & 0x1F;
    x.first_register = (instr >> 22) & 0x1F;
    x.second_register = (instr >> 17) & 0x1F;
    x.immediate = instr & 0xFFFF;
    retval[i] = x; 
  }
  return retval;
}

/*
 * Executes a single instruction and returns the next program counter
*/
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, unsigned int* registers, unsigned char* memory)
{
  // program_counter is a byte address, but instructions are 4 bytes each
  // divide by 4 to get the index into the instructions array
  instruction_t instr = instructions[program_counter / 4];

  //Memcopy is used when all 4 bytes are needed to be transferred
  switch(instr.opcode)
  {
  //Basic math and bit/byte movement functions
  case subl:
    registers[instr.first_register] = registers[instr.first_register] - instr.immediate;
    break;
  case addl_reg_reg:
    registers[instr.second_register] = registers[instr.first_register] + registers[instr.second_register];
    break;
  case addl_imm_reg:
    registers[instr.first_register] = registers[instr.first_register] + instr.immediate;
    break;
  case imull:
    registers[instr.second_register] = registers[instr.first_register] * registers[instr.second_register];
    break;
  case shrl:
    registers[instr.first_register] = registers[instr.first_register] >> 1;
    break;
  case movl_reg_reg:
    registers[instr.second_register] = registers[instr.first_register];
    break;
  case movl_deref_reg:
    memcpy(registers + instr.second_register, memory + registers[instr.first_register] + instr.immediate, 4);
    break;
  case movl_reg_deref:
    memcpy(memory + registers[instr.second_register] + instr.immediate, registers + instr.first_register, 4);
        break;
  case movl_imm_reg:
    registers[instr.first_register] = (int32_t)instr.immediate;
    break;
  //Detects/sets conditions codes; Carry, Zero, Sign, and Overflow
  case cmpl:
    //CF- If reg2 - reg1 results in unsigned overflow, CF is true
    registers[0] = 0;
    if ( (unsigned int)(registers[instr.second_register] < (unsigned int)registers[instr.first_register])){   
       registers[0] = registers[0]   | ( 1 << 0); //Sets 0th bit of eflags.  Yes only one bar since it is bitwise.
    }
    //ZF -If reg2 - reg1 == 0, ZF is true
    if ((registers[instr.second_register] - registers[instr.first_register]) == 0){
      registers[0] = registers[0] | (1 << 6); //Sets 6th bit of eflags true.
    }
    //SF- If most significant bit of (reg2 - reg1) is 1, SF is true	
    if (((int)registers[instr.second_register] - (int)registers[instr.first_register]) < 0){
      registers[0] = registers[0] | (1 << 7); //Sets 7th bit of eflags true
      }
    //OF- If reg2 - reg1 results in signed overflow, OF is true
    if (((long)(int)registers[instr.second_register] - (long)(int)registers[instr.first_register]) > 2147483647 || 
      ((long)(int)registers[instr.second_register] - (long)(int)registers[instr.first_register]) < -2147483648
                ){
      registers[0] = registers[0] | (1 << 11); //Sets 11th bit of eflags true.
    }
    break;
  //Jump methods
  case je:
    if ( (registers[0] >> 6) & 1){
        return (program_counter + 4) + instr.immediate;
    }
    else{
      return (program_counter + 4);
    }
    break; 
  case jl:
    if (( registers[0] >> 7 & 1)  ^ (registers[0] >> 11 & 1) ){ //get 6th bit
        return (program_counter + 4) + instr.immediate;
    }
    else{
      return (program_counter + 4);
    }
    break;
  case jle:
    if ((( registers[0] >> 7 & 1)  ^ (registers[0] >> 11 & 1) ) | ((registers[0] >> 6) & 1)) { //get 6th bit
        return (program_counter + 4) + instr.immediate;
    }
    else{
      return (program_counter + 4);
    }
    break;    
  case jbe:
    if (( registers[0] >> 0 & 1) | ( (registers[0] >> 6) & 1)){
        return (program_counter + 4) + instr.immediate;
    }
    else{
      return (program_counter + 4);
    }
    break;
  case jge: 
    if (!(( registers[0] >> 7 & 1)  ^ (registers[0] >> 11 & 1))){
        return (program_counter + 4) + instr.immediate;
    }
    else{
      return (program_counter + 4);
    }
    break;
  case jmp: 
    return (program_counter+4) + instr.immediate;
    break;
  //Read and/or jump forward an instruction
  case call:
    registers[8] = registers[8] -4;
    program_counter+=4;
    memcpy(memory + registers[8], &program_counter, 4);
    program_counter = program_counter + instr.immediate;
    return program_counter;
  //Checks if it is time for the program to exit, otherwise move forward an instruction
  case ret:
    if (registers[8] == 1024){
      exit(0);
    }
    else{
      memcpy(&program_counter, memory + registers[8], 4);
      registers[8] = registers[8] + 4;
    }
      return program_counter;
  //Push and Pop onto/from the stack
  case pushl:
    registers[8] = registers[8]-4;
    memcpy(memory + registers[8], registers + instr.first_register, 4);
    break;
  case popl:
    memcpy(registers + instr.first_register, memory + registers[8], 4);
    registers[8] = registers[8]+4;

    break;
  //----------------------------------
  case printr:
    printf("%d (0x%x)\n", registers[instr.first_register], registers[instr.first_register]);
    break;
  case readr:
    scanf("%d", &(registers[instr.first_register]));
    break;
  }

  // program_counter + 4 represents the subsequent instruction
  return (program_counter + 4);
}


/***********************************************/
/**** Begin helper functions. Do not modify ****/
/***********************************************/

/*
 * Returns the file size in bytes of the file referred to by the given descriptor
*/
unsigned int get_file_size(int file_descriptor)
{
  struct stat file_stat;
  fstat(file_descriptor, &file_stat);
  return file_stat.st_size;
}

/*
 * Loads the raw bytes of a file into an array of 4-byte units
*/
unsigned int* load_file(int file_descriptor, unsigned int size)
{
  unsigned int* raw_instruction_bytes = (unsigned int*)malloc(size);
  if(raw_instruction_bytes == NULL)
    error_exit("unable to allocate memory for instruction bytes (something went really wrong)");

  int num_read = read(file_descriptor, raw_instruction_bytes, size);

  if(num_read != size)
    error_exit("unable to read file (something went really wrong)");

  return raw_instruction_bytes;
}

/*
 * Prints the opcode, register IDs, and immediate of every instruction, 
 * assuming they have been decoded into the instructions array
*/
void print_instructions(instruction_t* instructions, unsigned int num_instructions)
{
  printf("instructions: \n");
  unsigned int i;
  for(i = 0; i < num_instructions; i++)
  {
    printf("op: %d, reg1: %d, reg2: %d, imm: %d\n", 
	   instructions[i].opcode,
	   instructions[i].first_register,
	   instructions[i].second_register,
	   instructions[i].immediate);
  }
  printf("--------------\n");
}


/*
 * Prints an error and then exits the program with status 1
*/
void error_exit(const char* message)
{
  printf("Error: %s\n", message);
  exit(1);
}
