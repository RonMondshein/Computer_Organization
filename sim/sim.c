#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h> 
#include <limits.h>
#include <ctype.h>
#endif

#define IMEMIN_LINE_LENGTH 12
#define DMEMIN_LINE_LENGTH 8
#define DISK_LINE_LENGTH 8
#define REGISTERS_COUNT 16
#define IOREGISTERS_COUNT 23
#define MEMORYSIZE 4096 // 4KB
#define  DISKSIZE 16384 // 16KB
#define SCREENSIZE 65536 // 64KB

//declerations
void write_to_trace(int* registers, int pc, char* instruction, FILE* trace);
int twos_complement(char* hex, int three_letters);

int inside_irq; /*1 if we are inside an irq, 0 otherwise. prevent nested irq support*/
int diskcycle = 0; // global variable to keep track of disk cycles

// Function to check if all characters are whitespace
int is_whitespace(const char* s) {
    while (*s) {
        if (!isspace((unsigned char)*s))
            return 0; // Found a non-whitespace character
        s++;
    }
    return 1; // All characters are whitespace
}

// Function to read a file into an array of strings
char** read_file_to_array(int line_length, FILE* file, size_t* line_amount) {
    char** lines = NULL;  // Array to store lines
    size_t line_count = 0;
    char buffer[100];  // Buffer to store the current line

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // Remove newline character at the end of the buffer, if present
        buffer[strcspn(buffer, "\n")] = '\0';

        // Skip empty lines or lines with only newline character
        if (strlen(buffer) == 0) {
            continue;
        }
        // Skip empty lines or lines with only whitespace characters
        if (is_whitespace(buffer)) {
            continue;
        }

        // Allocate memory for the new line pointer array
        char** temp_lines = realloc(lines, (line_count + 1) * sizeof(char*));
        if (temp_lines == NULL) {
            printf("Memory allocation error\n");
            // Free allocated memory for lines array if it was allocated
            if (lines != NULL) {
                for (size_t i = 0; i < line_count; ++i) {
                    free(lines[i]);
                }
                free(lines);
            }
            return NULL;
        }
        lines = temp_lines; // Update lines pointer if reallocation was successful
        // Allocate memory for the current line and copy the content
        lines[line_count] = malloc(strlen(buffer) + 1);
        if (lines[line_count] == NULL) {
            printf("Memory allocation error\n");
            // Free allocated memory for lines and lines array
            for (size_t i = 0; i < line_count; ++i) {
                free(lines[i]);
            }
            free(lines);
            return NULL;
        }
        strcpy(lines[line_count], buffer);

        // Increment the line count
        line_count++;
    }

    // Allocate space for the NULL terminator in the lines array
    char** temp_lines = realloc(lines, (line_count + 1) * sizeof(char*));
    if (temp_lines == NULL) {
        // Free allocated memory for lines array
        for (size_t i = 0; i < line_count; ++i) {
            free(lines[i]);
        }
        free(lines);
        printf("Memory allocation error for NULL terminator\n");
        return NULL;
    }
    lines = temp_lines;
    lines[line_count] = NULL;  // Null-terminate the lines array

    // Set the actual number of lines read
    *line_amount = line_count;

    return lines;
}

/*******************
* Interrupt Section *
********************/
/*
interups handler
input - *IO_array: io registers array, inside_irq - if we are inside an irq, current_instruction - the current instruction
output - return current instruction if we are inside an irq (we don't support nested irq), the address of the handler if we are not
 and -1 if none of the enables and their corresponding states are on
*/
int irq_handler(int* IO_array, int inside_irq, int current_instruction, int called_from_reti) {
    if ((IO_array[0] == 1 && IO_array[3] == 1) || (IO_array[1] == 1 && IO_array[4] == 1) || (IO_array[2] == 1 && IO_array[5] == 1)) {
        /*interruption had happen if and only if it at least one enable and its corresponding status are on*/
        if (inside_irq) {
            /*if we are already inside an irq, countinue (ignore nested irq)*/
            return current_instruction;
        }
        IO_array[3] = 0; /* irq0status is off */
        IO_array[4] = 0; /* irq1status is off */
        IO_array[5] = 0; /* irq2status is off */
        inside_irq = 1; /*we are inside an irq*/
        IO_array[7] = current_instruction; /* save the current address in irqreturn so we can return to it later on*/
        return IO_array[6]; /*return the address of the handler*/
    }
    if (called_from_reti) {
        return -1;
    }

    return current_instruction;
}

//irq0_handler
/*In every clock cycle in which the timer is enable, the timercurrent register is incremented by one
and implementing series of checks.*/
void irq0_checker(int* io_array) {
    io_array[3] = 0;  /* irq0status is off */
    if (io_array[11] == 1) {// Check if timerenable register is on
        io_array[12] += 1; // Incrementing timercurrent register by one
        if (io_array[12] == io_array[13]) { // check if timercurrent == timermax
            io_array[3] = 1; // irq0status is on
            io_array[12] = 0; //reset timercurrent
        }
    }
}

// irq1_check() checks if disk is busy or free and performs the read/write operation
// io_array: array of IO registers
// disk: array of disk contents
// memin: array of memory contents
void irq1_check(int* io_array, char** disk, char** memin) {
    if (io_array[17] != 0) return; // check if disk is busy 
    int sectorfirstrow = io_array[15] * 128; // get the sector number

    if (io_array[14] == 1) { //reading from disk
        for (int i = 0; i < 128; i++) {// read 128 words from disk
            strcpy(memin[(i + io_array[16]) % MEMORYSIZE], disk[sectorfirstrow + i]);
        }
    }

    if (io_array[14] == 2) { //writing to disk
        for (int i = 0; i < 128; i++) {// write 128 words to disk
            strcpy(disk[sectorfirstrow + i], memin[(i + io_array[16]) % MEMORYSIZE]);
        }

    }

    io_array[17] = 1; // disk is busy
    diskcycle = 0; // reset diskcycle
}

/*
external (to the processor) interrupt line, An interrupt file to the simmulator specifies when the interrupt occurs
input - *IO_array: io registers array, *irq2in: file with the interrupt, current_interrupt - current interrupt, init with -1 outside the function
int current_interrupt - current interrupt, init with -1 outside the function
ouput - None(void)
*/
void irq2_checker(int* IO_array, FILE* irq2in, int* current_interrupt, int* current_cycle, int* end_of_file) {
    IO_array[5] = 0; /* irq2status is off */
    int line = 0;
    int reader = -1;
    if (*current_interrupt == -1)
    {
        /*first time we enterted the function*/
        line = fscanf(irq2in, "%d", current_interrupt);
    }

    /*if the current cycle is greater than the current interrupt (we passed it) and the file is not empty, read the next interrupt*/
    if ((*current_cycle > *current_interrupt + 1) && !feof(irq2in) && *end_of_file)
    {
        line = fscanf(irq2in, "%d", &reader);
        //fscand didn't Succeeded
        if (line != 1 || reader == -1 || reader == '\n') {
            *end_of_file = 0;
            *current_interrupt = INT_MAX;
        }
        else {
            *current_interrupt = reader;
        }
    }
    /* the current interrupt is the current cycle, put the value in the IO array */
    if (*current_interrupt < *current_cycle)
    {
        /*if the interrupt is enabled, turn on irq2status*/
        IO_array[5] = 1;
    }
    if (feof(irq2in))
    {
        fclose(irq2in);
    }
}

/*update the disk timer if the disk is busy, if diskCycle reached 1024 or above, init the relevent registers*/
void disk_timer(int* io_array) {
    io_array[4] = 0; // irq1status is off
    if (io_array[17]) {
        diskcycle++; // if disk is busy, increment diskcycle
        // reached 1024 cycles
        if (diskcycle >= 1024) {
            // reset disk command
            io_array[14] = 0;
            // and disk status
            io_array[17] = 0;
            // and trips irq1status
            io_array[4] = 1;
        }
    }
}

/*
hwregout_p - log cycle count, operation type, register name, and data
 */
void hwregout_printer(int reg_num, int op, int* io_array, int* current_cycle, FILE* hwregtrace) {
    // Array of hardware register names for easy indexing based on line_num
    const char* hwreg_nums[] = {
        "irq0enable", "irq1enable", "irq2enable", "irq0status", "irq1status",
        "irq2status", "irqhandler", "irqreturn", "clks", "leds", "display7seg",
        "timerenable", "timercurrent", "timermax", "diskcmd", "disksector",
        "diskbuffer", "diskstatus", "reserved","reserved", "monitoraddr", "monitordata",
        "monitorcmd"
    };

    // Number of entries in hwreg_nums array
    const int hwreg_nums_count = sizeof(hwreg_nums) / sizeof(hwreg_nums[0]);

    // Check if line_num is within the bounds of hwreg_nums array
    if (reg_num >= 0 && reg_num < hwreg_nums_count) {
        // Log cycle count, operation type, register name, and data
        fprintf(hwregtrace, "%d %s %s %08X\n",
            *current_cycle,
            op == 0 ? "READ" : "WRITE",
            hwreg_nums[reg_num],
            io_array[reg_num]);
        fflush(hwregtrace);
    }
}

/******************
Arthmetic Opcodes Section
include add, sub, mac, and, or, xor, sll, sra, srl
******************/
/*
add - add reg_array[rs], reg_array[rt], reg_array[rm] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int add(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    reg_array[rd] = reg_array[rs] + reg_array[rt] + reg_array[rm];
    return current_instruction + 1;
}
/*
sub - sub reg_array[rt], reg_array[rm] from reg_array[rs] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int sub(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    reg_array[rd] = reg_array[rs] - reg_array[rt] - reg_array[rm];
    return current_instruction + 1;

}
/*
mac - multiply reg_array[rs] and reg_array[rt] then add reg_array[rm] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int mac(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    reg_array[rd] = (reg_array[rs] * reg_array[rt]) + reg_array[rm];
    return current_instruction + 1;
}
/*
and - bitwise AND reg_array[rs], reg_array[rt], reg_array[rm] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int and (int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    reg_array[rd] = reg_array[rs] & reg_array[rt] & reg_array[rm];
    return current_instruction + 1;
}
/*
or - bitwise OR reg_array[rs], reg_array[rt], reg_array[rm] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int or (int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    reg_array[rd] = reg_array[rs] | reg_array[rt] | reg_array[rm];
    return current_instruction + 1;
}
/*
xor - bitwise XOR reg_array[rs], reg_array[rt], reg_array[rm] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int xor (int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    reg_array[rd] = reg_array[rs] ^ reg_array[rt] ^ reg_array[rm];
    return current_instruction + 1;
}
/*
sll - shift left reg_array[rs] by reg_array[rt] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int sll(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    if (reg_array[rt] < 0) { // equal to shift right logical
        reg_array[rt] = -reg_array[rt];
        reg_array[rd] = (int)((unsigned int)reg_array[rs] >> (reg_array[rt] & 0x1F));
    }
    else {
        reg_array[rd] = reg_array[rs] << (reg_array[rt] & 0x1F);
    }
    return current_instruction + 1;
}
/*
sra - shift right arithmetic reg_array[rs] by reg_array[rt] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int sra(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    if (reg_array[rt] < 0) { // equal to shift left
        reg_array[rt] = -reg_array[rt];
        reg_array[rd] = reg_array[rs] << (reg_array[rt] & 0x1F);
    }
    else {
        reg_array[rd] = reg_array[rs] >> (reg_array[rt] & 0x1F);
    }
    return current_instruction + 1;
}
/*
srl - shift right reg_array[rs] by reg_array[rt] and store the result in reg_array[rd]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/

int srl(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction) {
    if (reg_array[rt] < 0) { // equal to shift left
        reg_array[rt] = -reg_array[rt];
        reg_array[rd] = reg_array[rs] << (reg_array[rt] & 0x1F);
    }
    else {
        reg_array[rd] = (int)((unsigned int)reg_array[rs] >> (reg_array[rt] & 0x1F));
    }
    return current_instruction + 1;
}

/********************
Memory Opcodes Section
include lw, sw
********************/
/*
lw - load data from dataMemory[reg_array[rs] + reg_array[rt]] + reg_array[rm] and store it in reg_array[rd]
input - *reg_array, *dataMemory, rd, rs, rt, rm
*/
int lw(int* reg_array, char** dataMemory, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    int old_imm1 = reg_array[1];
    int old_imm2 = reg_array[2];
    reg_array[1] = (int)unsigned_imm1;
    reg_array[2] = (int)unsigned_imm2;
    int place_in_memory = reg_array[rs] + reg_array[rt];
    if (rd == 0)
    {
        /*do nothing, can't write to register zero*/
        return current_instruction + 1;
    }
    if (reg_array == NULL )
    {
        printf("Error: Null pointer detected\n");
        return current_instruction + 1;
    }
    if (dataMemory == NULL)
    {
        printf("Error: Null pointer detected\n");
        reg_array[rd] = 0; /*if the array is null, put 0 in the register*/
        return current_instruction + 1;
    }
    if (place_in_memory >= MEMORYSIZE || place_in_memory < 0)
    {
        /*if the place in memory is out of bounds, take the 12 lowers bits*/
        place_in_memory = place_in_memory & 0x0FFF;
    }
    reg_array[1] = old_imm1;
    reg_array[2] = old_imm2;

    /*take the 3 LSB from dataMemory[place_in_memory]*/
    reg_array[rd] = twos_complement(dataMemory[place_in_memory], 0) + reg_array[rm];

    return current_instruction + 1;
}

/*
sw- store data from reg_array[rd] + reg_array[rm] in dataMemory[reg_array[rs] + reg_array[rt]]
input - *reg_array, *dataMemory, rd, rs, rt, rm
*/
int sw(int* reg_array, char** dataMemory, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    if (reg_array == NULL || dataMemory == NULL) {
        printf("Error: Null pointer detected\n");
        return current_instruction + 1;
    }
    int old_imm1 = reg_array[1];
    int old_imm2 = reg_array[2];
    reg_array[1] = (int)unsigned_imm1;
    reg_array[2] = (int)unsigned_imm2;
    int place_in_memory = reg_array[rs] + reg_array[rt];

    if (place_in_memory >= MEMORYSIZE || place_in_memory < 0) {
        // Adjust to fit within memory bounds
        place_in_memory = place_in_memory & 0x0FFF;
    }
    reg_array[1] = old_imm1;
    reg_array[2] = old_imm2;
    char result_str[9];  // Assuming 8 characters for hexadecimal representation + 1 for null terminator
    snprintf(result_str, sizeof(result_str), "%08X", reg_array[rd] + reg_array[rm]);

    // Allocate memory for the string and copy the result
    char* result_copy = strdup(result_str);
    if (result_copy == NULL) {
        printf("Error: Memory allocation failed\n");
        return current_instruction + 1;
    }

    // Store the data in the memory
    if (dataMemory[place_in_memory] != NULL) {
        free(dataMemory[place_in_memory]);  // Free previously allocated memory
    }
    dataMemory[place_in_memory] = result_copy;
    return current_instruction + 1;
}


/*********************
branch Opcodes Section
include beq, bne, blt, bgt, ble, bge, jal
*********************/

/*
beq - if reg_array[rs] == reg_array[rt] then jump to the instruction in the address reg_array[rm][low bits- 11:0]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int beq(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {

    if (reg_array[rs] == reg_array[rt])
    {
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        /*if the values are equal, jump to the address in rm*/
        return (reg_array[rm] & (0x0FFF));
    }
    return current_instruction + 1; /*if the values are not equal, continue to the next instruction*/
}
/*
bne - if reg_array[rs] != reg_array[rt] then jump to the instruction in the address reg_array[rm][low bits- 11:0]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int bne(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    if (reg_array[rs] != reg_array[rt])
    {
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        /*if the values are not equal, jump to the address in rm*/
        return (reg_array[rm] & (0x0FFF));
    }
    return current_instruction + 1; /*if the values are equal, continue to the next instruction*/
}
/*
blt - if reg_array[rs] < reg_array[rt] then jump to the instruction in the address reg_array[rm][low bits- 11:0]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int blt(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    if (reg_array[rs] < reg_array[rt])
    {
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        /*if the value in rs is less than the value in rt, jump to the address in rm*/
        return (reg_array[rm] & (0x0FFF));
    }
    return current_instruction + 1; /*if the value in rs is not less than the value in rt, continue to the next instruction*/
}
/*
bgt - if reg_array[rs] > reg_array[rt] then jump to the instruction in the address reg_array[rm][low bits- 11:0]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int bgt(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    if (reg_array[rs] > reg_array[rt])
    {
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        /*if the value in rs is greater than the value in rt, jump to the address in rm*/
        return (reg_array[rm] & (0x0FFF));
    }
    return current_instruction + 1; /*if the value in rs is not greater than the value in rt, continue to the next instruction*/
}
/*
ble - if reg_array[rs] <= reg_array[rt] then jump to the instruction in the address reg_array[rm][low bits- 11:0]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int ble(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    if (reg_array[rs] <= reg_array[rt])
    {
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        /*if the value in rs is less than or equal to the value in rt, jump to the address in rm*/
        return (reg_array[rm] & (0x0FFF));
    }
    return current_instruction + 1; /*if the value in rs is not less than or equal to the value in rt, continue to the next instruction*/
}
/*
bge - if reg_array[rs] >= reg_array[rt] then jump to the instruction in the address reg_array[rm][low bits- 11:0]
input - *reg_array, rd, rs, rt, rm, current_instruction
*/
int bge(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    if (reg_array[rs] >= reg_array[rt])
    {
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        /*if the value in rs is greater than or equal to the value in rt, jump to the address in rm*/
        return (reg_array[rm] & (0x0FFF));
    }
    return current_instruction + 1; /*if the value in rs is not greater than or equal to the value in rt, continue to the next instruction*/
}
/*
jal- jump and link, put the address of the next instruction in register rd and jump to the address in pc + 1
*/
int jal(int* reg_array, int rd, int rs, int rt, int rm, int current_instruction, unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    reg_array[rd] = current_instruction + 1; /*put the address of the next instruction in rd*/
    reg_array[1] = (int)unsigned_imm1;
    reg_array[2] = (int)unsigned_imm2;
    return (reg_array[rm] & (0x0FFF));
}

/***********************
 * IO Opcodes Section *
 ***********************/

 /*
 reti - return from interrupt, put the value in IOregisterd[rd] in the register rd
 input - *IOregisterd, *reg_array, rd, rs, rt, rm, current_instruction
  */
int reti(int* IOregisterd, int inside_irq, int current_instruction) {
    int temp_ins;
    int called_from_reti;
    inside_irq = 0; /*we are not inside an irq*/

    called_from_reti = 1;
    temp_ins = irq_handler(IOregisterd, inside_irq, current_instruction, called_from_reti);
    if (temp_ins != -1)
    {
        /*if we are inside an irq, return the address of the handler*/
        return temp_ins;
    }
    /*if we are not inside an irq, return the address in irqreturn*/
    return IOregisterd[7];
}
/*
in - put the value in io_array[reg_array[rs] + reg_array[rt]] in reg_array[rd]
input - rd, rs, rt, *reg_array, *io_array, *hwregtrace
*/
void in(int rd, int rs, int rt, int* reg_array, int* io_array, int* current_cycle, FILE* hwregtrace) {
    int reg_num = reg_array[rs] + reg_array[rt];

    if (reg_num == 22) {
        reg_array[rd] = 0; // A read from monitorcmd using the in instruction will return the value 0
    }

    else {
        if (reg_num == 6 || reg_num == 7 || reg_num == 16) {//registers representing addresses
            //reg_num 6 is a irqhandler, 7 is irqreturn, 16 is diskbuffer all of them are 12 bits
            reg_array[1] = (int)io_array[1];
            reg_array[2] = (int)io_array[2];
            reg_array[rd] = io_array[reg_num] & 0xFFF;

        }
        else if (reg_num == 20) {
            //reg_num 20 is monitoraddr, 16 bits 
            reg_array[1] = (int)io_array[1];
            reg_array[2] = (int)io_array[2];
            reg_array[rd] = io_array[reg_num] & 0xFFFF;

        }
        else {
            //for any other register
            reg_array[rd] = io_array[reg_num]; // Updating $rd register with the IO input
        }
    }
    hwregout_printer(reg_num, 0, io_array, current_cycle, hwregtrace);
}

/*
out - put the value in reg_array[rm] in io_array[reg_array[rs] + reg_array[rt]]
input - rm, rs, rt, *reg_array, *io_array, *hwregtrace_filename, *leds_filename, display7seg_filename, screen
*/
void out(int rm, int rs, int rt, int* reg_array, int* io_array, int* current_cycle, char** memlines, char** disklines, FILE* hwregtrace, FILE* leds, FILE* display7seg, int screen[SCREENSIZE], unsigned int unsigned_imm1, unsigned int unsigned_imm2) {
    int reg_num = reg_array[rs] + reg_array[rt];

    if (reg_num == 6 || reg_num == 7 || reg_num == 16) {//registers representing addresses
        //reg_num 6 is a irqhandler, 7 is irqreturn, 16 is diskbuffer all of them are 12 bits
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        io_array[reg_num] = reg_array[rm] & 0xFFF; // Updating the IO register with the value in $rm
    }

    else if (reg_num == 20) {
        //reg_num 20 is monitoraddr, 16 bits 
        reg_array[1] = (int)unsigned_imm1;
        reg_array[2] = (int)unsigned_imm2;
        io_array[reg_num] = reg_array[rm] & 0xFFFF; // 16 bits for monitoraddr
    }

    else {
        //for any other register
        io_array[reg_num] = reg_array[rm]; // Updating the IO register with the value in $rm
    }

    if (reg_num == 9) {
        //If leds are been changed, the clock cycle and the status of all leds are written to leds.txt
        fprintf(leds, "%d %08X\n", *current_cycle, io_array[9]);
    }
    if (io_array[22] == 1) { // monitorcmd register
        screen[io_array[20]] = io_array[21];
    }
    if (reg_num == 10) {
        //If display7seg is been changed, the clock cycle and the status of display7seg are written to display7seg.txt
        fprintf(display7seg, "%d %08X\n", *current_cycle, io_array[10]);
    }
    if (reg_num == 14) {
        //If diskcmd is been changed go to itq1_check
        irq1_check(io_array, disklines, memlines);
    }
    hwregout_printer(reg_num, 1, io_array, current_cycle, hwregtrace);

    if (io_array[22] == 1) {
        io_array[22] = 0; // Reset monitor command after update;
    }
}
/************************************************************************/

/*
 write the screen array to the monitor.txt and monitor.yuv files
    input - screen, monitor_filename, yuv_filename
    output - None(void)
*/
void monitoryuv_printer(int screen[SCREENSIZE], const char* monitor_filename, const char* yuv_filename) {
    unsigned char* pointer;
    unsigned char temp;
    int i = 0;
    int last = 0;
    /*find last value which is not zero*/
    for (i = 65534; i >= 0; i--) {
        if (screen[i] != 0) {
            last = i;
            break;
        }
    }
    FILE* monitor = fopen(monitor_filename, "w");
    /*check if the file open*/
    if (monitor == NULL) {
        printf("Error opening file");
        exit(1);
    }
    FILE* yuv = fopen(yuv_filename, "wb");
    /*check if the file open*/
    if (yuv == NULL) {
        printf("Error opening file");
        exit(1);
    }
    for (i = 0; i < SCREENSIZE; i++) {
        temp = screen[i];
        pointer = &temp;
        fwrite(pointer, 1, 1, yuv);
        fprintf(monitor, "%02X\n", screen[i]);
    }
    fclose(monitor);
    fclose(yuv);
}

char* hex_to_bin(char* hex) {
    // Lookup table for the binary representation of each hexadecimal digit
    char* lookup_table[16] = {
        "0000", "0001", "0010", "0011",
        "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011",
        "1100", "1101", "1110", "1111"
    };
    // Allocate memory for the binary string
    char* bin = malloc(33); // 32 characters + null terminator
    if (bin == NULL) {
        printf("Failed to allocate memory for bin\n");
        return NULL;
    }
    // Null-terminate the string
    bin[32] = '\0';
    // Convert each character of the hexadecimal string
    for (int i = 0; i < 8; i++) {
        int index;
        if (hex[i] >= '0' && hex[i] <= '9') {
            index = hex[i] - '0';
        }
        else if (hex[i] >= 'A' && hex[i] <= 'F') {
            index = hex[i] - 'A' + 10;
        }
        else if (hex[i] >= 'a' && hex[i] <= 'f') {
            index = hex[i] - 'a' + 10;
        }
        else {
            free(bin);
            return NULL;
        }
        strncpy(bin + i * 4, lookup_table[index], 4);
    }
    return bin;
}

int twos_complement(char* hex, int three_letters) {
    /*make 3 chars hex to 8 chars using sign extention*/
    char* new_hex;
    char* bin;
    if (hex[0] <= '7')
    {
        return (int)strtol(hex, NULL, 16);
    }
    if (three_letters == 1) {
        new_hex = (char*)malloc(sizeof(char) * 9); /* allocate 9 chars for 8 hex digits and the null terminator */
        if (new_hex == NULL) {
            printf("Memory allocation failed\n");
            // Handle memory allocation failure
            return -1; // or any other appropriate action
        }
        new_hex[7] = hex[2];
        new_hex[6] = hex[1];
        new_hex[5] = hex[0];
        if (new_hex == NULL) {
            printf("Memory allocation failed\n");
            // Handle memory allocation failure
            return -1; // or any other appropriate action
        }
        /*if the number is negative, make the 3 chars hex to 8 chars using sign extention*/
        for (int i = 0; i < 5; i++) {
            new_hex[i] = 'F';
        }
        new_hex[8] = '\0';
        bin = hex_to_bin(new_hex); /*new_hex from hex to binary*/
    }
    else {
        bin = hex_to_bin(hex);     /*new_hex from hex to binary*/

    }
    /*flip each bit*/
    for (int i = 0; i < 32; i++) {
        if (bin[i] == '1') {
            bin[i] = '0';
        }
        else {
            bin[i] = '1';
        }
    }
    /*add one to the binary*/
    for (int i = 31; i >= 0; i--) {
        if (bin[i] == '0') {
            bin[i] = '1';
            break;
        }
        else {
            bin[i] = '0';
        }
    }
    /*convert the binary to decimal*/
    int decimal = (int)strtol(bin, NULL, 2);
    return -decimal;
}

/*
ecxecute the instruction in the code
inputs- char** instructions,char** dataMemoryLines, int* registers, int* IOregisterd, int* dataMemory, int current_instruction, char* trace_filename, FILE* hwregtrace, FILE* leds, FILE* display7seg, int* screen
output- 1 if the program should exit, 0 otherwise
*/
int ecxecute_instruction(char** instructions, char** dataMemoryLines, char** diskLines, int* registers, int* IOregisterd, int instructions_size, int* current_instruction, int* current_cycle, FILE* trace, FILE* hwregtrace, FILE* leds, FILE* display7seg, int* screen, size_t* line_amount)
{
    char* instruction;
    int opcode;
    char opcode_char[3];
    int rd;
    char rd_char[2];
    int rs;
    char rs_char[2];
    int rt;
    char rt_char[2];
    int rm;
    char rm_char[2];
    int imm1;
    unsigned int unsigned_imm1;
    char imm1_char[4];
    int imm2;
    unsigned int unsigned_imm2;
    char imm2_char[4];
    int next_instruction;
    int exit;
    int temp_ins;
    char newline[2] = "\n";
    int keep_rd;
    temp_ins = -1;
    exit = 0;
    next_instruction = 0;
    /*instruction in place current_instruction*/
    instruction = instructions[(*current_instruction)];
    /*copy opcode*/
    strncpy(opcode_char, instruction, 2); opcode_char[2] = '\0';
    opcode = (int)strtol(opcode_char, NULL, 16);
    /*copy rd*/
    strncpy(rd_char, &instruction[2], 1); rd_char[1] = '\0';
    rd = (int)strtol(rd_char, NULL, 16);
    /*copy rs*/
    strncpy(rs_char, &instruction[3], 1); rs_char[1] = '\0';
    rs = (int)strtol(rs_char, NULL, 16);
    /*copy rt*/
    strncpy(rt_char, &instruction[4], 1); rt_char[1] = '\0';
    rt = (int)strtol(rt_char, NULL, 16);
    /*copy rm*/
    strncpy(rm_char, &instruction[5], 1); rm_char[1] = '\0';
    rm = (int)strtol(rm_char, NULL, 16);
    /*copy imm1*/
    strncpy(imm1_char, &instruction[6], 3); imm1_char[3] = '\0';
    unsigned_imm1 = strtoul(imm1_char, NULL, 16);
    /*copy imm2*/
    strncpy(imm2_char, &instruction[9], 3); imm2_char[3] = '\0';
    unsigned_imm2 = strtoul(imm2_char, NULL, 16);
    /*convert imm1 and imm2 to twos complement*/
    imm1 = twos_complement(imm1_char, 1);
    imm2 = twos_complement(imm2_char, 1);
    /*update imms registers*/
    registers[1] = imm1;
    registers[2] = imm2;

    keep_rd = registers[rd]; /*keep the value of rd in case register 0,1 or 2 changed*/
    write_to_trace(registers, *current_instruction, instruction, trace);
    switch (opcode)
    {
    case 0:
        /*add*/
        next_instruction = add(registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 1:
        /*sub*/
        next_instruction = sub(registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 2:
        /*mac*/
        next_instruction = mac(registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 3:
        /*and*/
        next_instruction = and (registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 4:
        /*or*/
        next_instruction = or (registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 5:
        /*xor*/
        next_instruction = xor (registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 6:
        /*sll*/
        next_instruction = sll(registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 7:
        /*sra*/
        next_instruction = sra(registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 8:
        /*srl*/
        next_instruction = srl(registers, rd, rs, rt, rm, (*current_instruction));
        break;
    case 9:
        /*beq*/
        next_instruction = beq(registers, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 10:
        /*bne*/
        next_instruction = bne(registers, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 11:
        /*blt*/
        next_instruction = blt(registers, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 12:
        /*bgt*/
        next_instruction = bgt(registers, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 13:
        /*ble*/
        next_instruction = ble(registers, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 14:
        /*bge*/
        next_instruction = bge(registers, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 15:
        /*jal*/
        next_instruction = jal(registers, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 16:
        /*lw*/
        next_instruction = lw(registers, dataMemoryLines, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 17:
        /*sw*/
        next_instruction = sw(registers, dataMemoryLines, rd, rs, rt, rm, (*current_instruction), unsigned_imm1, unsigned_imm2);
        break;
    case 18:
        /*reti*/
        next_instruction = reti(IOregisterd, inside_irq, (*current_instruction));
        break;
    case 19:
        /*in*/
        in(rd, rs, rt, registers, IOregisterd, current_cycle, hwregtrace);
        next_instruction = (*current_instruction) + 1;
        break;
    case 20:
        /*out*/
        out(rm, rs, rt, registers, IOregisterd, current_cycle, dataMemoryLines, diskLines, hwregtrace, leds, display7seg, screen, unsigned_imm1, unsigned_imm2);
        next_instruction = (*current_instruction) + 1;
        break;
    default: /*case 21- halt*/
        exit = 1; /*exit the loop*/
        break;
    }
    if (next_instruction >= instructions_size) {
        /*if the next instruction is out of bounds, exit the loop*/
        exit = 1;
    }
    if (rd == 0 || rd == 1 || rd == 2) {
        /*if this the register is 0, 1, or 2, don't write to it*/
        registers[rd] = keep_rd;
    }
    /*print current instruction in trace file*/
    (*current_cycle)++; /*increment the cycle*/
    IOregisterd[8] = IOregisterd[8] + 1; /*increment the clock cycle*/
    if (IOregisterd[8] == UINT32_MAX) /*if the clock cycle is at its maximum value, reset it*/
    {
        IOregisterd[8] = 0; /*reset the clock cycle*/
    }

    (*current_instruction) = next_instruction;
    return exit; /*return 1 if the program should exit, 0 otherwise*/

}

/*free the lines array*/
void free_lines_char(char** lines, size_t size) {
    for (size_t  i = 0; i < size; i++) {
        free(lines[i]);
    }
    free(lines);
}

/*
print the memory array to a file and free the array
input- char** array, const char* filename, int length_of_array
output- None(void)
*/
void print_memory_array_to_file(char** array, const char* filename, int length_of_array) {
    /* Find the index of the last non-zero line */
    int last_non_zero_index = -1;
    for (int i = 0; i < length_of_array; i++) {
        if (strcmp(array[i], "00000000") != 0) {
            last_non_zero_index = i;
        }
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Failed to open file: %s\n", filename);
        return;
    }

    /* If all lines are "00000000", don't write anything to the file*/
    if (last_non_zero_index == -1) {
        fprintf(file, "\n");
        fclose(file);
        return;
    }

    for (int i = 0; i <= last_non_zero_index; i++) {
        /* Write the line to the file to the last significant value*/
        fprintf(file, "%s\n", array[i]);
    }
    /* Close the file */
    fclose(file);
}

/*
write registers R3 to R15 into file
input- int registers[REGISTERS_COUNT], char* filename
output- None(void)
*/
void write_to_regout(int* registers, const char* filename) {
    FILE* regout = fopen(filename, "w");
    if (regout == NULL) {
        printf("Error opening file");
        exit(1);
    }
    for (int i = 0; i < REGISTERS_COUNT; i++) {
        if (i >= 3 && i <= 15) {
            fprintf(regout, "%08X\n", registers[i]);
        }
    }
    fclose(regout);
}

/*
write a line to the trace file in the format
PC INST R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15
input- int *registers, int pc, char* instruction, char* file_name
output- None(void)
*/
void write_to_trace(int* registers, int pc, char* instruction, FILE* trace) {
    if (trace == NULL) {
        printf("Error opening file");
        exit(1);
    }
    fprintf(trace, "%03X %s ", pc, instruction);
    for (int i = 0; i < REGISTERS_COUNT - 1; i++) {
        fprintf(trace, "%08X ", registers[i]);
    }
    fprintf(trace, "%08X\n", registers[15]);
    return;
}

char* alloc_and_copy(const char* src) {
    // Allocate memory for the copy
    char* copy = malloc(strlen(src) + 1);
    if (copy == NULL) {
        // Handle allocation failure
        return NULL;
    }

    // Copy the data
    strcpy(copy, src);

    // Return the copy
    return copy;
}

/*
function that reads a file and returns an array of strings, each string is a line in the file
if the file has less lines than num_lines, the rest of the array will be filled with "00000000"
input- const char* filename, int num_lines
output- char** lines
*/
char** from_file_to_array(const char* filename, int num_lines, int max_line_length) {
    size_t line_amount;
    char** lines;
    char** temp_lines;
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open file: %s\n", filename);
        return NULL;
    }

    temp_lines = read_file_to_array(max_line_length, file, &line_amount); /*read the file to an array of strings*/
    lines = malloc(num_lines * sizeof(char*));
    if (lines == NULL) {
        printf("Failed to allocate memory for lines\n");
        return NULL;
    }
    /*copy temp_lines to lines, if line_amount < num_lines add lines of "00000000"*/
    for (int i = 0; i < num_lines; i++) {
        if (i < line_amount) {
            lines[i] = alloc_and_copy(temp_lines[i]);
        }
        else {
            lines[i] = alloc_and_copy("00000000");
        }
    }
    /*free temp_lines*/
    free_lines_char(temp_lines, line_amount);

    fclose(file);
    return lines;
}

/*
run the functions in the code
input- char** instructions,char** dataMemoryLines, char** disk_lines, char* dmemout, char* diskout, char* regout, char* trace_filename,
char* irq2in, char* hwregtrace_filename, char* leds_filename, char* display7seg_filename, char* monitortxt, char* monitoryuv, char* cycles
output- None(void)
*/
void run_code(char** instructions, char** dataMemoryLines, char** disk_lines, const char* dmemout, const char* diskout, const char* regout, const char* trace_filename, const char* forirq2in, const char* hwregtrace_filename, const char* leds_filename,
    const char* display7seg_filename, const char* monitortxt, const char* monitoryuv, const char* cycles_filename, size_t* line_amount) {
    int i;
    int instructions_size;
    /*init the registers and the IO registers*/
    int* registers = (int*)calloc(REGISTERS_COUNT, sizeof(int));
    if (registers == NULL) {
        printf("Memory allocation failed for screen\n");
        exit(1); // Or handle the failure appropriately
    }

    /*init the screen with 0*/
    for (i = 0; i < REGISTERS_COUNT; i++) {
        registers[i] = 0;
    }
    int* IOregisterd = (int*)calloc(IOREGISTERS_COUNT, sizeof(int));
    if (IOregisterd == NULL) {
        printf("Memory allocation failed for screen\n");
        exit(1); // Or handle the failure appropriately
    }

    /*init the screen with 0*/
    for (i = 0; i < IOREGISTERS_COUNT; i++) {
        IOregisterd[i] = 0;
    }
    int* screen = (int*)calloc(SCREENSIZE, sizeof(int));
    if (screen == NULL) {
        printf("Memory allocation failed for screen\n");
        exit(1); // Or handle the failure appropriately
    }

    /*init the screen with 0*/
    for (i = 0; i < SCREENSIZE; i++) {
        screen[i] = 0;
    }
    int exit_prog;
    int current_interrupt_irq2 = -1; /*init with -1*/
    int called_from_reti;
    int current_instruction = 0; /*index of the current instruction start with the first in place 0*/
    int current_cycle = 0; /*start with cycle 0*/
    int flag_EOF_irq2 = 1; /*flag for end of file for irq2, 1 not EOF, otherwise 0*/
    exit_prog = 0; /*exit flag*/
    inside_irq = 0; /*we are not inside an irq*/

    FILE* irq2in = fopen(forirq2in, "r");
    FILE* trace = fopen(trace_filename, "w");
    FILE* hwregtrace = fopen(hwregtrace_filename, "w");
    FILE* leds = fopen(leds_filename, "w");
    FILE* display7seg = fopen(display7seg_filename, "w");
    FILE* cycles = fopen(cycles_filename, "w");

    if (irq2in == NULL) {
        printf("Error opening file");
        exit(1);
    }

    /*start irq_2*/
    irq2_checker(IOregisterd, irq2in, &current_interrupt_irq2, &current_cycle, &flag_EOF_irq2);

    instructions_size = (int)*line_amount;
    if (instructions_size == 0) {
        /*if the instructions array is empty, exit the program*/
        exit_prog = 1;
    }

    while (exit_prog == 0) {
        /*run the code*/
        /*check interups*/
        disk_timer(IOregisterd); /*disk timer*/
        irq0_checker(IOregisterd);
        irq2_checker(IOregisterd, irq2in, &current_interrupt_irq2, &current_cycle, &flag_EOF_irq2);
        called_from_reti = 0;
        current_instruction = irq_handler(IOregisterd, 0, current_instruction, called_from_reti);
        /*execute the instruction*/
        exit_prog = ecxecute_instruction(instructions, dataMemoryLines, disk_lines, registers, IOregisterd,
            instructions_size, &current_instruction, &current_cycle, trace, hwregtrace, leds, display7seg, screen, line_amount);
    }

    /*write the data memory to the file*/
    print_memory_array_to_file(dataMemoryLines, dmemout, MEMORYSIZE);
    /*write the disk to the file*/
    print_memory_array_to_file(disk_lines, diskout, DISKSIZE);
    /*write the registers to the file*/
    write_to_regout(registers, regout);
    /*write the screen to the file*/
    monitoryuv_printer(screen, monitortxt, monitoryuv);
    /*write the cycles to the file*/
    fprintf(cycles, "%d\n", current_cycle);

    /*close the files*/
    fclose(irq2in);
    fclose(trace);
    fclose(hwregtrace);
    fclose(leds);
    fclose(display7seg);
    fclose(cycles);

    /* Free the memory allocated for the lines */
    free_lines_char(dataMemoryLines, MEMORYSIZE);
    free_lines_char(disk_lines, DISKSIZE);
    free_lines_char(instructions, instructions_size);
    free(registers);
    free(IOregisterd);
    free(screen);

    return;
}

int main(int argc, char* argv[]) {
    const char* forimemin = argv[1];
    const char* dmemin = argv[2];
    const char* diskin = argv[3];
    const char* irq2in = argv[4];
    const char* dmemout = argv[5];
    const char* regout = argv[6];
    const char* trace = argv[7];
    const char* hwregtrace = argv[8];
    const char* cycles = argv[9];
    const char* leds = argv[10];
    const char* display7seg = argv[11];
    const char* diskout = argv[12];
    const char* monitortxt = argv[13];
    const char* monitoryuv = argv[14];
    size_t line_amount;
    if (argc != 15) {
        printf("Error in ARGS");
        return 1;
    }
    /*read imemin into array*/
    FILE* imemin = fopen(forimemin, "r");
    if (imemin == NULL) {
        printf("Error opening file");
    }
    char** imemin_lines = read_file_to_array(IMEMIN_LINE_LENGTH, imemin, &line_amount);
    if (imemin != NULL) {
        fclose(imemin);
    }

    /*read imemin into array*/
    char** dmemin_lines = from_file_to_array(dmemin, MEMORYSIZE, DMEMIN_LINE_LENGTH);
    /*read dmemin into array*/
    char** disk_lines = from_file_to_array(diskin, DISKSIZE, DISK_LINE_LENGTH);
    run_code(imemin_lines, dmemin_lines, disk_lines, dmemout, diskout, regout, trace, irq2in, hwregtrace, leds, display7seg, monitortxt, monitoryuv, cycles, &line_amount);
    return 0;
}
