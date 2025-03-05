#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Definition of Token struct
typedef struct {
    char type[50];
    char value[256];
} Token;

// Definition of LexicalAnalyzer struct
typedef struct {
    // Keywords array and count
    const char **keywords;
    int keywords_count;
    
    // Operators array and count
    const char **operators;
    int operators_count;
    
    // String containing punctuation characters
    const char *punctuation;
    
    // String containing operator characters (for single character check)
    const char *operator_chars;
    
    // Symbol table (dynamic array of identifiers)
    char **symbol_table;
    int symbol_table_count;
    int symbol_table_capacity;
    
    // Lexical errors (dynamic array)
    char **lexical_errors;
    int lexical_errors_count;
    int lexical_errors_capacity;
    
    // Tokens (dynamic array)
    Token *tokens;
    int tokens_count;
    int tokens_capacity;
    
    int current_pos;
    int line_no;
} LexicalAnalyzer;

// Function prototypes
void init_lexical_analyzer(LexicalAnalyzer *la);
int is_whitespace(LexicalAnalyzer *la, char ch);
int is_letter(LexicalAnalyzer *la, char ch);
int is_digit(LexicalAnalyzer *la, char ch);
char peek_next_non_whitespace(LexicalAnalyzer *la, const char *code);
Token read_lexeme(LexicalAnalyzer *la, const char *code);
Token read_character(LexicalAnalyzer *la, const char *code);
Token read_string(LexicalAnalyzer *la, const char *code);
Token read_operator(LexicalAnalyzer *la, const char *code);
void skip_comment(LexicalAnalyzer *la, const char *code);
void tokenize(LexicalAnalyzer *la, const char *code);
void analyze(LexicalAnalyzer *la, const char *filename);
void push_token(LexicalAnalyzer *la, Token token);
void push_symbol(LexicalAnalyzer *la, const char *identifier);
void push_lexical_error(LexicalAnalyzer *la, const char *error);
int is_in_keywords(LexicalAnalyzer *la, const char *lexeme);
int is_in_operators(LexicalAnalyzer *la, const char *op);
void free_lexical_analyzer(LexicalAnalyzer *la);

// Initialize the LexicalAnalyzer structure
void init_lexical_analyzer(LexicalAnalyzer *la) {
    // Initialize keywords set (array of string literals)
    static const char *keywords_arr[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "int", "long", "register", "return", "short", "signed", "sizeof", "static",
        "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
    };
    la->keywords = keywords_arr;
    la->keywords_count = sizeof(keywords_arr) / sizeof(keywords_arr[0]);
    
    // Initialize operators set (array of string literals)
    static const char *operators_arr[] = {
        "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~",
        "+=", "-=", "*=", "/=", "%=", "==", "<=", ">=", "!=", "&&", "||",
        ">>=", "<<=", "++", "--"
    };
    la->operators = operators_arr;
    la->operators_count = sizeof(operators_arr) / sizeof(operators_arr[0]);
    
    // Initialize punctuation characters (as a string; including '.' here)
    la->punctuation = "(){},;[].";
    
    // Operator characters used for single-character check
    la->operator_chars = "+-*/%=<>!&|^~";
    
    // Initialize symbol table dynamic array
    la->symbol_table = NULL;
    la->symbol_table_count = 0;
    la->symbol_table_capacity = 0;
    
    // Initialize lexical errors dynamic array
    la->lexical_errors = NULL;
    la->lexical_errors_count = 0;
    la->lexical_errors_capacity = 0;
    
    // Initialize tokens dynamic array
    la->tokens = NULL;
    la->tokens_count = 0;
    la->tokens_capacity = 0;
    
    la->current_pos = 0;
    la->line_no = 1;
}

// Check if character is whitespace
int is_whitespace(LexicalAnalyzer *la, char ch) {
    // Return true if ch is in ' \t\n\r'
    return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
}

// Check if character is a letter
int is_letter(LexicalAnalyzer *la, char ch) {
    // Convert character to lowercase and check if between 'a' and 'z'
    char lower = tolower(ch);
    return (lower >= 'a' && lower <= 'z');
}

// Check if character is a digit
int is_digit(LexicalAnalyzer *la, char ch) {
    return (ch >= '0' && ch <= '9');
}

// Peek the next non-whitespace character in the code
char peek_next_non_whitespace(LexicalAnalyzer *la, const char *code) {
    int pos = la->current_pos + 1;
    int len = strlen(code);
    while (pos < len && is_whitespace(la, code[pos])) {
        pos++;
    }
    if (pos < len) {
        return code[pos];
    }
    return '\0'; // Return null char if none found
}

// Check if lexeme exists in keywords array
int is_in_keywords(LexicalAnalyzer *la, const char *lexeme) {
    for (int i = 0; i < la->keywords_count; i++) {
        if (strcmp(la->keywords[i], lexeme) == 0) {
            return 1;
        }
    }
    return 0;
}

// Check if given operator string exists in operators array
int is_in_operators(LexicalAnalyzer *la, const char *op) {
    for (int i = 0; i < la->operators_count; i++) {
        if (strcmp(la->operators[i], op) == 0) {
            return 1;
        }
    }
    return 0;
}

// Push a token into the tokens dynamic array
void push_token(LexicalAnalyzer *la, Token token) {
    if (la->tokens_count >= la->tokens_capacity) {
        la->tokens_capacity = la->tokens_capacity == 0 ? 10 : la->tokens_capacity * 2;
        la->tokens = realloc(la->tokens, la->tokens_capacity * sizeof(Token));
    }
    la->tokens[la->tokens_count++] = token;
}

// Push identifier into symbol table (avoid duplicates)
void push_symbol(LexicalAnalyzer *la, const char *identifier) {
    // Check if identifier already exists
    for (int i = 0; i < la->symbol_table_count; i++) {
        if (strcmp(la->symbol_table[i], identifier) == 0) {
            return;
        }
    }
    if (la->symbol_table_count >= la->symbol_table_capacity) {
        la->symbol_table_capacity = la->symbol_table_capacity == 0 ? 10 : la->symbol_table_capacity * 2;
        la->symbol_table = realloc(la->symbol_table, la->symbol_table_capacity * sizeof(char *));
    }
    la->symbol_table[la->symbol_table_count] = malloc(strlen(identifier) + 1);
    strcpy(la->symbol_table[la->symbol_table_count], identifier);
    la->symbol_table_count++;
}

// Push an error message into lexical_errors dynamic array
void push_lexical_error(LexicalAnalyzer *la, const char *error) {
    if (la->lexical_errors_count >= la->lexical_errors_capacity) {
        la->lexical_errors_capacity = la->lexical_errors_capacity == 0 ? 10 : la->lexical_errors_capacity * 2;
        la->lexical_errors = realloc(la->lexical_errors, la->lexical_errors_capacity * sizeof(char *));
    }
    la->lexical_errors[la->lexical_errors_count] = malloc(strlen(error) + 1);
    strcpy(la->lexical_errors[la->lexical_errors_count], error);
    la->lexical_errors_count++;
}

// Read a lexeme from the code
Token read_lexeme(LexicalAnalyzer *la, const char *code) {
    Token token;
    token.type[0] = '\0';
    token.value[0] = '\0';
    char lexeme[256] = "";
    int start_pos = la->current_pos;
    int len = strlen(code);
    
    // Read the entire lexeme
    while (la->current_pos < len && 
           !is_whitespace(la, code[la->current_pos]) &&
           strchr(la->operator_chars, code[la->current_pos]) == NULL &&
           strchr(la->punctuation, code[la->current_pos]) == NULL) {
        int l = strlen(lexeme);
        if(l < 255) { // prevent overflow
            lexeme[l] = code[la->current_pos];
            lexeme[l+1] = '\0';
        }
        la->current_pos++;
    }
    
    la->current_pos--; // Move back one position as the main loop will increment
    
    // Check if it's a keyword
    if (is_in_keywords(la, lexeme)) {
        strcpy(token.type, "Keyword");
        strcpy(token.value, lexeme);
        return token;
    }
    
    // Handle identifiers and invalid lexemes
    if (strlen(lexeme) > 0) {
        // Check if first character is letter or underscore
        if (is_letter(la, lexeme[0]) || lexeme[0] == '_') {
            int valid = 1;
            // Check if all other characters are valid
            for (int i = 1; i < strlen(lexeme); i++) {
                if (!(is_letter(la, lexeme[i]) || is_digit(la, lexeme[i]) || lexeme[i] == '_')) {
                    valid = 0;
                    break;
                }
            }
            if (valid) {
                // Check if it's followed by '(' to identify function
                char next_char = peek_next_non_whitespace(la, code);
                if (next_char != '(') {  // If not a function, add to symbol table
                    push_symbol(la, lexeme);
                }
                strcpy(token.type, "Identifier");
                strcpy(token.value, lexeme);
                return token;
            }
        }
        
        // If starts with digit, check if it is a valid number
        if (is_digit(la, lexeme[0])) {
            char *endptr;
            strtod(lexeme, &endptr);
            if (*endptr == '\0') {
                strcpy(token.type, "Constant");
                strcpy(token.value, lexeme);
                return token;
            }
        }
        
        // Invalid lexeme
        push_lexical_error(la, lexeme);
        // Return an empty token (type remains empty)
        return token;
    }
    
    return token;
}

// Read a character literal from the code
Token read_character(LexicalAnalyzer *la, const char *code) {
    Token token;
    strcpy(token.type, "String");
    char chr[256] = "";
    int index = 0;
    // Start with the opening quote
    chr[index++] = '\'';
    la->current_pos++;  // Skip the opening quote
    
    int len = strlen(code);
    while (la->current_pos < len) {
        char current_char = code[la->current_pos];
        if (index < 255) {
            chr[index++] = current_char;
            chr[index] = '\0';
        }
        
        if (current_char == '\'') {
            break;
        }
        la->current_pos++;
    }
    
    strcpy(token.value, chr);
    return token;
}

// Read a string literal from the code
Token read_string(LexicalAnalyzer *la, const char *code) {
    Token token;
    strcpy(token.type, "String");
    char str_val[256] = "";
    int index = 0;
    // Start with the opening quote
    str_val[index++] = '"';
    la->current_pos++;  // Skip the opening quote
    
    int len = strlen(code);
    while (la->current_pos < len) {
        char ch = code[la->current_pos];
        if (index < 255) {
            str_val[index++] = ch;
            str_val[index] = '\0';
        }
        
        if (ch == '"') {
            break;
        }
        la->current_pos++;
    }
    
    strcpy(token.value, str_val);
    return token;
}

// Read an operator from the code
Token read_operator(LexicalAnalyzer *la, const char *code) {
    Token token;
    strcpy(token.type, "Operator");
    char op[3] = "";
    int len = strlen(code);
    op[0] = code[la->current_pos];
    op[1] = '\0';
    int next_pos = la->current_pos + 1;
    
    if (next_pos < len) {
        char potential_operator[3] = "";
        potential_operator[0] = code[la->current_pos];
        potential_operator[1] = code[next_pos];
        potential_operator[2] = '\0';
        if (is_in_operators(la, potential_operator)) {
            strcpy(op, potential_operator);
            la->current_pos += 1;
        }
    }
    
    strcpy(token.value, op);
    return token;
}

// Skip comment in the code
void skip_comment(LexicalAnalyzer *la, const char *code) {
    int len = strlen(code);
    // If starts with '//' then single-line comment
    if (la->current_pos + 1 < len && code[la->current_pos] == '/' && code[la->current_pos + 1] == '/') {
        while (la->current_pos < len && code[la->current_pos] != '\n') {
            la->current_pos++;
        }
    }
    // Else if starts with '/*' then multi-line comment
    else if (la->current_pos + 1 < len && code[la->current_pos] == '/' && code[la->current_pos + 1] == '*') {
        la->current_pos += 2;
        while (la->current_pos < len - 1) {
            if (code[la->current_pos] == '\n') {
                la->line_no++;
            }
            if (code[la->current_pos] == '*' && code[la->current_pos + 1] == '/') {
                la->current_pos += 1;
                break;
            }
            la->current_pos++;
        }
    }
}

// Tokenize the input code
void tokenize(LexicalAnalyzer *la, const char *code) {
    // Reset tokens
    la->tokens_count = 0;
    la->current_pos = 0;
    int len = strlen(code);
    
    while (la->current_pos < len) {
        char ch = code[la->current_pos];
        
        // Handle whitespace
        if (is_whitespace(la, ch)) {
            if (ch == '\n') {
                la->line_no++;
            }
            la->current_pos++;
            continue;
        }
        
        // Handle comments
        if (ch == '/' && la->current_pos + 1 < len && 
            (code[la->current_pos + 1] == '/' || code[la->current_pos + 1] == '*')) {
            skip_comment(la, code);
            la->current_pos++;
            continue;
        }
        
        // Handle identifiers, keywords, and invalid lexemes
        if (is_letter(la, ch) || ch == '_' || is_digit(la, ch)) {
            Token token = read_lexeme(la, code);
            if (strlen(token.type) > 0) {
                push_token(la, token);
            }
        }
        // Handle strings
        else if (ch == '"') {
            Token token = read_string(la, code);
            push_token(la, token);
        }
        // Handle character literals
        else if (ch == '\'') {
            Token token = read_character(la, code);
            push_token(la, token);
        }
        // Handle operators
        else if (strchr(la->operator_chars, ch) != NULL) {
            Token token = read_operator(la, code);
            push_token(la, token);
        }
        // Handle punctuation (including dot operator)
        else if (strchr(la->punctuation, ch) != NULL) {
            Token token;
            strcpy(token.type, "Punctuation");
            token.value[0] = ch;
            token.value[1] = '\0';
            push_token(la, token);
        }
        la->current_pos++;
    }
}

// Analyze the file with the given filename
void analyze(LexicalAnalyzer *la, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file '%s'\n", filename);
        exit(1);
    }
    
    // Read entire file into a buffer
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);
    char *code = malloc(fsize + 1);
    if (code == NULL) {
        fclose(file);
        printf("Memory allocation error\n");
        exit(1);
    }
    size_t read_size = fread(code, 1, fsize, file);
    code[read_size] = '\0';
    fclose(file);
    
    // Tokenize the code
    tokenize(la, code);
    
    // Print tokens
    printf("TOKENS\n");
    for (int i = 0; i < la->tokens_count; i++) {
        printf("%s: %s\n", la->tokens[i].type, la->tokens[i].value);
    }
    
    // Print lexical errors
    if (la->lexical_errors_count > 0) {
        printf("\nLEXICAL ERRORS\n");
        for (int i = 0; i < la->lexical_errors_count; i++) {
            printf("%s invalid lexeme\n", la->lexical_errors[i]);
        }
    }
    
    // Print symbol table entries (sorted alphabetically)
    // Simple bubble sort for demonstration
    for (int i = 0; i < la->symbol_table_count - 1; i++) {
        for (int j = i + 1; j < la->symbol_table_count; j++) {
            if (strcmp(la->symbol_table[i], la->symbol_table[j]) > 0) {
                char *temp = la->symbol_table[i];
                la->symbol_table[i] = la->symbol_table[j];
                la->symbol_table[j] = temp;
            }
        }
    }
    
    printf("\nSYMBOL TABLE ENTRIES\n");
    for (int i = 0; i < la->symbol_table_count; i++) {
        printf("%d) %s\n", i + 1, la->symbol_table[i]);
    }
    
    free(code);
}

// Free dynamically allocated memory in LexicalAnalyzer
void free_lexical_analyzer(LexicalAnalyzer *la) {
    for (int i = 0; i < la->symbol_table_count; i++) {
        free(la->symbol_table[i]);
    }
    free(la->symbol_table);
    
    for (int i = 0; i < la->lexical_errors_count; i++) {
        free(la->lexical_errors[i]);
    }
    free(la->lexical_errors);
    
    free(la->tokens);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: python lexical_analyzer.py <input_file>\n");
        exit(1);
    }
    
    char file_path[512];
    // Construct file path as in original code
    snprintf(file_path, sizeof(file_path), "/workspaces/DLP-PRACTICALS/practical_3/testcases/%s", argv[1]);
    
    LexicalAnalyzer analyzer;
    init_lexical_analyzer(&analyzer);
    analyze(&analyzer, file_path);
    free_lexical_analyzer(&analyzer);
    return 0;
}
