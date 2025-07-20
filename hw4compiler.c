// Sophia Kropivnitskaia and Harshika Jindal
// Course: Systems Software
// Semester: Summer 2025
// HW 4 - Extended PL/0 Compiler with Procedures and Modulo

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_ID_LEN 11
#define MAX_NUM_LEN 5
#define MAX_LINE_LEN 256
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_TOKENS 10000
#define CODE_SIZE 500

// Function prototypes
int isLetter(char c);
int isNumber(char c);
int isASpecialSymbol(char input);
int isReservedWord(char* id);
void scanTokens(FILE *input);
void get_next_token();
void error(int error_num);
void emit(int op, int L, int M);
int find_symbol(char* name);
void program();
void block();
void const_declaration();
int var_declaration();
void procedure_declaration();
void statement();
void condition();
void expression();
void term();
void factor();
void print_errors();
void print_source_program(FILE *input);

// Token types
typedef enum {
    modsym = 1, identsym = 2, numbersym = 3, plussym = 4, minussym = 5,
    multsym = 6, slashsym = 7, fisym = 8, eqlsym = 9, neqsym = 10,
    lessym = 11, leqsym = 12, gtrsym = 13, geqsym = 14, lparentsym = 15,
    rparentsym = 16, commasym = 17, semicolonsym = 18, periodsym = 19,
    becomessym = 20, beginsym = 21, endsym = 22, ifsym = 23, thensym = 24,
    whilesym = 25, dosym = 26, callsym = 27, constsym = 28, varsym = 29,
    procsym = 30, writesym = 31, readsym = 32, elsesym = 33
} tokenType;

// VM instruction opcodes
#define LIT 1
#define OPR 2
#define LOD 3
#define STO 4
#define CAL 5
#define INC 6
#define JMP 7
#define JPC 8
#define SYS 9

const char* opnames[] = {
    "", "LIT", "OPR", "LOD", "STO", "CAL", "INC", "JMP", "JPC", "SYS"
};

// Token structure
typedef struct {
    int type;
    char lexeme[100];
    int line;
    int column;
} Token;

// Virtual machine instruction structure
typedef struct {
    int op;  // opcode
    int L;   // lexicographical level
    int M;   // modifier
} instruction;

// Symbol table structure
typedef struct {
    int kind;       // const = 1, var = 2, proc = 3
    char name[11];  // name up to 11 chars
    int val;        // number (ASCII value)
    int level;      // L level
    int addr;       // M address
    int mark;       // to indicate unavailable or deleted
} symbol;

// Global variables
Token tokenList[MAX_TOKENS];
int tokenCount = 0;
int currentTokenIndex = 0;
Token* currentToken = NULL;

symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
int sym_table_size = 0;
instruction code[CODE_SIZE];
int cx = 0; // code index
int lex = 0; // Lexical Level 

int hasError = 0;
int errorCount = 0;

typedef struct {
    char message[100];
} Error;

Error errors[100];

const char symbols[] = {
    '+', '-', '*', '/', '(', ')', '=', ',', '.', '<', '>', ';', ':'
};

const char* error_messages[] = {
    "period expected", //0
    "identifier must be followed by =",//1
    "= must be followed by a number",//2
    "const, var, procedure must be followed by identifier",//3
    "semicolon or comma missing",//4
    "incorrect symbol after procedure declaration",//5
    "statement expected",//6
    "incorrect symbol after statement part in block",//7
    "assignment operator expected",//8
    "call must be followed by an identifier",//9
    "then expected",//10
    "do expected",//11
    "relational operator expected",//12
    "right parenthesis missing",//13
    "invalid factor",//14
    "call of a constant or variable is meaningless",//15
    "unmatched right parenthesis",//16
    "constants must be integers",//17
    "begin must be followed by end",//18
    "undeclared identifier",//19
    "assignment to constant or procedure not allowed"//20
};



int isLetter(char c) {
    return isalpha(c);
}

int isNumber(char c) {
    return isdigit(c);
}

int isASpecialSymbol(char input) {
    for (int i = 0; i < 13; i++) {
        if (input == symbols[i])
            return 1;
    }
    return 0;
}

int isReservedWord(char* id) {
    if (strcmp(id, "mod") == 0) return modsym;
    if (strcmp(id, "const") == 0) return constsym;
    if (strcmp(id, "var") == 0) return varsym;
    if (strcmp(id, "procedure") == 0) return procsym;
    if (strcmp(id, "call") == 0) return callsym;
    if (strcmp(id, "begin") == 0) return beginsym;
    if (strcmp(id, "end") == 0) return endsym;
    if (strcmp(id, "if") == 0) return ifsym;
    if (strcmp(id, "then") == 0) return thensym;
    if (strcmp(id, "else") == 0) return elsesym;
    if (strcmp(id, "fi") == 0) return fisym;
    if (strcmp(id, "while") == 0) return whilesym;
    if (strcmp(id, "do") == 0) return dosym;
    if (strcmp(id, "read") == 0) return readsym;
    if (strcmp(id, "write") == 0) return writesym;
    return 0; // not a reserved word
}

void scanTokens(FILE *input) {
    char buffer[MAX_LINE_LEN];
    int in_comment = 0;

    while (fgets(buffer, sizeof(buffer), input)) {
        int i = 0;
        while (buffer[i] != '\0') {
            char c = buffer[i];

            if (in_comment) {
                if (c == '*' && buffer[i+1] == '/') {
                    in_comment = 0;
                    i += 2;
                } else {
                    i++;
                }
                continue;
            }

            if (isspace(c)) { i++; continue; }

            if (c == '/' && buffer[i+1] == '*') {
                in_comment = 1;
                i += 2;
                continue;
            }

            if (isLetter(c)) {
                char id[100]; int j = 0;
                while (isLetter(buffer[i]) || isNumber(buffer[i])) {
                    if (j < 99) id[j++] = buffer[i];
                    i++;
                }
                id[j] = '\0';
                if (strlen(id) > MAX_ID_LEN) {
                    printf("%s\t\tError: Identifier too long\n", id); 
                    hasError = 1;
                }
                continue;
            }

            if (isNumber(c)) {
                char num[100]; int j = 0;
                while (isNumber(buffer[i])) {
                    if (j < 99) num[j++] = buffer[i];
                    i++;
                }
                if (buffer[i] == '.') {
                    while (isNumber(buffer[i])) { i++; }
                    continue;
                }
                num[j] = '\0';
                if (strlen(num) > MAX_NUM_LEN) {
                    printf("%s\t\tError: Number too long\n", num); 
                    hasError = 1;
                }
                continue;
            }

            if (!strchr("+-*/()=,.;<>:", c)) {
                printf("Error: Invalid symbol '%c'\n", c);
                hasError = 1;
            }
            i++;
        }
    }

    if (in_comment) {
        //add_error("Unterminated comment");
        printf("Error: Unterminated comment\n");
    }

    if (!hasError) {
        rewind(input);
        in_comment = 0;
        while (fgets(buffer, sizeof(buffer), input)) {
            int i = 0;
            while (buffer[i] != '\0') {
                char c = buffer[i];

                if (in_comment) {
                    if (c == '*' && buffer[i+1] == '/') {
                        in_comment = 0;
                        i += 2;
                    } else {
                        i++;
                    }
                    continue;
                }

                if (isspace(c)) { i++; continue; }

                if (c == '/' && buffer[i+1] == '*') {
                    in_comment = 1;
                    i += 2;
                    continue;
                }

                if (isLetter(c)) {
                    char id[100]; 
                    int j = 0;
                    while (isLetter(buffer[i]) || isNumber(buffer[i])) {
                        if (j < 99) id[j++] = buffer[i];
                        i++;
                    }
                    id[j] = '\0';
                    int token = isReservedWord(id);
                    tokenList[tokenCount].type = token ? token : identsym;
                    strcpy(tokenList[tokenCount].lexeme, token ? "" : id);

                    tokenCount++;
                    continue;
                }

                if (isNumber(c)) {
                    char num[100]; int j = 0;
                    while (isNumber(buffer[i])) {
                        if (j < 99) num[j++] = buffer[i];
                        i++;
                    }
                    num[j] = '\0';
                    tokenList[tokenCount].type = numbersym;
                    strcpy(tokenList[tokenCount].lexeme, num);
                    tokenCount++;
                    continue;
                }

                int singleToken = 0;
                switch (c) {
                    case '+': singleToken = plussym; break;
                    case '-': singleToken = minussym; break;
                    case '*': singleToken = multsym; break;
                    case '/': singleToken = slashsym; break;
                    case '(': singleToken = lparentsym; break;
                    case ')': singleToken = rparentsym; break;
                    case '=': singleToken = eqlsym; break;
                    case ',': singleToken = commasym; break;
                    case '.': singleToken = periodsym; break;
                    case ';': singleToken = semicolonsym; break;
                    case '<':
                        if (buffer[i+1] == '=') { singleToken = leqsym; i++; }
                        else if (buffer[i+1] == '>') { singleToken = neqsym; i++; }
                        else { singleToken = lessym; }
                        break;
                    case '>':
                        if (buffer[i+1] == '=') { singleToken = geqsym; i++; }
                        else { singleToken = gtrsym; }
                        break;
                    case ':':
                        if (buffer[i+1] == '=') { singleToken = becomessym; i++; }
                        break;
                }
                if (singleToken > 0) {
                    tokenList[tokenCount].type = singleToken;
                    tokenList[tokenCount].lexeme[0] = '\0';
                    tokenCount++;
                }
                i++;
            }
        }
    }
}

void get_next_token() {
    if (currentTokenIndex < tokenCount) {
        currentToken = &tokenList[currentTokenIndex++];
    } else {
        static Token endToken = {periodsym, "", -1, -1};
        currentToken = &endToken;
    }
}

void error(int error_num) {
    if (error_num >= 0 && error_num < sizeof(error_messages)/sizeof(error_messages[0])) {
        printf("Error: %s\n", error_messages[error_num]);
    } else {
        printf("Unknown error\n");
    }
    hasError = 1;
}

void emit(int op, int L, int M) {
    if (cx >= CODE_SIZE) {
        printf("Error: Program too long\n");
        exit(1);
    }
    code[cx].op = op;
    code[cx].L = L;
    code[cx].M = M;
    cx++;
}

int find_symbol(char* name) {
    for (int i = sym_table_size-1; i >= 0; i--) {
        if (symbol_table[i].mark == 0 && strcmp(symbol_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void program() {
    emit(JMP, 0, 0); // Will be fixed up after procedure declarations
    
    get_next_token();
    block();
    
    if (currentToken->type != periodsym) {
        error(0); // "period expected"
    }
    
    // Mark all symbols at program end
    for (int i = 0; i < sym_table_size; i++) {
        symbol_table[i].mark = 1;
    }
    
    emit(SYS, 0, 3); // HALT instruction
}

void block() {
    const_declaration();
    int num_vars = var_declaration();
    procedure_declaration();
    
    // Fix up the initial JMP to skip procedure bodies
    code[0].M = cx;
    
    emit(INC, 0, 3 + num_vars); // Allocate space for variables
    
    statement();
}

void const_declaration() {
    if (currentToken->type == constsym) {
        do {
            get_next_token();
            if (currentToken->type != identsym) {
                error(3); // "const must be followed by identifier"
                return;
            }
            
            char name[MAX_ID_LEN + 1];
            strcpy(name, currentToken->lexeme);
            
            if (find_symbol(name) != -1) {
                error(2); // "symbol already declared"
                return;
            }
            
            get_next_token();
            if (currentToken->type != eqlsym) {
                error(1); // "identifier must be followed by ="
                return;
            }
            
            get_next_token();
            if (currentToken->type != numbersym) {
                error(2); // "= must be followed by a number"
                return;
            }
            
            // Add to symbol table
            symbol_table[sym_table_size].kind = 1;
            strcpy(symbol_table[sym_table_size].name, name);
            symbol_table[sym_table_size].val = atoi(currentToken->lexeme);
            symbol_table[sym_table_size].level = lex;
            symbol_table[sym_table_size].addr = 0;
            symbol_table[sym_table_size].mark = 0;
            sym_table_size++;
            
            get_next_token();
        } while (currentToken->type == commasym);
        
        if (currentToken->type != semicolonsym) {
            error(4); // "semicolon missing"
            return;
        }
        get_next_token();
    }
}

int var_declaration() {
    int num_vars = 0;
    if (currentToken->type == varsym) {
        do {
            get_next_token();
            if (currentToken->type != identsym) {
                error(3); // "var must be followed by identifier"
                return num_vars;
            }
            
            char name[MAX_ID_LEN + 1];
            strcpy(name, currentToken->lexeme);
            
            if (find_symbol(name) != -1) {
                error(2); // "symbol already declared"
                return num_vars;
            }
            
            // Add to symbol table
            symbol_table[sym_table_size].kind = 2;
            strcpy(symbol_table[sym_table_size].name, name);
            symbol_table[sym_table_size].val = 0;
            symbol_table[sym_table_size].level = lex;
            symbol_table[sym_table_size].addr = num_vars + 3; // First var at address 3
            symbol_table[sym_table_size].mark = 0;
            sym_table_size++;
            num_vars++;
            
            get_next_token();
        } while (currentToken->type == commasym);
        
        if (currentToken->type != semicolonsym) {
            error(4); // "semicolon missing"
            return num_vars;
        }
        get_next_token();
    }
    return num_vars;
}

void procedure_declaration() {
    while (currentToken->type == procsym) {
        get_next_token();
        
        if (currentToken->type != identsym) {
            error(3); // "procedure must be followed by identifier"
            return;
        }
        
        char name[MAX_ID_LEN + 1];
        strcpy(name, currentToken->lexeme);
        
        if (find_symbol(name) != -1) {
            error(2); // "symbol already declared"
            return;
        }
        
        // Add procedure to symbol table
        symbol_table[sym_table_size].kind = 3;
        strcpy(symbol_table[sym_table_size].name, name);
        symbol_table[sym_table_size].level = lex;
        symbol_table[sym_table_size].addr = cx; // store current code index
        symbol_table[sym_table_size].mark = 0;
        sym_table_size++;
        
        get_next_token();
        
        if (currentToken->type != semicolonsym) {
            error(4); // "semicolon missing"
            return;
        }
        
        get_next_token();
        
        // Process procedure block with increased lexical level
        lex++;
        block();
        lex--;
        
        emit(OPR, 0, 0); // Return from procedure
        
        if (currentToken->type != semicolonsym) {
            error(4); // "semicolon missing"
            return;
        }
        
        get_next_token();
    }
}

void statement() {
    if (currentToken->type == identsym) {
        // Assignment statement
        char name[MAX_ID_LEN + 1];
        strcpy(name, currentToken->lexeme);
        int sym_idx = find_symbol(name);
        
        if (sym_idx == -1) {
            error(19); // "undeclared identifier"
            return;
        }
        if (symbol_table[sym_idx].kind != 2) {
            error(20); // "assignment to constant or procedure not allowed"
            return;
        }
        
        get_next_token();
        if (currentToken->type != becomessym) {
            error(8); // "assignment operator expected"
            return;
        }
        
        get_next_token();
        expression();
        emit(STO, symbol_table[sym_idx].level, symbol_table[sym_idx].addr);
    }
    else if (currentToken->type == callsym) {
        // Call statement
        get_next_token();
        
        if (currentToken->type != identsym) {
            error(9); // "call must be followed by an identifier"
            return;
        }
        
        int sym_idx = find_symbol(currentToken->lexeme);
        if (sym_idx == -1) {
            error(19); // "undeclared identifier"
            return;
        }
        if (symbol_table[sym_idx].kind != 3) {
            error(15); // "call of a constant or variable is meaningless"
            return;
        }
        
        emit(CAL, lex - symbol_table[sym_idx].level, symbol_table[sym_idx].addr);
        get_next_token();
    }
    else if (currentToken->type == beginsym) {
        get_next_token();
        
        // Parse first statement (required)
        statement();
        
        // Parse additional statements separated by semicolons
        while (currentToken->type == semicolonsym) {
            get_next_token();
            statement();
        }
        
        if (currentToken->type != endsym) {
            error(18); // "begin must be followed by end"
            return;
        }
        get_next_token();
    }
    else if (currentToken->type == ifsym) {
        // If statement
        get_next_token();
        condition();
        
        if (currentToken->type != thensym) {
            error(10); // "then expected"
            return;
        }
        
        int jpc_idx = cx;
        emit(JPC, 0, 0); // Placeholder for jump address
        
        get_next_token();
        statement();
        
        if (currentToken->type == elsesym) {
            // Handle else clause
            int jmp_idx = cx;
            emit(JMP, 0, 0); // Placeholder for jump over else
            
            // Update the if's JPC to jump to else
            code[jpc_idx].M = cx;
            
            get_next_token();
            statement();
            
            // Update the JMP to jump after else
            code[jmp_idx].M = cx;
        } else {
            // No else, just update JPC
            code[jpc_idx].M = cx;
        }
        
        if (currentToken->type != fisym) {
            error(10); // "fi expected"
            return;
        }
        get_next_token();
    }
    else if (currentToken->type == whilesym) {
        // While statement
        int loop_idx = cx;
        get_next_token();
        condition();
        
        if (currentToken->type != dosym) {
            error(11); // "do expected"
            return;
        }
        
        int jpc_idx = cx;
        emit(JPC, 0, 0); // Placeholder for jump address
        
        get_next_token();
        statement();
        
        emit(JMP, 0, loop_idx);
        code[jpc_idx].M = cx; // Update jump address
    }
    else if (currentToken->type == readsym) {
        // Read statement
        get_next_token();
        if (currentToken->type != identsym) {
            error(3); // "read must be followed by identifier"
            return;
        }
        
        int sym_idx = find_symbol(currentToken->lexeme);
        if (sym_idx == -1) {
            error(19); // "undeclared identifier"
            return;
        }
        if (symbol_table[sym_idx].kind != 2) {
            error(20); // "assignment to constant or procedure not allowed"
            return;
        }
        
        emit(SYS, 0, 1); // READ
        emit(STO, symbol_table[sym_idx].level, symbol_table[sym_idx].addr);
        get_next_token();
    }
    else if (currentToken->type == writesym) {
        // Write statement
        get_next_token();
        expression();
        emit(SYS, 0, 2); // WRITE
    }
    // Empty statement is allowed
}

void condition() {
    expression();
    if (currentToken->type == eqlsym || currentToken->type == neqsym || 
        currentToken->type == lessym || currentToken->type == leqsym || 
        currentToken->type == gtrsym || currentToken->type == geqsym) {
        
        int relop = currentToken->type;
        get_next_token();
        expression();
        
        switch (relop) {
            case eqlsym: emit(OPR, 0, 8); break;  // EQL
            case neqsym: emit(OPR, 0, 9); break;  // NEQ
            case lessym: emit(OPR, 0, 10); break; // LSS
            case leqsym: emit(OPR, 0, 11); break; // LEQ
            case gtrsym: emit(OPR, 0, 12); break; // GTR
            case geqsym: emit(OPR, 0, 13); break; // GEQ
        }
    }
    else {
        error(12); // "relational operator expected"
    }
}

void expression() {
    if (currentToken->type == plussym || currentToken->type == minussym) {
        int is_negative = (currentToken->type == minussym);
        get_next_token();
        term();
        
        if (is_negative) {
            emit(OPR, 0, 1); // NEG
        }
    } else {
        term();
    }
    
    while (currentToken->type == plussym || currentToken->type == minussym) {
        int op = currentToken->type;
        get_next_token();
        term();
        
        if (op == plussym) {
            emit(OPR, 0, 2); // ADD
        } else {
            emit(OPR, 0, 3); // SUB
        }
    }
}

void term() {
    factor();
    while (currentToken->type == multsym || currentToken->type == slashsym || currentToken->type == modsym) {
        int op = currentToken->type;
        get_next_token();
        factor();
        
        switch (op) {
            case multsym: emit(OPR, 0, 4); break; // MUL
            case slashsym: emit(OPR, 0, 5); break; // DIV
            case modsym: emit(OPR, 0, 11); break; // MOD
        }
    }
}

void factor() {
    if (currentToken->type == identsym) {
        int sym_idx = find_symbol(currentToken->lexeme);
        if (sym_idx == -1) {
            error(19); // "undeclared identifier"
            return;
        }
        
        if (symbol_table[sym_idx].kind == 1) {
            emit(LIT, 0, symbol_table[sym_idx].val); // Constant
        } else if (symbol_table[sym_idx].kind == 2) {
            emit(LOD, symbol_table[sym_idx].level, symbol_table[sym_idx].addr); // Variable
        } else {
            error(21); // "cannot use procedure as factor"
            return;
        }
        
        get_next_token();
    }
    else if (currentToken->type == numbersym) {
        emit(LIT, 0, atoi(currentToken->lexeme));
        get_next_token();
    }
    else if (currentToken->type == lparentsym) {
        get_next_token();
        expression();
        if (currentToken->type != rparentsym) {
            error(13); // "right parenthesis missing"
            return;
        }
        get_next_token();
    }
    else {
        error(14); // "invalid factor"
    }
}

void print_errors() {
    if (errorCount > 0) {
        printf("\nErrors:\n");
        for (int i = 0; i < errorCount; i++) {
            printf("%s\n", errors[i].message);
        }
    }
}

void print_source_program(FILE *input) {
    rewind(input);
    printf("\nSource Program:\n");
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), input)) {
        printf("%s", line);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    
    // if (argc < 2) {
   //      printf("Usage: %s <input_file>\n", argv[0]);
   //     return 1;
   // }
       
    //for testing purposes
    char* InputFile = "input.txt";
    //char* InputFile = argv[1];
    FILE *input = fopen(InputFile, "r");
    if (!input) {
        perror("Error opening file");
        return 1;
    }

    // First pass - lexical analysis
    print_source_program(input);
    scanTokens(input);
    
    print_errors();
    if (hasError) {
        fclose(input);
        return 1;
    }
    
    // Second pass - parsing and code generation
    currentTokenIndex = 0;
    get_next_token();
    program();
    
    if (!hasError) {
        printf("\nNo errors, program is syntactically correct.\n");
        
        // Print generated code
        printf("\nAssembly Code:\n");
        printf("Line OP   L M\n");
        for (int i = 0; i < cx; i++) {
            printf("%4d %-3s  %d %d\n", i, opnames[code[i].op], code[i].L, code[i].M);
        }
        
        // Print symbol table
        printf("\nSymbol Table:\n");
        printf("Kind | Name      | Value | Level | Address | Mark\n");
        printf("-------------------------------------------------\n");
        for (int i = 0; i < sym_table_size; i++) {
            printf("%4d | %-10s | %5d | %5d | %7d | %4d\n",
                   symbol_table[i].kind,
                   symbol_table[i].name,
                   symbol_table[i].val,
                   symbol_table[i].level,
                   symbol_table[i].addr,
                   symbol_table[i].mark);
        }
        
        // Generate elf.txt
        FILE *elf = fopen("elf.txt", "w");
        if (elf) {
            for (int i = 0; i < cx; i++) {
                fprintf(elf, "%d %d %d\n", code[i].op, code[i].L, code[i].M);
            }
            fclose(elf);
            printf("\nMachine code written to elf.txt\n");
        } else {
            printf("Error: Could not create elf.txt\n");
        }
    }
    
    fclose(input);
    return 0;
}