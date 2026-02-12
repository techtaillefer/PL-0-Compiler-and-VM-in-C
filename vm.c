// Isabella Taillefer
// P-Machine
// COP 3402 - Montagne

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PAS_SIZE 500

// Instruction struct //
typedef struct instruction {
    int op; // Operation Code
    int l;  // Lexicographical level
    int m;  // Address, value, or operation
} instruction;

// CPU Registers //
int sp, bp, pc;
instruction ir; // Instruction Register
int pas[MAX_PAS_SIZE] = {0}; // Process Address Space

// Instruction name arrays.
char *opNames[] = {"", "LIT", "OPR", "LOD", "STO", "CAL", "INC", "JMP", "JPC", "SYS"};
char *oprNames[] = {"RTN", "ADD", "SUB", "MUL", "DIV", "EQL", "NEQ", "LSS", "LEQ", "GTR", "GEQ", "MOD"}; // Added "MOD" at index 11

// Finds the base L levels down.
int base(int BP, int L) { 
    int arb = BP;
    while (L > 0) {
        arb = pas[arb];
        L--;
    }
    return arb;
}

// Prints the current execution state.
void printExecution(int pas[], int BP, int SP, int PC) {
    static int arBases[100];
    static int arCount = 0;

    if (ir.op == 5) { // CAL
        arBases[arCount++] = BP;
    } else if (ir.op == 2 && ir.m == 0) { // RTN
        if (arCount > 0) arCount--;
    }
    if (strcmp(opNames[ir.op], "OPR") == 0) { 
        printf("%-5s%-4d%-7d%-5d%-5d%-5d", oprNames[ir.m], ir.l, ir.m, pc, bp, sp);
    } else {
        printf("%-5s%-4d%-7d%-5d%-5d%-5d", opNames[ir.op], ir.l, ir.m, pc, bp, sp);
    }
    for (int i = MAX_PAS_SIZE - 1; i >= SP; i--) { 
        int isARBase = 0;
        for (int j = 0; j < arCount; j++) {
            if (i == arBases[j]) {
                isARBase = 1;
                break;
            }
        }
        if (isARBase) printf("| ");
        printf("%d ", pas[i]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]); 
        return EXIT_FAILURE;
    }

    FILE *inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        perror("Opening file");
        return EXIT_FAILURE;
    }

    sp = 500;
    bp = sp - 1;
    pc = 10;

    int idx = 10;
    while (fscanf(inputFile, "%d %d %d", &pas[idx], &pas[idx + 1], &pas[idx + 2]) == 3) {
        idx += 3; 
    }
    fclose(inputFile);

    printf("                PC   BP   SP   Stack\n");
    printf("%-16s%-5d%-5d%-5d\n", "Initial values:", pc, bp, sp);
    printf("\n");

    int eop = 1;
    while (eop) { 
        ir.op = pas[pc];
        ir.l = pas[pc + 1];
        ir.m = pas[pc + 2];
        pc += 3;

        switch (ir.op) {
            case 1: // LIT
                sp--;
                pas[sp] = ir.m;
                break;
            case 2: // OPR
                switch (ir.m) {
                    case 0: // RTN
                        sp = bp + 1;
                        bp = pas[sp - 2]; 
                        pc = pas[sp - 3]; 
                        break;
                    case 1: // ADD
                        pas[sp + 1] = pas[sp + 1] + pas[sp];
                        sp++;
                        break;
                    case 2: // SUB
                        pas[sp + 1] = pas[sp + 1] - pas[sp];
                        sp++;
                        break;
                    case 3: // MUL
                        pas[sp + 1] = pas[sp + 1] * pas[sp];
                        sp++;
                        break;
                    case 4: // DIV
                        pas[sp + 1] = pas[sp + 1] / pas[sp];
                        sp++;
                        break;
                    case 5: // EQL
                        pas[sp + 1] = (pas[sp + 1] == pas[sp]);
                        sp++;
                        break;
                    case 6: // NEQ
                        pas[sp + 1] = (pas[sp + 1] != pas[sp]);
                        sp++;
                        break;
                    case 7: // LSS
                        pas[sp + 1] = (pas[sp + 1] < pas[sp]);
                        sp++;
                        break;
                    case 8: // LEQ
                        pas[sp + 1] = (pas[sp + 1] <= pas[sp]);
                        sp++;
                        break;
                    case 9: // GTR
                        pas[sp + 1] = (pas[sp + 1] > pas[sp]);
                        sp++;
                        break;
                    case 10: // GEQ
                        pas[sp + 1] = (pas[sp + 1] >= pas[sp]);
                        sp++;
                        break;
                    case 11: // MOD
                        pas[sp + 1] = pas[sp + 1] % pas[sp];
                        sp++;
                        break;
                }
                break;
            case 3: // LOD
                sp--;
                pas[sp] = pas[base(bp, ir.l) - ir.m];
                break;
            case 4: // STO
                pas[base(bp, ir.l) - ir.m] = pas[sp];
                sp++;
                break;
            case 5: // CAL
                pas[sp - 1] = base(bp, ir.l);
                pas[sp - 2] = bp;
                pas[sp - 3] = pc;
                bp = sp - 1;
                pc = ir.m;
                break;
            case 6: // INC
                sp -= ir.m;
                break;
            case 7: // JMP
                pc = ir.m;
                break;
            case 8: // JPC
                if (pas[sp] == 0)
                    pc = ir.m;
                sp++;
                break;
            case 9: // SYS
                switch (ir.m) {
                    case 1:
                        printf("Output result is: %d\n", pas[sp]);
                        sp++;
                        break;
                    case 2:
                        sp--;
                        printf("Please Enter an Integer: ");
                        scanf("%d", &pas[sp]);
                        break;
                    case 3:
                        eop = 0;
                        break;
                }
                break;
        }
        printExecution(pas, bp, sp, pc);
    }
    return 0;
}
