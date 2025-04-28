#include <stdlib.h>

#include "kriolu.h"

#define RETURN_DEFER(value) \
    do                      \
    {                       \
        result = (value);   \
        goto defer;         \
    } while (0)

void print_usage();
int  file_read(const char* file_path, char** buffer_out);
bool filename_ends_with(const char* filename, const char* extension);
void token_print(Token token);

int main(int argc, const char* argv[]) {
    const char* source_code = NULL;

    if (argc < 2) {
        fprintf(stderr, "Error: few arguments to run.\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    bool is_info = false;
    bool is_help = false;
    bool is_file = false;
    if (strcmp(argv[1], "info") == 0)  is_info = true;
    else if (strcmp(argv[1], "djudan") == 0) is_help = true;
    else if (filename_ends_with(argv[1], ".k")) is_file = true;


    if (is_info) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("Kriolu: version 1.0.0 development mode");
#else 
        printf("Kriolu: version 1.0.0 release mode");
#endif
        return 0;
    }

    if (is_help) {
        print_usage();
        return 0;
    }

    // 
    // Is a file section
    //

    if (!filename_ends_with(argv[1], ".k")) {
        fprintf(stderr, "Error: file extension not supported.\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    int result = file_read(argv[1], &source_code);
    if (result <= 0) exit(EXIT_FAILURE);

    bool is_flag_lexer = false;
    bool is_flag_parser = false;
    bool is_flag_bytecode = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-lexer") == 0) is_flag_lexer = true;
        else if (strcmp(argv[i], "-parser") == 0) is_flag_parser = true;
        else if (strcmp(argv[i], "-bytecode") == 0) is_flag_bytecode = true;
    }


    if (is_flag_lexer) {
        Lexer lexer;
        lexer_init(&lexer, source_code);
        lexer_debug_dump_tokens(&lexer);
        return 0;
    }

    if (is_flag_parser) {
        Parser parser;
        Bytecode bytecode;
        parser_init(&parser, source_code);

        // bytecode_emitter_begin();
        ArrayStatement* statements = parser_parse(&parser);
        // bytecode = bytecode_emitter_end();

        printf("\n");
        for (int i = 0; i < statements->count; i++) {
            statement_print(&statements->items[i], 0);
            printf("\n");

            // expression_print(statements->items[i].expression, 0);
            // printf("\n");
            // expression_print_tree(statements->items[i].expression, 0);
            // expression_free(statements->items[i].expression);
        }

        bytecode_free(&bytecode);

        return 0;
    }

    if (is_flag_bytecode) {
        Parser parser;
        Bytecode bytecode;
        parser_init(&parser, source_code);

        bytecode_emitter_begin();
        parser_parse(&parser);
        bytecode = bytecode_emitter_end();

        bytecode_disassemble(&bytecode, "Bytecode");
        bytecode_free(&bytecode);

        return 0;
    }


    Parser parser;
    Bytecode bytecode;
    parser_init(&parser, source_code);
    vm_init();

    bytecode_emitter_begin();
    parser_parse(&parser);
    bytecode = bytecode_emitter_end();

    vm_interpret(&bytecode);

    bytecode_free(&bytecode);
    vm_free();

    return 0;
}

int file_read(const char* file_path, char** buffer_out)
{
    int result = 0;
    FILE* file = fopen(file_path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Error: Could not open file \"%s\".\n", file_path);
        RETURN_DEFER(-1);
    }

    // Get the file size.
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    if (file_size == 0)
    {
        fprintf(stderr, "Error: File is empty.\n");
        RETURN_DEFER(file_size);
    }

    *buffer_out = malloc(file_size + 1);
    if (*buffer_out == NULL)
    {
        fprintf(stderr, "Error: Could not allocate buffer for the file size.\n");
        RETURN_DEFER(-1);
    }

    size_t bytes_read = fread(*buffer_out, sizeof(char), file_size, file);
    if (bytes_read < file_size)
    {
        fprintf(stderr, "Error: reading contents of the file failed.\n");
        RETURN_DEFER(-1);
    }

    (*buffer_out)[bytes_read] = '\0';
    result = bytes_read;

defer:
    if (file) fclose(file);
    if (*buffer_out != NULL && result == -1) free(*buffer_out);
    return result;
}

bool filename_ends_with(const char* filename, const char* extension) {
    int filename_length = strlen(filename);
    int extension_length = strlen(extension);

    for (int i = 0; i < extension_length; i++) {
        if (filename[filename_length - i - 1] != extension[extension_length - i - 1])
            return false;
    }

    return true;
}

void print_usage()
{
    printf("Kriolu interpreter v1.0.0\n\n");
    printf("Usage:\n");
    printf("  kriolu <filename.k|info|djudan> [optional flags]\n\n");
    printf("Optional flags:\n");
    printf("  -lexer                   Sends tokens to the stdout.\n");
    printf("  -parser                  Sends AST to the stdout.\n");
    printf("  -bytecode                Sends bytecodes to the stdout.\n");
}
