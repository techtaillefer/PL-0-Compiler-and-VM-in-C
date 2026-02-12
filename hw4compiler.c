// Isabella Taillefer
// PL/0 Compiler -  Recursive Descent Parser and Code Generator
// COP 3402 - Montagne

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constraints
#define MAX_IDENT_LENGTH 12 // store 11 chars + 1 for \0
#define MAX_NUMBER_LENGTH 5
#define MAX_LEXEMES 9999          // Maximum number of lexemes/tokens
#define INPUT_BUFFER_SIZE 9999    // Maximum size for input file buffer
#define TMP_SIZE 9999             // Maximum size for temporary token buffer
#define MAX_LEXEME_NAME_LENGTH 50 // Maximum size for lexeme name

#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_VM_CODE_SIZE 1000

// Updated Opcode definitions for code generation
#define LIT 1 // Load literal
#define OPR 2 // Operation
#define LOD 3 // Load variable
#define STO 4 // Store
#define CAL 5 // Call procedure
#define INC 6 // Increment (allocate space)
#define JMP 7 // Jump
#define JPC 8 // Jump if false
#define SYS 9 // System write,read,halt

// Global variable for tracking lexical level (main level is 0)
int currentLevel = 0;

// Token type declarations
typedef enum {
  modsym = 1,
  identsym,
  numbersym,
  plussym,
  minussym,
  multsym,
  slashsym,
  fisym,
  eqlsym,
  neqsym,
  lessym,
  leqsym,
  gtrsym,
  geqsym,
  lparentsym,
  rparentsym,
  commasym,
  semicolonsym,
  periodsym,
  becomessym,
  beginsym,
  endsym,
  ifsym,
  thensym,
  whilesym,
  dosym,
  callsym, 
  constsym,
  varsym,
  procsym, 
  writesym,
  readsym,
  elsesym
} token_type;

// Symbol Table 
typedef struct {
  int kind; // 1 for const, 2 for var, 3 for proc
  char name[MAX_IDENT_LENGTH];
  int val;
  int level;
  int addr;
  int mark;
} symbol;

symbol symbol_table[MAX_SYMBOL_TABLE_SIZE]; // 500 max
int symbol_count = 0; // count # of  current symbols in table

// Lexeme Struct (reps token)
typedef struct lexeme {
  char name[MAX_IDENT_LENGTH];
  int value;
  int type;
} lexeme;

// Global variables for scanning
// Used by LA to store tokens and track position
lexeme *list = NULL; // holds the list of tokens
int lex_index = 0;   
int input_index = 0; 
char tmp[TMP_SIZE];  
int tmp_index = 0;   

// Global variables for parsing
// Used to help parser know what's the next token to process
int token_index = 0;
int current_token;

// VM Code Array - opcode, level, address
typedef struct {
  int op;
  int l;
  int m;
} instruction;

// Global variables for code generation
instruction vm_code[MAX_VM_CODE_SIZE];
int vm_index = 0;

// counter for number of vars declared (used to handle duplicates later)
int varCount = 0; 


// Function Prototypes //
// Parser/Code Gen FP
void program();
void block();
void constDeclaration();
int varDeclaration();
void procedureDeclaration();  
void statement();
void expression();
void term();
void factor();
void condition();
int rel_op();
void handleError(int errorCode);
void handleUndeclIdent(const char *ident);
void getNextToken();
void insertSymbol(int kind, char *name, int val);
void emit(int op, int l, int m);
const char *oprMnemonic(int opr);
void printVMCode();
void printSymbolTable();
int symbolTableLookup(const char *name);
int symbolExistsInCurrentLevel(const char *name, int kind);

// Scanner FP
lexeme *LA(char *input);
int is_symbol(char input_char);
int commentChecker(char *input, int first_loop);
void lineCommentChecker(char *input);
void invisibleCharChecker(char *input);
void wordChecker(char *input);
void numberChecker(char *input);
void symbolChecker(char *input);
void errorChecker(int error_code, const char *errorLex);
void printtokens();

// (!!!) Clarification note for OPR codes in HW1 VM: //
//ADD - 1, SUB - 2, MUL - 3, DIV - 4, EQL - 5, NEQ - 6, LSS - 7, LEQ - 8, GTR - 9, GEQ - 10, 
//and now MOD - 11
// I adjusted JPC/JMP to match our HW1 VM code (in statement function for if and while)
//////////////////////////////////////////////////////////////////////////////////////

// --- Parsing and Code Generation --- //

// Function to map opcode to its mnemonic string for printing
const char *opToString(int op) {
  switch (op) {
  case LIT:
    return "LIT";
  case OPR:
    return "OPR";
  case LOD:
    return "LOD";
  case STO:
    return "STO";
  case CAL:
    return "CAL";
  case INC:
    return "INC";
  case JMP:
    return "JMP";
  case JPC:
    return "JPC";
  case SYS:
    return "SYS";
  default:
    return "UNKNOWN";
  }
}

// Updated/combined some errors for Hw4
// Error Handling: 17 parser errors -- the other 3 lexical errors are handled in
// the scanner code section!
void handleError(int errorCode) {
  const char
      *errorMessages[] =
          {
              "program must end with a period",                 // 0
              "const, var, procedure, and call must be followed by an identifier",     // 1
              "symbol name has already been declared",          // 2
              "constants must be assigned with =",              // 3
              "constants must be assigned an integer value",    // 4
              "Semicolon or comma missing",                     // 5
              "undeclared identifier", // 6 - handled in the following function
              "only variable values may be altered",            // 7
              "assignment statements must use :=",              // 8
              "Semicolon or end expected",                      // 9
              "if must be followed by then",                    // 10
              "while must be followed by do",                   // 11
              "condition must contain a comparison operator",   // 12
              "right parenthesis must follow left parenthesis", // 13
              "arithmetic equations must contain operands, parentheses, numbers, or symbols",      // 14
              "if must be followed by fi",                       // 15 
              "if statement must include an else clause",        // 16
              "call of a non-procedure is not allowed"           // 17 (constant/variable is meaningless)


          };

  printf("Error: %s\n", errorMessages[errorCode]);
  exit(1);
}

// (!) # 6 Error code to specify undeclared identifier function based on HW3 requiremnts:
// Essentially prints an error when an indentier is used without being declared
void handleUndeclIdent(const char *id) {
  printf("Error: undeclared identifier %s\n", id);
  exit(1);
}

// Gets next token (updates current_token to next token from token list)
void getNextToken() {
  if (token_index < lex_index) {
    current_token = list[token_index++].type;
  }
}

// Insert symbol into Symbol Table
// Insert symbol into Symbol Table, now storing the current lexical level.
void insertSymbol(int kind, char *name, int val) {
    if (symbol_count >= MAX_SYMBOL_TABLE_SIZE) {
        printf("Error: Symbol table overflow\n");
        exit(1);
    }
    if (symbolExistsInCurrentLevel(name, kind)) {
        handleError(2);
    }

    strcpy(symbol_table[symbol_count].name, name);
    symbol_table[symbol_count].kind = kind;
    symbol_table[symbol_count].val = val;
    symbol_table[symbol_count].level = currentLevel;
    symbol_table[symbol_count].mark = 0;

    if (kind == 2) { // For variables
        symbol_table[symbol_count].addr = varCount + 3;
        symbol_table[symbol_count].mark = 1;
        varCount++;
    } else if (kind == 1) {
        symbol_table[symbol_count].addr = 0;
        symbol_table[symbol_count].mark = 1;
    } else if (kind == 3) {
        // For procedures, we still need to check for duplicates in current level
        // if (symbolExistsInCurrentLevel(name,kind)) {
        //     handleError(2);
        // }
        symbol_table[symbol_count].addr = 0;
        symbol_table[symbol_count].mark = 1;
    }
    symbol_count++;
}


// Generate VM Code (adds new instruction to vm code array)
void emit(int op, int l, int m) {
  if (vm_index >= MAX_VM_CODE_SIZE) {
    printf("Error: VM code overflow\n");
    exit(1);
  }
  vm_code[vm_index].op = op;
  vm_code[vm_index].l = l;
  vm_code[vm_index].m = m;
  vm_index++;
}


// Function for OPR mnemonic codes
const char *oprMnemonic(int m) {
    switch (m) {
        case 0:  return "RTN";
        case 1:  return "ADD";
        case 2:  return "SUB";
        case 3:  return "MULT";
        case 4:  return "DIV";
        case 5:  return "EQL";
        case 6:  return "NEQ";
        case 7:  return "LSS";
        case 8:  return "LEQ";
        case 9:  return "GTR";
        case 10: return "GEQ";
        case 11: return "MOD";
        default: return "OPR";
    }
}

// Prints the generated Code
void printVMCode() {
  printf("No errors, program is syntactically correct.\n\n");
  printf("Assembly Code:\n\n");
  printf(" Line  OP      L  M\n");
  for (int i = 0; i < vm_index; i++) {
      if (vm_code[i].op == OPR) {
          // For OPR instructions, prints the corresponding code to vm_code[i].m.
          printf(" %d    %-7s  %d  %d\n", i, oprMnemonic(vm_code[i].m),
                   vm_code[i].l, vm_code[i].m);
      } else {
          printf(" %d    %-7s  %d  %d\n", i, opToString(vm_code[i].op),
                   vm_code[i].l, vm_code[i].m);
        }
    }
}

// Print the Symbol Table
void printSymbolTable() {
  printf("\nSymbol Table:\n\n");
  printf("%5s | %10s | %7s | %6s | %8s | %4s\n", "Kind", "Name", "Value", "Level", "Address", "Mark");
  printf("--------------------------------------------------------\n");
  for (int i = 0; i < symbol_count; i++) {
    printf("%5d | %10s | %7d | %6d | %8d | %4d\n", symbol_table[i].kind,
           symbol_table[i].name, symbol_table[i].val, symbol_table[i].level,
           symbol_table[i].addr, symbol_table[i].mark);
  }
}


// Lookup a symbol in the symbol table - reverse linear search from end to start
int symbolTableLookup(const char *name) {
    // First try to find in current level
    for (int i = symbol_count - 1; i >= 0; i--) {
        if (strcmp(symbol_table[i].name, name) == 0 && 
            symbol_table[i].level == currentLevel && 
            symbol_table[i].mark == 1) {
            return i;
        }
    }

    // if  not found in current level, look in outer levels
    for (int i = symbol_count - 1; i >= 0; i--) {
        if (strcmp(symbol_table[i].name, name) == 0 && 
            symbol_table[i].level < currentLevel && 
            symbol_table[i].mark == 1) {
            return i;
        }
    }
    return -1;
}

// Add function to check if symbol exists in current level only
int symbolExistsInCurrentLevel(const char *name, int kind) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0 && 
            symbol_table[i].kind == kind && 
            symbol_table[i].mark == 0 && 
            ((kind == 1) || // For constants, only check name and kind (level always 0)
             (kind != 1 && symbol_table[i].level == currentLevel))) { // For vars and procs, check level too
            return 1;
        }
    }
    return 0;
}

// Parse Program
void program() {
  block();
  if (current_token != periodsym) {
    handleError(0);
  }
  emit(SYS, 0, 3); // system halt
}

// Parse Block
void block() {
    varCount = 0; // reset varCount for each block
    int skipJumpIndex = vm_index;
    emit(JMP, 0, 0);

    constDeclaration();
    int numVars = varDeclaration();

    while (current_token == procsym) {
        procedureDeclaration();
    }

    vm_code[skipJumpIndex].m = 10 + (vm_index * 3);
    emit(INC, 0, numVars + 3); // + 3 is for RA, SL, DL
    statement();

    for (int i = 0; i < symbol_count; i++) {
        if (symbol_table[i].level == currentLevel) {
            symbol_table[i].mark = 1;
        }
    }
}

// Process constant declarations (and checks if already declared)
void constDeclaration() {
    if (current_token == constsym) {
        getNextToken();
        if (current_token != identsym) {
            handleError(1);
        }
        char constName[MAX_IDENT_LENGTH];
        strcpy(constName, list[token_index - 1].name);
        getNextToken();
        if (current_token != eqlsym) {
            handleError(3);
        }
        getNextToken();
        if (current_token != numbersym) {
            handleError(4);
        }
        // Get const value, check if already declared, and insert into symbol table 
        int newVal = list[token_index - 1].value;
        int idx = symbolTableLookup(constName);
        if (idx != -1 && symbol_table[idx].kind == 1 && symbol_table[idx].val == newVal) {
            handleError(2);
        }
        insertSymbol(1, constName, newVal);
        getNextToken();
        while (current_token == commasym) {
            getNextToken();
            if (current_token != identsym) {
                handleError(1);
            }
            strcpy(constName, list[token_index - 1].name);
            getNextToken();
            if (current_token != eqlsym) {
                handleError(3);
            }
            getNextToken();
            if (current_token != numbersym) {
                handleError(4);
            }
            newVal = list[token_index - 1].value;
            idx = symbolTableLookup(constName);
            if (idx != -1 && symbol_table[idx].kind == 1 && symbol_table[idx].val == newVal) {
                handleError(2);
            }
            insertSymbol(1, constName, newVal);
            getNextToken();
        }
        if (current_token != semicolonsym) {
            handleError(5);
        }
        getNextToken();
    }
}

// Process variable declarations - returns count of variables declared.
int varDeclaration() {
    int numVars = 0;
    if (current_token == varsym) {
        getNextToken();
        if (current_token != identsym) {
            handleError(1);
        }
        char varName[MAX_IDENT_LENGTH];
        strcpy(varName, list[token_index - 1].name);
        if (symbolExistsInCurrentLevel(varName, 2)) {
            handleError(2);
        }
        insertSymbol(2, varName, 0);
        numVars++;
        getNextToken();
        while (current_token == commasym) {
            getNextToken();
            if (current_token != identsym) {
                handleError(1);
            }
            strcpy(varName, list[token_index - 1].name);
            if (symbolExistsInCurrentLevel(varName, 2)) {
                handleError(2);
            }
            insertSymbol(2, varName, 0);
            numVars++;
            getNextToken();
        }
        if (current_token != semicolonsym) {
            handleError(5);
        }
        getNextToken();
    }
    return numVars;
}

// Process procedure declarations - proc, ident, block, semicolon
void procedureDeclaration() {
    if (current_token != procsym) {
        handleError(1); 
    }
    getNextToken(); 
    if (current_token != identsym) {
        handleError(1); // procedure must be followed by an identifier
    }
    char procName[MAX_IDENT_LENGTH];
    strcpy(procName, list[token_index - 1].name);
    getNextToken(); 
    if (current_token != semicolonsym) {
        handleError(5);
    }
    getNextToken(); // consume semicolon

    // Insert procedure symbol with kind = 3
    insertSymbol(3, procName, 0);
    // Set the procedure's code address to the current vm_index
    symbol_table[symbol_count - 1].addr = 10 + (vm_index * 3);

    // Increase lex level for procedure block
    currentLevel++;
    block();

    emit(OPR, 0, 0);

    if (current_token != semicolonsym) {
        handleError(5);
    }
    getNextToken(); // consume semicolon
    currentLevel--;

}

// Parse statement - begin/end, if-else-fi, while, read, write, call.
void statement() {
  if (current_token == identsym) {
      int symIndex = symbolTableLookup(list[token_index - 1].name);
      if (symIndex == -1) {
          handleUndeclIdent(list[token_index - 1].name);
      }
      if (symbol_table[symIndex].kind != 2) {
          handleError(7);
      }
      getNextToken();
      if (current_token != becomessym)
          handleError(8);
      getNextToken();
      expression();
      // Compute the relative level before storing
      int relLevel = currentLevel - symbol_table[symIndex].level;
      emit(STO, relLevel, symbol_table[symIndex].addr);
  }
 else if (current_token == beginsym) {
    do {
      getNextToken();
      statement();
    } while (current_token == semicolonsym);
    if (current_token != endsym)
      handleError(9);
    getNextToken();
  } else if (current_token == ifsym) {
    getNextToken(); 
    condition();
    int jpcIdx = vm_index;
    emit(JPC, 0, 0); // placeholder for jump to else branch
    if (current_token != thensym)
      handleError(10);
    getNextToken(); 
    statement();  
    int jmpIdx = vm_index;
    emit(JMP, 0, 0); // placeholder to jump over else branch
    vm_code[jpcIdx].m = 10 + (vm_index * 3); // set jump target for false condition to start of else branch
    if (current_token != elsesym)
      handleError(16); // missing else clause
    getNextToken(); 
    statement();  
    if (current_token != fisym)
      handleError(15);
    getNextToken(); 
    vm_code[jmpIdx].m = 10 + (vm_index * 3); // update jump target to after else branch
  } else if (current_token == whilesym) {
    getNextToken();
    int loopStart = vm_index;
    condition();
    int jpcIndex = vm_index;
    emit(JPC, 0, 0); // placeholder for JPC
    if (current_token != dosym)
      handleError(11);
    getNextToken();
    statement();
    emit(JMP, 0, 10 + (loopStart * 3));
    vm_code[jpcIndex].m = 10 + (vm_index * 3);
  } else if (current_token == readsym) {
    getNextToken();
    if (current_token != identsym)
      handleError(1);
    int symIndex = symbolTableLookup(list[token_index - 1].name);
    if (symIndex == -1)
      handleUndeclIdent(list[token_index - 1].name);
    if (symbol_table[symIndex].kind != 2)
      handleError(7);
    getNextToken();
    emit(SYS, 0, 2); // read
    int relLevel = currentLevel - symbol_table[symIndex].level;
    emit(STO, relLevel, symbol_table[symIndex].addr);
  } else if (current_token == writesym) {
    getNextToken();
    expression();
    emit(SYS, 0, 1); // write
  } else if (current_token == callsym) { 
    getNextToken(); 
    if (current_token != identsym)
      handleError(1);
    int symIndex = symbolTableLookup(list[token_index - 1].name);
    if (symIndex == -1)
      handleUndeclIdent(list[token_index - 1].name);
    if (symbol_table[symIndex].kind != 3)
      handleError(17); // call of a non-procedure is not allowed
    getNextToken(); 
    // Calculate the correct level for the CAL instruction
    int relLevel = currentLevel - symbol_table[symIndex].level;
    emit(CAL, relLevel, symbol_table[symIndex].addr);
  }
}

// Parse Expression – handles addition/subtraction.
void expression() {
  term();
  while (current_token == plussym || current_token == minussym) {
    int op = current_token;
    getNextToken();
    term();
    if (op == plussym) {
      emit(OPR, 0, 1); // ADD
    } else if (op == minussym) {
      emit(OPR, 0, 2); // SUB
    }
  }
}

// Parse Term – handles multiplication, division, and modulo.
void term() {
  factor();
  while (current_token == multsym || current_token == slashsym ||
         current_token == modsym) {
    int op = current_token;
    getNextToken();
    factor();
    if (op == multsym) {
      emit(OPR, 0, 3); // MULT
    } else if (op == slashsym) {
      emit(OPR, 0, 4); // DIV
    } else if (op == modsym) {
      emit(OPR, 0, 11); // MOD -> OPR 0 11
    }
  }
}

// Parse Factor – handles identifiers, numbers, and parentheses.
void factor() {
  if(current_token == identsym) {
      int symIndex = symbolTableLookup(list[token_index - 1].name);
      if (symIndex == -1) {
          handleUndeclIdent(list[token_index - 1].name);
      }
      if (symbol_table[symIndex].kind == 1) {
          emit(LIT, 0, symbol_table[symIndex].val);
      } else if (symbol_table[symIndex].kind == 2) {
          // Compute relative level:
          int relLevel = currentLevel - symbol_table[symIndex].level;
          emit(LOD, relLevel, symbol_table[symIndex].addr);
      } else {
          handleUndeclIdent(list[token_index - 1].name);
      }
      getNextToken();
  } else if (current_token == numbersym) {
    int val = list[token_index - 1].value;
    emit(LIT, 0, val);
    getNextToken();
  } else if (current_token == lparentsym) {
    getNextToken();
    expression();
    if (current_token != rparentsym)
      handleError(13);
    getNextToken();
  } else {
    handleError(14);
  }
}

// Parse relational condition.
void condition() {
  expression();
  if (current_token == eqlsym || current_token == neqsym ||
      current_token == lessym || current_token == leqsym ||
      current_token == gtrsym || current_token == geqsym) {

    int op_code = rel_op();
    getNextToken();
    expression();
    emit(OPR, 0, op_code);
  } else {
    handleError(12);
  }
}

// Map relational operator tokens to OPR codes.
int rel_op() {
  if (current_token == eqlsym) {
    return 5; // EQL
  } else if (current_token == neqsym) {
    return 6; // NEQ
  } else if (current_token == lessym) {
    return 7; // LSS
  } else if (current_token == leqsym) {
    return 8; // LEQ
  } else if (current_token == gtrsym) {
    return 9; // GTR
  } else if (current_token == geqsym) {
    return 10; // GEQ
  } else {
    handleError(12);
    return -1;
  }
}

// Main function: reads input, invokes lexical analysis and parsing,
// prints VM assembly and symbol table, and writes the elf.txt file.
int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Error: please include the file name\n");
    return EXIT_FAILURE;
  }
  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    printf("Error: could not open file %s\n", argv[1]);
    return EXIT_FAILURE;
  }
  char source_code[INPUT_BUFFER_SIZE];
  int i = 0;
  char c;

  // Read file character by character
  while ((c = fgetc(fp)) != EOF && i < INPUT_BUFFER_SIZE - 1) {
    source_code[i++] = c;
  }
  source_code[i] = '\0';
  fclose(fp);

  // Initialize all symbol table entries to 0
  for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
    symbol_table[i].kind = 0;
    symbol_table[i].val = 0;
    symbol_table[i].level = 0;
    symbol_table[i].addr = 0;
    symbol_table[i].mark = 0;
    memset(symbol_table[i].name, 0, MAX_IDENT_LENGTH);
  }

  // Perform lexical analysis to build token list.
  list = LA(source_code);
  printtokens(); // Check for lexical errors
  getNextToken();
  program();

  // If we get here, there were no errors, so print source (according to hw4) and generated code
  printf("Source Program:\n%s\n\n", source_code);

  // Print generated assembly code and symbol table.
  printVMCode();
  printSymbolTable();

  // Create the elf.txt file containing the VM code.
  FILE *elfFile = fopen("elf.txt", "w");
  if (!elfFile) {
    printf("Error: could not open elf.txt for writing\n");
    exit(1);
  }
  for (int i = 0; i < vm_index; i++) {
    fprintf(elfFile, "%d %d %d\n", vm_code[i].op, vm_code[i].l, vm_code[i].m);
  }
  fclose(elfFile);

  return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////

// --- Lexical Analyzer (LA) and Supporting Functions --- //


/*Symbol checking function*/
// Check if the character is one of the recognized symbol characters
int is_symbol(char input_char) {
  switch (input_char) {
  case '=':
  case '<':
  case '>':
  case '*':
  case '/':
  case '+':
  case '-':
  case '(':
  case ')':
  case ',':
  case '.':
  case ';':
  case ':':
    return 1;
  default:
    return 0;
  }
}

/*Comment checking function*/
int commentChecker(char *input, int first_loop) {
  if (first_loop == 1)
    input_index += 2; // skip initial "/*"
  if (input[input_index] == '*' && input[input_index + 1] == '/') {
    input_index += 2;
    return 0;
  } else {
    if (input[input_index] == '\0')
      return 0; // simply return 0 if end of input is reached 
    input_index++;
    return commentChecker(input, 0);
  }
}

/*Line comment checking function*/
// Process a single-line comment, Keeps going til newline or end of input
void lineCommentChecker(char *input) {
  while (input[input_index] != '\0' && input[input_index] != '\n')
    input_index++;
}

/*Invisible character checking function*/
// Skip over whitespace & control characters
void invisibleCharChecker(char *input) {
  while (input[input_index] != '\0' &&
         (iscntrl(input[input_index]) || isspace(input[input_index])))
    input_index++;
}

// Updated wordChecker: now recognizes "procedure", "call", and "else" as reserved words.
void wordChecker(char *input) {
  while (input[input_index] != '\0' && isalnum(input[input_index]) &&
         tmp_index < TMP_SIZE - 1) {
    tmp[tmp_index++] = input[input_index++];
  }
  tmp[tmp_index] = '\0';

  if (strlen(tmp) > MAX_IDENT_LENGTH) {
    errorChecker(3, tmp);
    tmp_index = 0;
    return;
  }

  if (lex_index >= MAX_LEXEMES) {
    printf("Error: lexeme list overflow\n");
    exit(1);
  }

  if (strcmp(tmp, "const") == 0)
    list[lex_index].type = constsym;
  else if (strcmp(tmp, "var") == 0)
    list[lex_index].type = varsym;
  else if (strcmp(tmp, "procedure") == 0)
    list[lex_index].type = procsym;
  else if (strcmp(tmp, "call") == 0)
    list[lex_index].type = callsym;
  else if (strcmp(tmp, "if") == 0)
    list[lex_index].type = ifsym;
  else if (strcmp(tmp, "then") == 0)
    list[lex_index].type = thensym;
  else if (strcmp(tmp, "else") == 0)
    list[lex_index].type = elsesym;
  else if (strcmp(tmp, "while") == 0)
    list[lex_index].type = whilesym;
  else if (strcmp(tmp, "do") == 0)
    list[lex_index].type = dosym;
  else if (strcmp(tmp, "begin") == 0)
    list[lex_index].type = beginsym;
  else if (strcmp(tmp, "end") == 0)
    list[lex_index].type = endsym;
  else if (strcmp(tmp, "read") == 0)
    list[lex_index].type = readsym;
  else if (strcmp(tmp, "write") == 0)
    list[lex_index].type = writesym;
  else if (strcmp(tmp, "fi") == 0)
    list[lex_index].type = fisym;
  else if (strcmp(tmp, "mod") == 0)
    list[lex_index].type = modsym;
  else
    list[lex_index].type = identsym;

  strncpy(list[lex_index].name, tmp, sizeof(list[lex_index].name) - 1);
  list[lex_index].name[sizeof(list[lex_index].name) - 1] = '\0';
  lex_index++;
  tmp_index = 0;
}
/*Number checking function*/
// Process numbers via reading the digit
void numberChecker(char *input) {
  while (input[input_index] != '\0' && isdigit(input[input_index]) &&
         tmp_index < TMP_SIZE - 1) {
    tmp[tmp_index++] = input[input_index++];
  }
  tmp[tmp_index] = '\0';

  if (strlen(tmp) > MAX_NUMBER_LENGTH) {
    errorChecker(2, tmp); // Error code 2:  Number too long
    tmp_index = 0;
    return;
  }

  // If letter is right after number, ERROR
  if (isalpha(input[input_index])) {
    errorChecker(1, tmp); // Error code 1: Invalid symbol
    tmp_index = 0;
    return;
  }
  if (lex_index >= MAX_LEXEMES) {
    printf("Error: lexeme list overflow\n");
    exit(1);
  }
  list[lex_index].type = numbersym;
  list[lex_index].value = atoi(tmp);
  strncpy(list[lex_index].name, tmp, sizeof(list[lex_index].name) - 1);
  list[lex_index].name[sizeof(list[lex_index].name) - 1] = '\0';
  lex_index++;
  tmp_index = 0;
}

/*Symbol Checking Function */
// Processes symbols =, <, etc
void symbolChecker(char *input) {
  int symbol_type = -1;
  char errLex[3] = {0}; // buffer for the symbol lexeme
  switch (input[input_index]) {
  case '=':
    symbol_type = eqlsym;
    input_index++;
    strncpy(errLex, "=", sizeof(errLex) - 1);
    break;
  case '<':
    if (input[input_index + 1] == '>') {
      symbol_type = neqsym;
      input_index += 2;
      strncpy(errLex, "<>", sizeof(errLex) - 1);
    } else if (input[input_index + 1] == '=') {
      symbol_type = leqsym;
      input_index += 2;
      strncpy(errLex, "<=", sizeof(errLex) - 1);
    } else {
      symbol_type = lessym;
      input_index++;
      snprintf(errLex, sizeof(errLex), "%c", '<');
    }
    break;
  case '>':
    if (input[input_index + 1] == '=') {
      symbol_type = geqsym;
      input_index += 2;
      strncpy(errLex, ">=", sizeof(errLex) - 1);
    } else {
      symbol_type = gtrsym;
      input_index++;
      snprintf(errLex, sizeof(errLex), "%c", '>');
    }
    break;
  case ':':
    if (input[input_index + 1] == '=') {
      symbol_type = becomessym;
      input_index += 2;
      strncpy(errLex, ":=", sizeof(errLex) - 1);
    } else {
      symbol_type = -1;
      errLex[0] = input[input_index];
      errLex[1] = '\0';
      input_index++;
      errorChecker(1, errLex);
      return;
    }
    break;
  case '/':
    symbol_type = slashsym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", '/');
    break;
  case '*':
    symbol_type = multsym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", '*');
    break;
  case '+':
    symbol_type = plussym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", '+');
    break;
  case '-':
    symbol_type = minussym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", '-');
    break;
  case '(':
    symbol_type = lparentsym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", '(');
    break;
  case ')':
    symbol_type = rparentsym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", ')');
    break;
  case ',':
    symbol_type = commasym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", ',');
    break;
  case '.':
    symbol_type = periodsym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", '.');
    break;
  case ';':
    symbol_type = semicolonsym;
    input_index++;
    snprintf(errLex, sizeof(errLex), "%c", ';');
    break;
  default:
    errLex[0] = input[input_index];
    errLex[1] = '\0';
    input_index++;
    errorChecker(1, errLex);
    return;
  }
  if (lex_index >= MAX_LEXEMES) {
    printf("Error: lexeme list overflow\n");
    exit(1);
  }
  list[lex_index].type = symbol_type;
  strncpy(list[lex_index].name, errLex, sizeof(list[lex_index].name) - 1);
  list[lex_index].name[sizeof(list[lex_index].name) - 1] = '\0';
  lex_index++;
}

/*Error checking function*/
/* 
   Error codes:
      1 -> Error: Invalid symbol
      2 -> Error: Number length too long
      3 -> Error: Ident length too long
*/
void errorChecker(int error_code, const char *errorLex) {
  if (lex_index >= MAX_LEXEMES) {
    printf("Error: lexeme list overflow\n");
    exit(1);
  }
  if (errorLex != NULL) {
    strncpy(list[lex_index].name, errorLex, sizeof(list[lex_index].name) - 1);
    list[lex_index].name[sizeof(list[lex_index].name) - 1] = '\0';
  } else {
    strncpy(list[lex_index].name, "ERROR", sizeof(list[lex_index].name) - 1);
    list[lex_index].name[sizeof(list[lex_index].name) - 1] = '\0';
  }
  list[lex_index].type = -error_code; // negative value indicates an erro
  lex_index++;
}

void printtokens(void) {
    int i;
    int errorFound = 0;  // Flag to track if any errors were found

    // Only iterate through tokens to find errors
    for (i = 0; i < lex_index; i++) {
        if (list[i].type < 0) {  // Negative type indicates error
            if (!errorFound) {  // Print header only if this is the first error
                printf("Lexical Errors:\n");
                errorFound = 1;
            }

            switch (-list[i].type) {
                case 1:
                    printf("Error: Invalid symbol '%s'\n", list[i].name);
                    break;
                case 2:
                    printf("Error: Number '%s' is too long\n", list[i].name);
                    break;
                case 3:
                    printf("Error: Identifier '%s' is too long\n", list[i].name);
                    break;
            }
        }
    }

    // If no errors were found, program can continue
    if (!errorFound) {
        return;
    } else {
        exit(1);  // Exit if errors were found
    }
}

/*Lexical analyzer*/
//Processes the input string --> builds the lexeme table
lexeme *LA(char *input) {
  size_t input_len = strlen(input);
  list = malloc(MAX_LEXEMES * sizeof(lexeme));
  if (list == NULL) {
    printf("Error: Memory allocation for lexeme list failed\n");
    exit(1);
  }
  lex_index = 0;

  // Process each character of input
  while (input_index < input_len) { 
    // Check for block comments delimited by /* ... */ only if a closing "*/" exists.
    if (input[input_index] == '/' && input[input_index + 1] == '*') {
      char *closing = strstr(input + input_index + 2, "*/");
      if (closing != NULL) {
        commentChecker(input, 1); // Process block comment
        continue;
      }
      // If no closing "*/" is found, dont treat as a comment
      // the '/' and '*' will be tokenized normally
    }
    // Do not treat // as comments -- tokenize them
    if (input[input_index] == '"') {
      errorChecker(1, "\"");
      input_index++;
      continue;
    } else if (iscntrl(input[input_index]) || isspace(input[input_index])) {
      invisibleCharChecker(input);
    } else if (isalpha(input[input_index])) {
      wordChecker(input);
    } else if (isdigit(input[input_index])) {
      numberChecker(input);
    } else if (is_symbol(input[input_index])) {
      symbolChecker(input);
    } else {
      char unknown[2] = {input[input_index], '\0'};
      input_index++;
      errorChecker(1, unknown);
    }
  }
  return list;
}
