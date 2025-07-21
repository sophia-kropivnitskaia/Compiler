// Sophia Kropivnitskaia and Harshika Jindal
// Course: Systems Software
// Semester: Summer 2025
// HW 4 - Extended PL/0 Compiler (Parser and Code Generator)

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

// Token types (updated for HW4)
typedef enum {
    modsym = 1, identsym = 2, numbersym = 3, plussym = 4, minussym = 5, 
    multsym = 6, slashsym = 7, fisym = 8, eqlsym = 9, neqsym = 10, lessym = 11, 
    leqsym = 12, gtrsym = 13, geqsym = 14, lparentsym = 15, rparentsym = 16, 
    commasym = 17, semicolonsym = 18, periodsym = 19, becomessym = 20, 
    beginsym = 21, endsym = 22, ifsym = 23, thensym = 24, whilesym = 25, 
    dosym = 26, callsym = 27, constsym = 28, varsym = 29, procsym = 30,
    writesym = 31, readsym = 32, elsesym = 33
} tokenType;

// VM instructions
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

// Symbol table structure (updated for procedures)
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
int cx = 0;  // code index
int currentLevel = 0; // Current lexical level

// Error handling
typedef struct {
    int line;
    int column;
    char message[100];
} Error;

Error errors[100];
int errorCount = 0;
int hasError = 0;

// Error messages (updated for HW4)
const char* error_messages[] = {
    "program must end with period",
    "const, var, procedure must be followed by identifier",
    "symbol name has already been declared",
    "constants must be assigned with =",
    "constants must be assigned an integer value",
    "constant and variable declarations must be followed by a semicolon",
    "incorrect symbol after procedure declaration",
    "statement expected",
    "incorrect symbol after statement part in block",
    "period expected",
    "semicolon between statements missing",
    "undeclared identifier",
    "assignment to constant or procedure is not allowed",
    "assignment operator expected",
    "call must be followed by an identifier",
    "call of a constant or variable is meaningless",
    "then expected",
    "semicolon or end expected",
    "do expected",
    "incorrect symbol following statement",
    "relational operator expected",
    "expression must not contain a procedure identifier",
    "right parenthesis missing",
    "the preceding factor cannot begin with this symbol",
    "an expression cannot begin with this symbol",
    "this number is too large",
    "identifier too long",
    "invalid symbol",
    "else expected",
    "if must be followed by then and else"
};

// Function prototypes
void scanTokens(FILE *input);
int isReservedWord(char* id);
void get_next_token();
void error(int error_num);
void emit(int op, int L, int M);
int find_symbol(char* name, int level);
void program();
void block(int level);
void const_declaration();
int var_declaration();
void procedure_declaration(int level);
void statement();
void condition();
void expression();
void term();
void factor();
void print_errors();
void generate_elf_file();

// Helper functions
void add_error(const char* msg) {
    if (errorCount < 100) {
        strncpy(errors[errorCount].message, msg, 99);
        errorCount++;
        hasError = 1;
    }
}

int isLetter(char c) {
    return isalpha(c);
}

int isNumber(char c) {
    return isdigit(c);
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
    int lineNum = 1;

    while (fgets(buffer, sizeof(buffer), input)) {
        int i = 0;
        int colNum = 1;
        while (buffer[i] != '\0') {
            char c = buffer[i];

            // Skip whitespace
            if (isspace(c)) { 
                i++; 
                colNum++;
                continue; 
            }

            // Handle comments
            if (buffer[i] == '/' && buffer[i + 1] == '*') {
                int j = i + 2;
                colNum += 2;
                while (!(buffer[j] == '*' && buffer[j + 1] == '/')) {
                    if (buffer[j] == '\0') {
                        add_error("Unterminated comment");
                        break;
                    }
                    j++;
                    colNum++;
                }
                i = j + 2;
                colNum += 2;
                continue;
            }

            // Process identifiers and reserved words
            if (isLetter(c)) {
                int startCol = colNum;
                char id[100];
                int j = 0;
                while (isLetter(buffer[i]) || isNumber(buffer[i])) {
                    if (j < 99) id[j++] = buffer[i];
                    i++;
                    colNum++;
                }
                id[j] = '\0';

                if (strlen(id) > MAX_ID_LEN) {
                    add_error("Identifier too long");
                }
                continue;
            }

            // Process numbers
            if (isNumber(c)) {
                int startCol = colNum;
                char num[100];
                int j = 0;
                while (isNumber(buffer[i])) {
                    if (j < 99) num[j++] = buffer[i];
                    i++;
                    colNum++;
                }
                // Check for decimal point (invalid in PL/0)
                if (buffer[i] == '.') {
                    add_error("Decimal numbers not allowed");
                    while (isNumber(buffer[i]) || buffer[i] == '.') {
                        i++;
                        colNum++;
                    }
                    continue;
                }
                num[j] = '\0';
                if (strlen(num) > MAX_NUM_LEN) {
                    add_error("Number too long");
                }
                continue;
            }

            // Process special symbols
            switch (c) {
                case '+': break;
                case '-': break;
                case '*': break;
                case '/': break;
                case '(': break;
                case ')': break;
                case '=': break;
                case ',': break;
                case '.': break;
                case '<':
                    if (buffer[i+1] == '=') { i++; colNum++; }
                    else if (buffer[i+1] == '>') { i++; colNum++; }
                    break;
                case '>':
                    if (buffer[i+1] == '=') { i++; colNum++; }
                    break;
                case ';': break;
                case ':':
                    if (buffer[i+1] == '=') { i++; colNum++; }
                    else { add_error("Invalid symbol ':'"); }
                    break;
                default:
                    add_error("Invalid symbol");
                    break;
            }
            i++;
            colNum++;
        }
        lineNum++;
    }

    if (!hasError) {
        // Store tokens for parser
        rewind(input);
        lineNum = 1;
        while (fgets(buffer, sizeof(buffer), input)) {
            int i = 0;
            int colNum = 1;
            while (buffer[i] != '\0') {
                char c = buffer[i];

                if (isspace(c)) {
                    i++;
                    colNum++;
                    continue;
                }

                if (c == '/' && buffer[i+1] == '*') {
                    i += 2;
                    colNum += 2;
                    while (!(buffer[i] == '*' && buffer[i+1] == '/')) {
                        if (buffer[i] == '\0') break;
                        i++;
                        colNum++;
                    }
                    i += 2;
                    colNum += 2;
                    continue;
                }

                if (isLetter(c)) {
                    char id[100];
                    int j = 0;
                    int startCol = colNum;
                    while (isLetter(buffer[i]) || isNumber(buffer[i])) {
                        if (j < 99) id[j++] = buffer[i];
                        i++;
                        colNum++;
                    }
                    id[j] = '\0';
                    int token = isReservedWord(id);
                    tokenList[tokenCount].type = token ? token : identsym;
                    strcpy(tokenList[tokenCount].lexeme, token ? "" : id);
                    tokenList[tokenCount].line = lineNum;
                    tokenList[tokenCount].column = startCol;
                    tokenCount++;
                    continue;
                }

                if (isNumber(c)) {
                    char num[100];
                    int j = 0;
                    int startCol = colNum;
                    while (isNumber(buffer[i])) {
                        if (j < 99) num[j++] = buffer[i];
                        i++;
                        colNum++;
                    }
                    num[j] = '\0';
                    tokenList[tokenCount].type = numbersym;
                    strcpy(tokenList[tokenCount].lexeme, num);
                    tokenList[tokenCount].line = lineNum;
                    tokenList[tokenCount].column = startCol;
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
                    case '<':
                        if (buffer[i+1] == '=') { singleToken = leqsym; i++; colNum++; }
                        else if (buffer[i+1] == '>') { singleToken = neqsym; i++; colNum++; }
                        else { singleToken = lessym; }
                        break;
                    case '>':
                        if (buffer[i+1] == '=') { singleToken = geqsym; i++; colNum++; }
                        else { singleToken = gtrsym; }
                        break;
                    case ';': singleToken = semicolonsym; break;
                    case ':':
                        if (buffer[i+1] == '=') { singleToken = becomessym; i++; colNum++; }
                        break;
                }
                if (singleToken > 0) {
                    tokenList[tokenCount].type = singleToken;
                    tokenList[tokenCount].lexeme[0] = '\0';
                    tokenList[tokenCount].line = lineNum;
                    tokenList[tokenCount].column = colNum;
                    tokenCount++;
                }
                i++;
                colNum++;
            }
            lineNum++;
        }
    }
}

void get_next_token() {
    if (currentTokenIndex < tokenCount) {
        currentToken = &tokenList[currentTokenIndex++];
    } else {
        // End of tokens, treat as period
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
    exit(1);
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

// symbolTable Check with level
int find_symbol(char* name, int level) {
    for (int i = sym_table_size - 1; i >= 0; i--) {
        if (symbol_table[i].mark == 0 && strcmp(symbol_table[i].name, name) == 0) {
            // Check if the symbol is in current or enclosing scope
            if (symbol_table[i].level <= level) {
                return i;
            }
        }
    }
    return -1;
}

void program() {
    // First instruction should be JMP to skip variable allocation
    emit(JMP, 0, 0); // Address will be updated later

    get_next_token();
    block(0);
    if (tokenList[tokenCount-1].type != periodsym) {
        error(0); // program must end with period
    }

    // Mark all symbols at program end
    for (int i = 0; i < sym_table_size; i++) {
        symbol_table[i].mark = 1;
    }

    emit(SYS, 0, 3); // HALT instruction
}

void block(int level) {
    int tx0 = sym_table_size; // Save current symbol table position
    int cx0 = cx;             // Save current code position
    
    // Update the JMP instruction to jump to current code position
    if (level == 0) {
        code[0].M = cx;
    }

    const_declaration();
    int num_vars = var_declaration();
    
    // Process procedure declarations
    while (currentToken->type == procsym) {
        procedure_declaration(level);
    }
    
    emit(INC, 0, 3 + num_vars); // Allocate space for variables
    
    statement();
    
    // Mark symbols that go out of scope
    for (int i = tx0; i < sym_table_size; i++) {
        if (symbol_table[i].level == level) {
            symbol_table[i].mark = 1;
        }
    }
}

void const_declaration() {
    if (currentToken->type == constsym) {
        do {
            get_next_token();
            if (currentToken->type != identsym) {
                error(1); // const must be followed by identifier
            }
            
            char name[MAX_ID_LEN + 1];
            strcpy(name, currentToken->lexeme);
            
            if (find_symbol(name, currentLevel) != -1) {
                error(2); // symbol already declared
            }
            
            get_next_token();
            if (currentToken->type != eqlsym) {
                error(3); // constants must be assigned with =
            }
            
            get_next_token();
            if (currentToken->type != numbersym) {
                error(4); // constants must be assigned an integer value
            }
            
            // Add to symbol table
            symbol_table[sym_table_size].kind = 1;
            strcpy(symbol_table[sym_table_size].name, name);
            symbol_table[sym_table_size].val = atoi(currentToken->lexeme);
            symbol_table[sym_table_size].level = currentLevel;
            symbol_table[sym_table_size].addr = 0;
            symbol_table[sym_table_size].mark = 0;
            sym_table_size++;
            
            get_next_token();
        } while (currentToken->type == commasym);
        
        if (currentToken->type != semicolonsym) {
            error(5); // const declaration must end with semicolon
        }
        get_next_token();
    }
}

int var_declaration() {
    int num_vars = 0;
    if (currentToken->type == varsym) {
        do {
            num_vars++;
            get_next_token();
            if (currentToken->type != identsym) {
                error(1); // var must be followed by identifier
            }
            
            char name[MAX_ID_LEN + 1];
            strcpy(name, currentToken->lexeme);
            
            if (find_symbol(name, currentLevel) != -1) {
                error(2); // symbol already declared
            }
            
            // Add to symbol table
            symbol_table[sym_table_size].kind = 2;
            strcpy(symbol_table[sym_table_size].name, name);
            symbol_table[sym_table_size].val = 0;
            symbol_table[sym_table_size].level = currentLevel;
            symbol_table[sym_table_size].addr = num_vars + 2; // First var at address 3
            symbol_table[sym_table_size].mark = 0;
            sym_table_size++;
            
            get_next_token();
        } while (currentToken->type == commasym);
        
        if (currentToken->type != semicolonsym) {
            error(5); // var declaration must end with semicolon
        }
        get_next_token();
    }
    return num_vars;
}

void procedure_declaration(int level) {
    get_next_token(); // Skip 'procedure'
    
    if (currentToken->type != identsym) {
        error(1); // procedure must be followed by identifier
    }
    
    char name[MAX_ID_LEN + 1];
    strcpy(name, currentToken->lexeme);
    
    if (find_symbol(name, currentLevel) != -1) {
        error(2); // symbol already declared
    }
    
    // Add procedure to symbol table
    symbol_table[sym_table_size].kind = 3;
    strcpy(symbol_table[sym_table_size].name, name);
    symbol_table[sym_table_size].val = 0;
    symbol_table[sym_table_size].level = currentLevel;
    symbol_table[sym_table_size].addr = cx; // Address of procedure code
    symbol_table[sym_table_size].mark = 0;
    int proc_index = sym_table_size;
    sym_table_size++;
    
    get_next_token();
    if (currentToken->type != semicolonsym) {
        error(5); // procedure declaration must end with semicolon
    }
    get_next_token();
    
    // Process procedure block
    currentLevel++;
    block(currentLevel);
    currentLevel--;
    
    emit(OPR, 0, 0); // Return from procedure
    
    if (currentToken->type != semicolonsym) {
        error(5); // procedure must end with semicolon
    }
    get_next_token();
    
    // Update procedure address in symbol table
    symbol_table[proc_index].addr = cx;
}

void statement() {
    if (currentToken->type == identsym) {
        // Assignment statement
        char name[MAX_ID_LEN + 1];
        strcpy(name, currentToken->lexeme);
        int sym_idx = find_symbol(name, currentLevel);
        
        if (sym_idx == -1) {
            error(11); // undeclared identifier
        }
        if (symbol_table[sym_idx].kind != 2) {
            error(12); // only variables can be assigned to
        }
        
        get_next_token();
        if (currentToken->type != becomessym) {
            error(13); // assignment must use :=
        }
        
        get_next_token();
        expression();
        emit(STO, currentLevel - symbol_table[sym_idx].level, symbol_table[sym_idx].addr);
    }
    else if (currentToken->type == callsym) {
        // Call statement
        get_next_token();
        if (currentToken->type != identsym) {
            error(14); // call must be followed by identifier
        }
        
        char name[MAX_ID_LEN + 1];
        strcpy(name, currentToken->lexeme);
        int sym_idx = find_symbol(name, currentLevel);
        
        if (sym_idx == -1) {
            error(11); // undeclared identifier
        }
        if (symbol_table[sym_idx].kind != 3) {
            error(15); // can only call procedures
        }
        
        emit(CAL, currentLevel - symbol_table[sym_idx].level, symbol_table[sym_idx].addr);
        get_next_token();
    }
    else if (currentToken->type == beginsym) {
        // Compound statement
        get_next_token();
        statement();
        
        while (currentToken->type == semicolonsym) {
            get_next_token();
            // Check if this is an empty statement before end
            if (currentToken->type != endsym) {
                statement();
            }
        }
        
        if (currentToken->type != endsym) {
            error(9); // begin must be followed by end
        }
        get_next_token();
    }
    else if (currentToken->type == ifsym) {
        // If statement with else clause
        get_next_token();
        condition();
        
        if (currentToken->type != thensym) {
            error(16); // if must be followed by then
        }
        
        int jpc_idx = cx;
        emit(JPC, 0, 0); // Placeholder address
        
        get_next_token();
        statement();
        
        if (currentToken->type != elsesym) {
            error(28); // then must be followed by else
        }
        
        int jmp_idx = cx;
        emit(JMP, 0, 0); // Placeholder for jump over else
        
        // Update the JPC to jump to else part
        code[jpc_idx].M = cx;
        
        get_next_token();
        statement();
        
        if (currentToken->type != fisym) {
            error(29); // else must be followed by fi
        }
        get_next_token();
        
        // Update the JMP to jump after else part
        code[jmp_idx].M = cx;
    }
    else if (currentToken->type == whilesym) {
        // While statement
        int loop_idx = cx;
        get_next_token();
        condition();
        
        if (currentToken->type != dosym) {
            error(18); // while must be followed by do
        }
        
        int jpc_idx = cx;
        emit(JPC, 0, 0); // Placeholder address
        
        get_next_token();
        statement();
        
        emit(JMP, 0, loop_idx);
        code[jpc_idx].M = cx; // Update jump address
    }
    else if (currentToken->type == readsym) {
        // Read statement
        get_next_token();
        if (currentToken->type != identsym) {
            error(1); // read must be followed by identifier
        }
        
        int sym_idx = find_symbol(currentToken->lexeme, currentLevel);
        if (sym_idx == -1) {
            error(11); // undeclared identifier
        }
        if (symbol_table[sym_idx].kind != 2) {
            error(12); // only variables can be read into
        }
        
        emit(SYS, 0, 1); // READ
        emit(STO, currentLevel - symbol_table[sym_idx].level, symbol_table[sym_idx].addr);
        
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
        error(20); // condition must contain comparison operator
    }
}

void expression() {
    while (currentToken->type == plussym || currentToken->type == minussym) {
        int addop = currentToken->type;
        if (addop == plussym) {
            get_next_token();
            term();
            emit(OPR, 0, 2); // ADD
        } else {
            get_next_token();
            term();
            emit(OPR, 0, 3); // SUB
        }
    }
}

void term() {
    factor();
    while (currentToken->type == multsym || currentToken->type == slashsym || currentToken->type == modsym) {
        int mulop = currentToken->type;
        get_next_token();
        factor();
        switch (mulop) {
            case multsym: emit(OPR, 0, 4); break; // MUL
            case slashsym: emit(OPR, 0, 5); break; // DIV
            case modsym: emit(OPR, 0, 11); break; // MOD (new for HW4)
        }
    }
}

void factor() {
    if (currentToken->type == identsym) {
        int sym_idx = find_symbol(currentToken->lexeme, currentLevel);
        if (sym_idx == -1) {
            error(11); // undeclared identifier
        }
        
        if (symbol_table[sym_idx].kind == 1) {
            emit(LIT, 0, symbol_table[sym_idx].val); // Constant
        }
        else if (symbol_table[sym_idx].kind == 2) {
            emit(LOD, currentLevel - symbol_table[sym_idx].level, symbol_table[sym_idx].addr); // Variable
        } else {
            error(21); // can't use procedure in expression
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
            error(22); // right parenthesis must follow left parenthesis
        }
        get_next_token();
    }
    else {
        error(24); // invalid factor
    }
}

void print_errors() {
    if (errorCount > 0) {
        printf("\nErrors: ");
        for (int i = 0; i < errorCount; i++) {
            printf("%s\n", errors[i].message);
        }
    }
}

void generate_elf_file() {
    FILE* elf = fopen("elf.txt", "w");
    if (!elf) {
        perror("Error creating elf.txt");
        return;
    }
    
    for (int i = 0; i < cx; i++) {
        fprintf(elf, "%d %d %d\n", code[i].op, code[i].L, code[i].M);
    }
    
    fclose(elf);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (!input) {
        perror("Error opening file");
        return 1;
    }

    // First pass - lexical analysis
    scanTokens(input);
    
    print_errors();
    if (hasError) {
        fclose(input);
        return 1;
    }
    
    // Second pass - parsing and code generation
    program();
    
    // Print generated code
    printf("\nAssembly Code:\n");
    printf("Line OP   L M\n");
    for (int i = 0; i < cx; i++) {
        printf("%4d %-3s  %d %d\n", i, opnames[code[i].op], code[i].L, code[i].M);
    }
    
    // Print symbol table
    printf("\nSymbol Table:\n");
    printf("Kind | Name     | Value | Level | Address | Mark\n");
    printf("-----------------------------------------------\n");
    for (int i = 0; i < sym_table_size; i++) {
        printf("%4d | %-8s | %5d | %5d | %7d | %4d\n",
               symbol_table[i].kind,
               symbol_table[i].name,
               symbol_table[i].val,
               symbol_table[i].level,
               symbol_table[i].addr,
               symbol_table[i].mark);
    }
    
    // Generate elf.txt file
    generate_elf_file();
    
    fclose(input);
    return 0;
}
