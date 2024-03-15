#define _CRT_SECURE_NO_WARNINGS // Disable deprecation warning for fopen
#ifdef _MSC_VER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#endif

#define MAX_LINE 500
#define MEMORYSIZE 4096 // 4KB
/*******
 * This file contains the functions that are used to create the assembler
*******/

/**
 * Label section
**/
/*Struct that keeps Label data- name, it location in the assembly code and the next label*/
typedef struct Label {
    char name[50]; /*label's name, max size is 50*/
    char address[50]; /*label's location in the assembly code*/
    struct Label* next; /*next label in the list*/
} Label;

void assignAddress(struct Label* newLabel, int address) {
    sprintf(newLabel->address, "%d", address);
}

/*
Add new label to the end of the labels list
input- head_Label, label's name and it's location in the code
output- none (void)
*/
void addLabel(Label** head, char* name, int address) {
    /*Make new label*/
    Label* newLabel;
    newLabel = (Label*)malloc(sizeof(Label));
    if (newLabel == NULL) {
        printf("Error");
        return;
    }
    strcpy(newLabel->name, name);
    assignAddress(newLabel, address);
    newLabel->next = NULL;
    /*if no head yet, make the new label the head label*/
    if (*head == NULL) {
        *head = newLabel;
    }
    else {
        /*put the new label last in the list*/
        Label* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newLabel;
    }
}

/*
Find label location by it's name
input- head_Label, and label's name
output label's location in the code
*/
char* findLabel(Label* head, char* name) {
    Label* current = head;
    while (current != NULL) {
        /*if the current node's label is the same as we want, return it address*/

        if (strcmp(current->name, name) == 0) {
            return current->address;
        }
        current = current->next;
    }
    /*No address to be found*/
    return NULL;
}

/*
Destroy the labels list
input- head_Label
output- none (void)
*/
void destroy_labels(Label* head) {
    Label* current = head;
    while (current != NULL) {
        Label* temp = current;
        /*move to the next label*/
        current = current->next;
        /*free the current label*/
        free(temp);
    }
}

/***
 * Memory section
***/
/*
instructionMemory line part
*/
/*instructionMemory struct contains: place of line of code, opcode, rd, rs, rt, imm1, imm2*/
typedef struct instructionMemory {
    int place; /*place of line of code*/
    char* opcode; /*opcode*/
    char* rd; /*register rd*/
    char* rs; /*register rs*/
    char* rt; /*register t*/
    char* rm; /*register rm*/
    char* imm1; /*imm1*/
    char* imm2; /*imm2*/
    struct instructionMemory* next; /*next memory line in the list*/
} instructionMemory;

/*
Make new instructionMemory line
input: place of line of code, opcode, rd, rs, rt, imm1, imm2
output: new memory line
*/
instructionMemory* makeMemoryLine(int place, const char opcode[20], const char rd[20], const char rs[20], const char rt[20], const char rm[20], const char imm1[20], const char imm2[20], instructionMemory* next) {
    instructionMemory* newInsMemory = (instructionMemory*)malloc(sizeof(instructionMemory));
    if (newInsMemory == NULL) {
        // Handle memory allocation failure
        return NULL;
    }
    newInsMemory->place = place;

    // Copy opcode
    if (opcode != NULL && strlen(opcode) < 20) {
        newInsMemory->opcode = _strdup(opcode);
    }
    else {
        // Assign default value if opcode is NULL or too long
        newInsMemory->opcode = _strdup("default_opcode");
    }

    // Copy rd
    if (rd != NULL && strlen(rd) < 20) {
        newInsMemory->rd = _strdup(rd);
    }
    else {
        newInsMemory->rd = _strdup("default_rd");
    }

    // Copy rs
    if (rs != NULL && strlen(rs) < 20) {
        newInsMemory->rs = _strdup(rs);
    }
    else {
        newInsMemory->rs = _strdup("default_rs");
    }

    // Copy rt
    if (rt != NULL && strlen(rt) < 20) {
        newInsMemory->rt = _strdup(rt);
    }
    else {
        newInsMemory->rt = _strdup("default_rt");
    }

    // Copy rm
    if (rm != NULL && strlen(rm) < 20) {
        newInsMemory->rm = _strdup(rm);
    }
    else {
        newInsMemory->rm = _strdup("default_rm");
    }

    // Copy imm1
    if (imm1 != NULL && strlen(imm1) < 20) {
        newInsMemory->imm1 = _strdup(imm1);
    }
    else {
        newInsMemory->imm1 = _strdup("default_imm1");
    }

    // Copy imm2
    if (imm2 != NULL && strlen(imm2) < 20) {
        newInsMemory->imm2 = _strdup(imm2);
    }
    else {
        newInsMemory->imm2 = _strdup("default_imm2");
    }

    // Set next pointer
    newInsMemory->next = next;
    return newInsMemory;
}


/*
Add new instructionMemory to the end of the memory lines list
input- head_instructionMemory, place of line of code, opcode, rd, rs, rt, imm1, imm2
output- none (void)
*/
void addInsMemory(instructionMemory** head, int place, char* opcode, char* rd, char* rs, char* rt, char* rm, char* imm1, char* imm2) {
    /*Make new memory line*/
    instructionMemory* newMemoryLine = makeMemoryLine(place, opcode, rd, rs, rt, rm, imm1, imm2, NULL);
    /*if no head yet, make the new instruction memory line the head memory line*/
    if (*head == NULL) {
        *head = newMemoryLine;
    }
    else {
        /*put the new instruction memory line last in the list*/
        instructionMemory* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newMemoryLine;
    }
}

/*
Destroy the instruction memory lines list
input- head_instructionMemory
output- none (void)
*/
void destroy_memory_lines(instructionMemory* head) {
    instructionMemory* current = head;
    while (current != NULL) {
        instructionMemory* temp = current;
        /*move to the next memory line*/
        current = current->next;
        /*free the current memory line*/
        free(temp);
    }
}

/*
data memory part
*/
/*Data memory struct contains: place, value*/
typedef struct dataMemory {
    int place; /*place of line of code*/
    int value; /*value*/
    struct dataMemory* next; /*next data memory line in the list*/
} dataMemory;
/*
Make data memory  line
input: place of line of code, value
output: new memory line
*/
dataMemory* makeDataMemoryLine(int place, int value, dataMemory* next) {
    dataMemory* newDataMemory = (dataMemory*)malloc(sizeof(dataMemory));
    if (newDataMemory == NULL) {
        // Handle memory allocation failure
        printf("Error: memory allocation failed\n");
        return NULL;
    }
    newDataMemory->place = place;
    newDataMemory->value = value;
    newDataMemory->next = next;
    return newDataMemory;
}
/*
Add new dataMemory to the end of the memory lines list
input- head_dataMemory, place of line of code, opcode, rd, rs, rt, imm1, imm2
output- none (void)
*/
void addDataMemory(dataMemory** head, int place, int value) {
    /*Make new memory line*/
    dataMemory* newDataMemory = makeDataMemoryLine(place, value, NULL);
    /*if no head yet, make the new data memory line the head memory line*/
    if (*head == NULL) {
        *head = newDataMemory;
    }
    else {
        /*put the new data memory line last in the list*/
        dataMemory* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newDataMemory;
    }
}

/*
Destroy the instruction memory lines list
input- head_instructionMemory
output- none (void)
*/
void destroy_data_memory(dataMemory* head) {
    dataMemory* current = head;
    while (current != NULL) {
        dataMemory* temp = current;
        /*move to the next memory line*/
        current = current->next;
        /*free the current memory line*/
        free(temp);
    }
}

/*
Total memory struct contains: instruction memory and data memory
*/
typedef struct totalMemory {
    instructionMemory* head_instructionMemory; /*head of instruction memory*/
    dataMemory* head_dataMemory; /*head of data memory*/
} totalMemory;

/*
Returns if there is a label in the line, if there is, return the place of :
input- line
output- the place of : if there is a label, -1 if not
*/
int there_is_a_label(char* line) {
    int there_is_a_label;
    there_is_a_label = -1;
    for (int i = 0; i < sizeof(line); i++)
    {
        if (line[i] == '#')
        {
            there_is_a_label = -1;
            break;
        }
        if (line[i] == ':')
        {
            there_is_a_label = i;
            break;
        }
    }

    return there_is_a_label;
}

/*
Returns if there is a function in the line
input- line
output- 1 if there is a function, 0 if not
*/
int there_is_a_function(char* line) {
    int there_is_a_function = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '#') break;
        if (line[i] == ':') continue;
        if (line[i] == '$') {
            there_is_a_function = 1;
            break;
        }
    }
    return there_is_a_function;
}

/*
First pass on the input txt- find the labels and save them in the labels list
input- input txt file
output- head_Label
*/
Label* first_pass(FILE* file) {
    /*Make the head label*/
    Label* head_Label = NULL;
    /*Make the current line, max is 500*/
    char current_line[MAX_LINE];
    /*Make the current place*/
    int current_place = 0;
    /*Make the current label*/
    char current_label_name[50] = "";

    /*Save if found label- found - 1 not found - 0*/
    int found_label;
    int place_in_word;
    int i;
    int temp_place;

    current_place = 0;
    found_label = 0;
    place_in_word = 0;
    temp_place = 0;

    while (fgets(current_line, MAX_LINE, file) != NULL)
    {
        if (there_is_a_function(current_line) == 1)
        {
            current_place++;
        }

        for (int place = 0; place < sizeof(current_line); place++) {
            //if the line is a label
            if (current_line[place] == ':')
            {

                // save label name
                for (int i = 0; i < place; i++)
                {
                    if (current_line[i] == ' ' || current_line[i] == '\t' || current_line[i] == '\n') continue;
                    current_label_name[place_in_word] = current_line[i];
                    place_in_word++;
                    found_label = 1;
                }
            }
            if (found_label == 1)
            {
                // add label to the list
                if ((there_is_a_function(current_line) == 1) && (there_is_a_label(current_line) != -1)) {
                    temp_place = current_place - 1;
                }
                else {
                    temp_place = current_place;
                }
                addLabel(&head_Label, current_label_name, temp_place);
                //reset the variables
                found_label = 0;
                place_in_word = 0;
                for (i = 0; i < 50; i++) {
                    current_label_name[i] = '\0';
                }
            }
        }

    }
    /* Close the file */
    fclose(file);
    /*return the labels list*/
    return head_Label;
}

/*
Help function that find out if a char is a digit
input- char
output- 1 if it is a digit, 0 if not
*/
int charIsDigit(char c) {
    /* Check if the character is a digit (0-9)*/
    if (c >= '0' && c <= '9') {
        return 1;  /* True (character is a digit)*/
    }
    else {
        return 0;  /* False (character is not a digit) */
    }
}
/*
Help function that find out if a char is a letter
input- char
output- 1 if it is a letter, 0 if not
*/
int charIsLetter(char c) {
    /* Check if the character is a letter (a-z, A-Z)*/
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return 1;  /* True (character is a letter)*/
    }
    else {
        return 0;  /* False (character is not a letter) */
    }
}

/*
from string to number
input- string
output- int (the decimal number), -1 if there is an error
*/
int fromStringToNumber(char* string) {
    int number;
    number = 0;

    for (int i = 0; i < strlen(string); i++)
    {
        /*is hex number*/
        if (string[0] == '0' && string[1] == 'x')
        {
            if (i > 1)
            {
                if ((string[i] - '0' >= 0) && (string[i] - '0' < 10))
                {
                    number = number * 16 + (string[i] - '0');
                }
                else if ((string[i] - 'A' >= 0) && (string[i] - 'A' < 6))
                {
                    number = number * 16 + (string[i] - 'A' + 10);
                }
                else if ((string[i] - 'a' >= 0) && (string[i] - 'a' < 6))
                {
                    number = number * 16 + (string[i] - 'a' + 10);
                }
            }
        }
        /*is negetive number*/
        else if (string[0] == '-')
        {
            if (i > 0)
            {
                number = number * 10 + (string[i] - '0');
            }
        }
        else /*is positive number*/
        {
            number = number * 10 + (string[i] - '0');
        }
    }
    if (string[0] == '-')
    {
        number = -number;
    }
    return number;
}

/*
help function that copy a section from a string from place i to place j to new string
input- string, i, j
output- new string
*/
char* copySection(char* string, int i, int j) {
    char* newString;
    int k;
    k = 0;
    newString = (char*)malloc(sizeof(char) * (j - i + 1));
    if (newString == NULL) {
        // Handle memory allocation failure
        printf("Error: memory allocation failed\n");
        return NULL;
    }
    for (int place = i; place < j; place++)
    {
        newString[k] = string[place];
        k++;
    }
    newString[k] = '\0';
    return newString;
}

void strcpy_ignore_spaces(char* dest, const char* src) {
    while (*src) {
        if (*src != ' ') {
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0';
}

/*
Second pass on the input txt- find the commands and save them in the memory lines list
input- input txt file
output- head_MemoryLine
*/
totalMemory* second_pass(char* input_file) {
    /*Open the input file*/
    FILE* file;
    file = fopen(input_file, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        exit(1);
    }
    /*Make the head memory line*/
    instructionMemory* head_MemoryLine = NULL;
    /*Make the current line, max is 500*/
    char current_line[500];
    /*Make the current place*/
    int current_place;
    /*Make the current opcode*/
    char current_opcode[20];
    /*Make the current rd*/
    char current_rd[20];
    /*Make the current rs*/
    char current_rs[20];
    /*Make the current rt*/
    char current_rt[20];
    /*Make the current rm*/
    char current_rm[20];
    /*Make the current imm1*/
    char current_imm1[20];
    /*Make the current imm2*/
    char current_imm2[20];
    /*Make the data memory head*/
    dataMemory* head_dataMemory = NULL;
    char* reg;
    int label_place;
    label_place = -1;
    int begin;
    int end;
    int place;
    int value;
    place = 0;
    value = 0;
    begin = 0;
    end = 0;

    char dotword[6];
    strcpy(dotword, ".word");
    char* start;
    char current_place_char[100];
    char value_char[100];
    start = &current_line[0];

    totalMemory* total_memory = (totalMemory*)malloc(sizeof(totalMemory));

    /*Save the current place*/
    current_place = 0;
    while (fgets(current_line, sizeof(current_line), file) != NULL) {
        label_place = there_is_a_label(current_line);
        if (*current_line == '\n' || current_line[0] == '#') continue;
        start = current_line;
        while (*start == ' ' || *start == '\t') { start++; }
        if (strncmp(start, ".word", 5) == 0) {
            begin = 5;
            while (charIsDigit(current_line[begin]) != 1 && charIsLetter(current_line[begin]) != 1) begin++;
            if (sscanf(start, ".word %s %s", current_place_char, value_char) == 2) {
                while (current_place < 0) current_place += MEMORYSIZE;
                current_place = fromStringToNumber(current_place_char);
                value = fromStringToNumber(value_char);
                addDataMemory(&head_dataMemory, (current_place % MEMORYSIZE), value);
                current_place++;
            }
            else {
                // Handle sscanf failure
                printf("Error: sscanf failed to parse input string\n");
            }
        }
        if ((strncmp(start, "word", 4) == 0) && current_line[0] == '.') {
            begin = 5;
            while (charIsDigit(current_line[begin]) != 1 && charIsLetter(current_line[begin]) != 1) begin++;
            int sscanf_result = sscanf(start, "word %s %s", current_place_char, value_char);
            if (sscanf_result == 2) {
                addDataMemory(&head_dataMemory, current_place, value);
                current_place++;
            }
            else {
                // Handle sscanf failure
                printf("Error: sscanf failed to parse input string\n");
            }
        }
        if (there_is_a_function(current_line) == 1) {
            /* save in instructionMemory */
            begin = 0;
            while (current_line[begin] == ' ' || current_line[begin] == '\t') {
                begin++;
            }
            end = begin;
            while (current_line[end] != ' ' && current_line[end] != ':') end++;
            if (current_line[end] == ':') {
                begin = end + 1;
                while (current_line[begin] == ' ') begin++;
                end = begin;
                while (current_line[end] != ' ' && current_line[end] != ':') end++;

            }
            /*saves opcode*/
            reg = copySection(current_line, begin, end);
            strcpy(current_opcode, reg);
            free(reg);
            begin = end + 1;
            /*checks the place of register rd*/
            while (current_line[begin] != '$') begin++;
            end = begin;
            while (current_line[end] != ',') end++;
            /*saves register rd*/
            reg = copySection(current_line, begin, end);
            strcpy_ignore_spaces(current_rd, reg);
            free(reg);
            begin = end + 1;
            /*checks the place of register rs*/
            while (current_line[begin] != '$') begin++;
            end = begin;
            while (current_line[end] != ',') end++;
            /*saves register rs*/
            reg = copySection(current_line, begin, end);
            strcpy_ignore_spaces(current_rs, reg);
            free(reg);
            begin = end + 1;
            /*checks the place of register rt*/
            while (current_line[begin] != '$') begin++;
            end = begin;
            while (current_line[end] != ',') end++;
            /*saves register rt*/
            reg = copySection(current_line, begin, end);
            strcpy_ignore_spaces(current_rt, reg);
            free(reg);
            begin = end + 1;
            /*checks the place of register rm*/
            while (current_line[begin] != '$') begin++;
            end = begin;
            while (current_line[end] != ',') end++;
            /*saves register rm*/
            reg = copySection(current_line, begin, end);
            strcpy_ignore_spaces(current_rm, reg);
            free(reg);
            begin = end + 1;
            /*checks the place of imm1*/
            while (charIsDigit(current_line[begin]) != 1 && charIsLetter(current_line[begin]) != 1) begin++;
            end = begin;
            while (current_line[end] != ',') end++;
            /*saves imm1*/
            if (current_line[begin - 1] == '-') begin--;
            reg = copySection(current_line, begin, end);
            strcpy_ignore_spaces(current_imm1, reg);
            free(reg);
            begin = end + 1;
            /*checks the place of imm2*/
            while (charIsDigit(current_line[begin]) != 1 && charIsLetter(current_line[begin]) != 1) begin++;
            end = begin;
            while (charIsDigit(current_line[end]) || charIsLetter(current_line[end])) end++;
            /*saves imm2*/
            if (current_line[begin - 1] == '-') begin--;
            reg = copySection(current_line, begin, end);
            strcpy_ignore_spaces(current_imm2, reg);

            free(reg);
            /*add to memory line*/
            addInsMemory(&head_MemoryLine, current_place, current_opcode, current_rd, current_rs, current_rt, current_rm, current_imm1, current_imm2);
        }
        current_place++;
    }
    /* Close the file */
    fclose(file);
    /*return total memory AKA data memory + instructions memory*/
    total_memory->head_instructionMemory = head_MemoryLine;
    total_memory->head_dataMemory = head_dataMemory;
    return total_memory;
}

/*
from int decimal to 8 char hex
input- decimal number
output- 8 char hex
*/
char* from_decimal_to_8_char_hex(int number) {
    char* hex = (char*)malloc(sizeof(char) * 9); /* allocate 9 chars for 8 hex digits and the null terminator */
    if (hex == NULL) {
        // Handle allocation failure
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    sprintf(hex, "%08X", number);
    return hex;
}

/*
Save the Data memory in the output file named dmemin.txt"
input- total_memory
output- none (void)
*/
void sava_data_memory_into_file(totalMemory total, FILE* file)
{
    int max_place;
    max_place = -1;
    /*Make the current data memory line*/
    dataMemory* current_data_memory_line = total.head_dataMemory;
    /*find most bigger place of a data memory*/

    while (current_data_memory_line != NULL)
    {
        if (current_data_memory_line->place > max_place)
        {
            max_place = current_data_memory_line->place;
        }
        current_data_memory_line = current_data_memory_line->next;
    }
    if (max_place == -1)
    {
        return;
    }
    current_data_memory_line = total.head_dataMemory;
    /*from data memory to array*/
    int* data_memory_array = (int*)malloc(sizeof(int) * (max_place + 1));

    if (data_memory_array == NULL) {
        printf("Error: memory allocation failed\n");
        return;
    }
 
    /*initilaze the array in zero*/
    for (int i = 0; i <= max_place; i++)
    {
        data_memory_array[i] = 0;
    }
    /*Save the data memory lines in the file*/
    while (current_data_memory_line != NULL)
    {
        data_memory_array[current_data_memory_line->place] = current_data_memory_line->value;
        current_data_memory_line = current_data_memory_line->next;
    }
    /*Save the data memory array into file*/
    for (int i = 0; i <= max_place; i++)
    {
        fprintf(file, "%s\n", from_decimal_to_8_char_hex(data_memory_array[i]));
    }
}

/*
from opcode to hex
input- opcode in char*
output- opcode in hex
*/
char* from_opcode_to_hex(char* opcode) {
    if (strcmp(opcode, "add") == 0) return "00";
    else if (strcmp(opcode, "sub") == 0) return "01";
    else if (strcmp(opcode, "mac") == 0) return "02";
    else if (strcmp(opcode, "and") == 0) return "03";
    else if (strcmp(opcode, "or") == 0) return "04";
    else if (strcmp(opcode, "xor") == 0) return "05";
    else if (strcmp(opcode, "sll") == 0) return "06";
    else if (strcmp(opcode, "sra") == 0) return "07";
    else if (strcmp(opcode, "srl") == 0) return "08";
    else if (strcmp(opcode, "beq") == 0) return "09";
    else if (strcmp(opcode, "bne") == 0) return "0A";
    else if (strcmp(opcode, "blt") == 0) return "0B";
    else if (strcmp(opcode, "bgt") == 0) return "0C";
    else if (strcmp(opcode, "ble") == 0) return "0D";
    else if (strcmp(opcode, "bge") == 0) return "0E";
    else if (strcmp(opcode, "jal") == 0) return "0F";
    else if (strcmp(opcode, "lw") == 0) return "10";
    else if (strcmp(opcode, "sw") == 0) return "11";
    else if (strcmp(opcode, "reti") == 0) return "12";
    else if (strcmp(opcode, "in") == 0) return "13";
    else if (strcmp(opcode, "out") == 0) return "14";
    else /*halt*/ return "15";
}

/*
from register to hex
input- register in char
output- register in hex
*/
char from_register_to_hex(char* opcode) {
    if (strcmp(opcode, "$zero") == 0) return '0';
    else if (strcmp(opcode, "$imm1") == 0) return '1';
    else if (strcmp(opcode, "$imm2") == 0) return '2';
    else if (strcmp(opcode, "$v0") == 0) return '3';
    else if (strcmp(opcode, "$a0") == 0) return '4';
    else if (strcmp(opcode, "$a1") == 0) return '5';
    else if (strcmp(opcode, "$a2") == 0) return '6';
    else if (strcmp(opcode, "$t0") == 0) return '7';
    else if (strcmp(opcode, "$t1") == 0) return '8';
    else if (strcmp(opcode, "$t2") == 0) return '9';
    else if (strcmp(opcode, "$s0") == 0) return 'A';
    else if (strcmp(opcode, "$s1") == 0) return 'B';
    else if (strcmp(opcode, "$s2") == 0) return 'C';
    else if (strcmp(opcode, "$gp") == 0) return 'D';
    else if (strcmp(opcode, "$sp") == 0) return 'E';
    else /*ra*/ return 'F';
}

/*
from decimal to 3 hex digits
input- decimal number in char*
output- 3 hex digits
*/
char* from_decimal_to_3_hex_digits(char* number) {
    int number_int;
    if (number[0] == '-')
    {
        number_int = fromStringToNumber(number + 1); /* get the absolute value */
        number_int = (1 << 12) - number_int; /* calculate 2's complement */
    }
    else {
        number_int = fromStringToNumber(number);

    }
    while (number_int > 4095 || number_int < -4096) {
        if (number_int > 4095) {
            number_int = number_int - 4096;
        }
        else if (number_int < -4096) {
            number_int = number_int + 4096;
        }
    }
    // Allocate memory for the hexadecimal string
    char* hex = (char*)malloc(sizeof(char) * 4); // Allocate 4 chars for 3 hex digits and the null terminator

    // Check if memory allocation was successful
    if (hex == NULL) {
        // Handle memory allocation failure
        return NULL;
    }

    // Format the number as a hexadecimal string
    sprintf(hex, "%03X", number_int);
    return hex;

}

/*
Save the Instructions memory in the output file named inemin.txt"
input- total_memory
output- none (void)
*/
void save_instruction_memory_into_file(totalMemory* total, Label* head_Label, FILE* file)
{

    /*Make the current instruction memory line*/
    instructionMemory* current_instruction_memory_line = total->head_instructionMemory;
    /*Save the instruction memory lines in the file*/
    while (current_instruction_memory_line != NULL)
    {
        if (findLabel(head_Label, current_instruction_memory_line->imm1) != NULL) {
            current_instruction_memory_line->imm1 = findLabel(head_Label, current_instruction_memory_line->imm1);
        }
        if (findLabel(head_Label, current_instruction_memory_line->imm2) != NULL) {
            current_instruction_memory_line->imm2 = findLabel(head_Label, current_instruction_memory_line->imm2);
        }


        char* decimal_to_hex1 = from_decimal_to_3_hex_digits(current_instruction_memory_line->imm1);
        char* decimal_to_hex2 = from_decimal_to_3_hex_digits(current_instruction_memory_line->imm2);

        fprintf(file, "%s%c%c%c%c%s%s\n", from_opcode_to_hex(current_instruction_memory_line->opcode), from_register_to_hex(current_instruction_memory_line->rd), from_register_to_hex(current_instruction_memory_line->rs), from_register_to_hex(current_instruction_memory_line->rt), from_register_to_hex(current_instruction_memory_line->rm), from_decimal_to_3_hex_digits(current_instruction_memory_line->imm1), from_decimal_to_3_hex_digits(current_instruction_memory_line->imm2));
        current_instruction_memory_line = current_instruction_memory_line->next;
    }
}

/*
main function
*/
int main(int argc, char* argv[]) {
    FILE* mem_file;
    FILE* inst_file;
    FILE* asm_file;
    Label* head_Label;
    totalMemory* total_memory;
    /*Check if the input is valid*/
    if (argc != 4) {
        printf("Error: invalid input\n");
        exit(1);
    }
    /*opem asm file*/
    asm_file = fopen(argv[1], "r");
    if (asm_file == NULL) {
        printf("Error: invalid input file\n");
        exit(1);
    }
    /*Make the head label*/
    head_Label = first_pass(asm_file);
    fclose(asm_file);
    /*Make the total memory*/
    total_memory = second_pass(argv[1]);
    mem_file = fopen(argv[3], "w");
    if (mem_file == NULL) {
        printf("Error: invalid output file\n");
        exit(1);
    }
    /*Save the data memory into file*/
    sava_data_memory_into_file(*total_memory, mem_file);
    fclose(mem_file);
    inst_file = fopen(argv[2], "w");
    if (inst_file == NULL) {
        printf("Error: invalid output file\n");
        exit(1);
    }
    /*Save the instruction memory into file*/
    save_instruction_memory_into_file(total_memory, head_Label, inst_file);
    fclose(inst_file);
    /*free*/
    /*Destroy the labels list*/
    destroy_labels(head_Label);
    /*Destroy the data memory list*/
    destroy_data_memory(total_memory->head_dataMemory);
    /*Destroy the instruction memory list*/
    destroy_memory_lines(total_memory->head_instructionMemory);
    /*Destroy the total memory*/
    free(total_memory);
    return 0;
}
