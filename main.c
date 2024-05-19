#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Feature flags
#define ENABLE_ENCODING
#define ENABLE_LEXER
#define ENABLE_PARSER
#define ENABLE_COMPILER

// Encoding management
typedef enum {
    ENC_UTF8,
    ENC_UTF16,
    ENC_UTF32,
    ENC_BAUDOT,
    ENC_CUSTOM
} EncodingType;

typedef struct {
    EncodingType type;
    char* custom_encoding_file;
} EncodingInfo;

#ifdef ENABLE_ENCODING
char* decode_source_code(const char* source_code, EncodingInfo* encoding_info);
void free_decoded_source_code(char* decoded_source_code);
#endif

// Preprocessor directives
typedef struct {
    bool typeless;
    bool lineaware;
    bool nosemicolons;
    EncodingInfo encoding_info;
} PreprocessorDirectives;

#ifdef ENABLE_LEXER
// Lexer inline has supporting .h and .c
typedef enum {
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_KEYWORD,
    TOK_OPERATOR,
    TOK_PUNCTUATOR,
    TOK_EOF,
} TokenType;

typedef struct {
    TokenType type;
    char* value;
} Token;

Token* lex_source_code(char* decoded_source_code, PreprocessorDirectives* directives);
void free_tokens(Token* tokens);
#endif

#ifdef ENABLE_PARSER
// Parser
typedef struct Node Node;

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_STATEMENT,
    NODE_EXPRESSION,
} NodeType;

struct Node {
    NodeType type;
    Node* left;
    Node* right;
};

Node* parse_source_code(Token* tokens, PreprocessorDirectives* directives);
void free_ast(Node* root);
#endif

#ifdef ENABLE_COMPILER
// Compiler
void compile_source_code(Node* ast_root, PreprocessorDirectives* directives);
void traverse_and_generate_code(Node* node);
#endif

void parse_preprocessor_directives(const char* source_code, PreprocessorDirectives* directives);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    // Load source code file
    FILE* source_file = fopen(argv[1], "r");
    if (!source_file) {
        printf("Error: Could not open source file %s\n", argv[1]);
        return 1;
    }

    fseek(source_file, 0, SEEK_END);
    long source_code_size = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);

    char* source_code = malloc(source_code_size + 1);
    fread(source_code, 1, source_code_size, source_file);
    source_code[source_code_size] = '\0';

    fclose(source_file);

    // Parse preprocessor directives
    PreprocessorDirectives directives = {0};
    parse_preprocessor_directives(source_code, &directives);

#ifdef ENABLE_ENCODING
    // Decode source code based on encoding
    char* decoded_source_code = decode_source_code(source_code, &directives.encoding_info);
    free(source_code);

    if (!decoded_source_code) {
        printf("Error: Could not decode source code\n");
        return 1;
    }
#else
    char* decoded_source_code = source_code;
#endif

#ifdef ENABLE_LEXER
    // Lexing inline but does have supporting .h and .c for intial boot strap same for parser
    Token* tokens = lex_source_code(decoded_source_code, &directives);
    if (!tokens) {
        printf("Error: Could not tokenize source code\n");
        free_decoded_source_code(decoded_source_code);
        return 1;
    }
#endif

#ifdef ENABLE_PARSER
    // Parsing
    Node* ast_root = parse_source_code(tokens, &directives);
    free_tokens(tokens);
    if (!ast_root) {
        printf("Error: Could not parse source code\n");
        free_decoded_source_code(decoded_source_code);
        return 1;
    }
#else
    Node* ast_root = NULL;
#endif

#ifdef ENABLE_COMPILER
    // Compiling
    compile_source_code(ast_root, &directives);
    free_ast(ast_root);
#endif

#ifdef ENABLE_ENCODING
    free_decoded_source_code(decoded_source_code);
#else
    free(source_code);
#endif

    return 0;
}

// Parse preprocessor directives incomplete
void parse_preprocessor_directives(const char* source_code, PreprocessorDirectives* directives) {
    const char* start = source_code;
    const char* end;

    while ((start = strstr(start, "#")) != NULL) {
        start++;
        if (strncmp(start, "typeless", 8) == 0) {
            start += 8;
            if (*start == '=') {
                start++;
                directives->typeless = (*start == 't' || *start == 'T');
            }
        } else if (strncmp(start, "lineaware", 9) == 0) {
            start += 9;
            if (*start == ' ') {
                start++;
                directives->lineaware = (*start == 't' || *start == 'T');
            }
        } else if (strncmp(start, "nosemicolons", 12) == 0) {
            start += 12;
            if (*start == ' ') {
                start++;
                directives->nosemicolons = (*start == 't' || *start == 'T');
            }
        } else if (strncmp(start, "encoding", 8) == 0) {
            start += 8;
            if (*start == '(') {
                start++;
                if (strncmp(start, "true", 4) == 0) {
                    start += 4;
                    if (*start == ',') {
                        start++;
                        end = strchr(start, ')');
                        if (end) {
                            size_t encoding_length = end - start;
                            char* encoding = malloc(encoding_length + 1);
                            strncpy(encoding, start, encoding_length);
                            encoding[encoding_length] = '\0';
                            if (strcmp(encoding, "UTF-8") == 0) {
                                directives->encoding_info.type = ENC_UTF8;
                            } else if (strcmp(encoding, "UTF-16") == 0) {
                                directives->encoding_info.type = ENC_UTF16;
                            } else if (strcmp(encoding, "UTF-32") == 0) {
                                directives->encoding_info.type = ENC_UTF32;
                            } else if (strcmp(encoding, "Baudot") == 0) {
                                directives->encoding_info.type = ENC_BAUDOT;
                            } else if (strcmp(encoding, "Custom") == 0) {
                                directives->encoding_info.type = ENC_CUSTOM;
                                // Handle custom encoding file needed
                            }
                            free(encoding);
                        }
                    }
                }
            }
        }
        start = end;
    }
}

#ifdef ENABLE_ENCODING
// Decode source code based on encoding
char* decode_source_code(const char* source_code, EncodingInfo* encoding_info) {
    switch (encoding_info->type) {
        case ENC_UTF8:
            return strdup(source_code);
        case ENC_UTF16:
            // Implement UTF-16 decoding
            break;
        case ENC_UTF32:
            // Implement UTF-32 decoding
            break;
        case ENC_BAUDOT:
            // Implement Baudot/ITA2 TBD decoding
            break;
        case ENC_CUSTOM:
            // Implement custom encoding decoding
            break;
    }
    return NULL;
}

// Free decoded source code
void free_decoded_source_code(char* decoded_source_code) {
    free(decoded_source_code);
}
#endif

#ifdef ENABLE_LEXER
// Lexical analysis
Token* lex_source_code(char* decoded_source_code, PreprocessorDirectives* directives) {
    Token* tokens = NULL;
    int token_count = 0;
    int token_capacity = 0;

    char* current = decoded_source_code;
    while (*current) {
        // Skip whitespace
        while (isspace(*current)) {
            current++;
        }

        // Handle identifiers and keywords
        if (isalpha(*current)) {
            int start = current - decoded_source_code;
            while (isalnum(*current)) {
                current++;
            }
            int length = current - (decoded_source_code + start);
            Token token = {TOK_IDENT, NULL};
            token.value = malloc(length + 1);
            strncpy(token.value, decoded_source_code + start, length);
            token.value[length] = '\0';

            // Check if the token is a keyword
            if (strcmp(token.value, "if") == 0 || strcmp(token.value, "else") == 0 || 
                strcmp(token.value, "while") == 0 || strcmp(token.value, "return") == 0) {
                token.type = TOK_KEYWORD;
            }

            // Add the token to the list
            if (token_count >= token_capacity) {
                token_capacity = (token_capacity + 1) * 2;
                tokens = realloc(tokens, token_capacity * sizeof(Token));
            }
            tokens[token_count++] = token;
        }
        // Handle numbers
        else if (isdigit(*current)) {
            int start = current - decoded_source_code;
            while (isdigit(*current)) {
                current++;
            }
            int length = current - (decoded_source_code + start);
            Token token = {TOK_NUMBER, NULL};
            token.value = malloc(length + 1);
            strncpy(token.value, decoded_source_code + start, length);
            token.value[length] = '\0';

            // Add the token to the list
            if (token_count >= token_capacity) {
                token_capacity = (token_capacity + 1) * 2;
                tokens = realloc(tokens, token_capacity * sizeof(Token));
            }
            tokens[token_count++] = token;
        }
        // Handle strings
        else if (*current == '"' || *current == '\'') {
            char quote = *current;
            current++;
            int start = current - decoded_source_code;
            while (*current && *current != quote) {
                current++;
            }
            int length = current - (decoded_source_code + start);
            Token token = {TOK_STRING, NULL};
            token.value = malloc(length + 1);
            strncpy(token.value, decoded_source_code + start, length);
            token.value[length] = '\0';
            if (*current == quote) {
                current++;
            }

            // Add the token to the list
            if (token_count >= token_capacity) {
                token_capacity = (token_capacity + 1) * 2;
                tokens = realloc(tokens, token_capacity * sizeof(Token));
            }
            tokens[token_count++] = token;
        }
        // Handle operators and punctuators
        else if (ispunct(*current)) {
            Token token = {TOK_OPERATOR, NULL};
            token.value = malloc(2);
            token.value[0] = *current;
            token.value[1] = '\0';

            // Check for multi-character operators
            if ((*current == '&' && *(current + 1) == '&') || 
                (*current == '|' && *(current + 1) == '|') ||
                (*current == '=' && *(current + 1) == '=') ||
                (*current == '!' && *(current + 1) == '=')) {
                token.value[1] = *(++current);
                token.value[2] = '\0';
            }
            current++;

            // Add the token to the list
            if (token_count >= token_capacity) {
                token_capacity = (token_capacity + 1) * 2;
                tokens = realloc(tokens, token_capacity * sizeof(Token));
            }
            tokens[token_count++] = token;
        }
        // Handle unknown characters
        else {
            current++;
        }
    }

    // Add a sentinel token
    Token sentinel = {TOK_EOF, NULL};
    if (token_count >= token_capacity) {
        token_capacity++;
        tokens = realloc(tokens, token_capacity * sizeof(Token));
    }
    tokens[token_count++] = sentinel;

    return tokens;
}

// Free tokens
void free_tokens(Token* tokens) {
    Token* current = tokens;
    while (current->value) {
        free(current->value);
        current++;
    }
    free(tokens);
}
#endif

#ifdef ENABLE_PARSER
// Parsing
Node* parse_source_code(Token* tokens, PreprocessorDirectives* directives) {
    // Implement parsing of the tokens into an AST
    // Example implementation (placeholder)
    Node* root = malloc(sizeof(Node));
    root->type = NODE_PROGRAM;
    // Parse tokens into AST nodes
    return root;
}

// Free AST nodes
void free_ast(Node* root) {
    // Implement freeing of the AST nodes
    free(root);
}
#endif

#ifdef ENABLE_COMPILER
// Helper function to traverse the AST and generate machine code
void traverse_and_generate_code(Node* node) {
    if (!node) return;

    // Recursively traverse the left and right children (if any)
    traverse_and_generate_code(node->left);
    traverse_and_generate_code(node->right);

    // Generate code based on the node type
    switch (node->type) {
        case NODE_PROGRAM:
            printf("Generating code for program node...\n");
            // Add code generation logic for the program node
            break;
        case NODE_FUNCTION:
            printf("Generating code for function node...\n");
            // Add code generation logic for the function node
            break;
        case NODE_STATEMENT:
            printf("Generating code for statement node...\n");
            // Add code generation logic for the statement node
            break;
        case NODE_EXPRESSION:
            printf("Generating code for expression node...\n");
            // Add code generation logic for the expression node
            break;
        // Add cases for more node types as needed
        default:
            fprintf(stderr, "Error: Unknown node type %d\n", node->type);
            exit(EXIT_FAILURE);
    }
}

// Compilation
void compile_source_code(Node* ast_root, PreprocessorDirectives* directives) {
    if (ast_root) {
        // Traverse the AST and generate machine code
        traverse_and_generate_code(ast_root);
    }
}
#endif

// strdup implementation  needed
#ifndef _MSC_VER
char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}
#endif
