#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define RETURN_DEFER(value) \
    do                      \
    {                       \
        result = (value);   \
        goto defer;         \
    } while (0)

void print_usage();
int file_read(const char *file_path, char **buffer_out);
bool filename_ends_with(const char *filename, const char *extension);

int main(int argc, const char *argv[])
{
    char *source_code;

    if (argc < 2)
    {
        fprintf(stderr, "Error: few arguments to run.\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if (!filename_ends_with(argv[1], ".k"))
    {
        fprintf(stderr, "Error: file extension not supported.\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    int result = file_read(argv[1], &source_code);
    if (result < 0)
        exit(EXIT_FAILURE);

    bool is_flag_lexer = false;
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-lexer") == 0)
        {
            is_flag_lexer = true;
        }
    }

    return 0;
}

int file_read(const char *file_path, char **buffer_out)
{
    int result = 0;
    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Error: Could not open file \"%s\".\n", file_path);
        RETURN_DEFER(-1);
    }

    // Get the file size.
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

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
    if (file)
        fclose(file);
    if (buffer_out != NULL && result == -1)
        free(*buffer_out);

    return result;
}

bool filename_ends_with(const char *filename, const char *extension)
{
    int filename_length = strlen(filename);
    int extension_length = strlen(extension);

    for (int i = 0; i < extension_length; i++)
    {
        if (filename[filename_length - i - 1] != extension[extension_length - i - 1])
            return false;
    }

    return true;
}

void print_usage()
{
    printf("Kriolu interpreter v0.0.1\n\n");
    printf("Usage:\n");
    printf("  kriolu <filename.k> [optional flags]>\n\n");
    printf("Optional flags:\n");
    printf("  -lexer                   Sends tokens to the stdout.\n");
    printf("  -parser                  Sends bytecodes to the stdout.\n");
}