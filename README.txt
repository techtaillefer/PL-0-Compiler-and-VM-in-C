Isabella Taillefer
COP 3402 - Montagne
-------------------

# Homework 4: [PL/0 Compiler and VM]

---------------------------------------

## Description:
The goal of this project is to take a Tiny PL/0 source program and run it through a full compile-and-execute pipeline by combining a Recursive Descent Parser with Code Generation and a stack-based Virtual Machine (P-Machine). Starting with the scanner from HW2, the compiler processes tokens, parses the program based on PL/0 grammar, builds a symbol table for constants and variables (no procedures yet), and generates low-level assembly-like instructions like JMP, JPC, INC, LOD, STO, and OPR. It immediately halts with a clear error message if a syntax rule is violated. On the execution side, the Virtual Machine takes that generated code and simulates a runtime environment using a stack (PAS) and registers like PC, BP, and SP. It interprets and executes each instruction based on a fetch-decode-execute cycle, handling control flow, arithmetic, and memory operations as defined in HW1. Altogether, this project connects lexical analysis, parsing, code generation, and execution into one cohesive system where PL/0 source code is turned into VM code and executed through a simulated ISA.

---------------------------------------

## Usage/How to run the program:

1. Make sure all necessary input and hw4compiler.c files are in same directory

Go to the terminal and write:

2. $gcc -o hw4 hw4compiler.c
3. $gcc -o vm vm.c
4. $./hw4 input.txt
5. If there is no error, everything should print as expected, the assembly code should        write to elf.txt
6. If input has an error, the terminal will only display that error
7. $gcc ./vm elf.txt
8. Everything should print as expected

Notes:

- If there are no errors an ELF file (elf.txt) will be created within same directory, containing the final machine code for the VM. It will be updated each time program is ran.

- There are 2 error inducing input files and 2 of their output files containing the resulting message from the terminal. They are depicted as test_case_error1.txt/test_cases_output1.txt and test_case_error2.txt/test_cases_output2.txt. 


---------------------------------------

## One Example:
$gcc -o hw4 hw4compiler.c
$gcc -o vm vm.c
$./hw4 input.txt

[Input file]:
var a, b, c;
begin
  a := 1;
  b := 3;
  if a > b then
    c := a + b
  else
    c := a - b
  fi;
  while a < c do
    a := a + 1;
end.


[Termial Output]:

 Source Program:
var a, b, c;
begin
  a := 1;
  b := 3;
  if a > b then
    c := a + b
  else
    c := a - b
  fi;
  while a < c do
    a := a + 1;
end.


No errors, program is syntactically correct.

Assembly Code:

 Line  OP      L  M
 0    JMP      0  13
 1    INC      0  6
 2    LIT      0  1
 3    STO      0  3
 4    LIT      0  3
 5    STO      0  4
 6    LOD      0  3
 7    LOD      0  4
 8    GTR      0  9
 9    JPC      0  55
 10    LOD      0  3
 11    LOD      0  4
 12    ADD      0  1
 13    STO      0  5
 14    JMP      0  67
 15    LOD      0  3
 16    LOD      0  4
 17    SUB      0  2
 18    STO      0  5
 19    LOD      0  3
 20    LOD      0  5
 21    LSS      0  7
 22    JPC      0  94
 23    LOD      0  3
 24    LIT      0  1
 25    ADD      0  1
 26    STO      0  3
 27    JMP      0  67
 28    SYS      0  3

Symbol Table:

 Kind |       Name |   Value |  Level |  Address | Mark
--------------------------------------------------------
    2 |          a |       0 |      0 |        3 |    1
    2 |          b |       0 |      0 |        4 |    1
    2 |          c |       0 |      0 |        5 |    1


[elf.txt Output]:
7 0 13
6 0 6
1 0 1
4 0 3
1 0 3
4 0 4
3 0 3
3 0 4
2 0 9
8 0 55
3 0 3
3 0 4
2 0 1
4 0 5
7 0 67
3 0 3
3 0 4
2 0 2
4 0 5
3 0 3
3 0 5
2 0 7
8 0 94
3 0 3
1 0 1
2 0 1
4 0 3
7 0 67
9 0 3

[Terminal Output - vm.c code]:
$./vm elf.txt

 PC   BP   SP   Stack
Initial values: 10   499  500  

JMP  0   13     13   499  500  
INC  0   6      16   499  494  0 0 0 0 0 0 
LIT  0   1      19   499  493  0 0 0 0 0 0 1 
STO  0   3      22   499  494  0 0 0 1 0 0 
LIT  0   3      25   499  493  0 0 0 1 0 0 3 
STO  0   4      28   499  494  0 0 0 1 3 0 
LOD  0   3      31   499  493  0 0 0 1 3 0 1 
LOD  0   4      34   499  492  0 0 0 1 3 0 1 3 
GTR  0   9      37   499  493  0 0 0 1 3 0 0 
JPC  0   55     55   499  494  0 0 0 1 3 0 
LOD  0   3      58   499  493  0 0 0 1 3 0 1 
LOD  0   4      61   499  492  0 0 0 1 3 0 1 3 
SUB  0   2      64   499  493  0 0 0 1 3 0 -2 
STO  0   5      67   499  494  0 0 0 1 3 -2 
LOD  0   3      70   499  493  0 0 0 1 3 -2 1 
LOD  0   5      73   499  492  0 0 0 1 3 -2 1 -2 
LSS  0   7      76   499  493  0 0 0 1 3 -2 0 
JPC  0   94     94   499  494  0 0 0 1 3 -2 
SYS  0   3      97   499  494  0 0 0 1 3 -2 

## Alternate Example in case of an error:
$gcc -o hw4 hw4compiler.c
$./a.out input.txt

[Input file]:
var f, n;

begin
  n := 3;
  f := 1;
  call n 
end.


[Terminal Output]:
Error: call of a non-procedure is not allowed

[elf.txt output]: 




-- There isn't an output for elf.txt when there's an error -- 
---------------------------------------

## No team.

---------------------------------------

## Contact Information:
Isabella Taillefer, is152290@ucf.edu

