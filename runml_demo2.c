#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void compiler(FILE *ml_file, FILE *c_file){
    if (!ml_file || !c_file) {
        perror("Error opening file");
        return;
    }

    // Writing the headers and main function declaration to the C file
    fprintf(c_file, "#include <stdio.h>\n\nint main() {\n");

    char line[1024];
    char var_name[100];
    double value;
    //char *token = strtok(line, " ");
    // Read the entire line from the ml file
    while (fgets(line, sizeof(line), ml_file)) {
        // Skip any lines that are comments or empty
        if (line[0] == '#' || strlen(line) < 3) continue;

        // Parse the assignment assuming no comments in the line
        if (sscanf(line, "%s <- %lf", var_name, &value) == 2) {
            fprintf(c_file, "    double %s = %.6f;\n", var_name, value);
        }


        //Handling the print statement
        char *token = strtok(line, " ");
        if (strcmp(token, "print") == 0) {
            // Handle print statement
            token = strtok(NULL, "\n");
            fprintf(c_file, "    printf(\"%%.6f\\n\", %s);\n", token);
        }
    }

    fprintf(c_file, "\n    return 0;\n}\n"); // Close the main function
}


int main(int argc, char *argv[]) {
    //FILE *ml_file = fopen("input.txt", "r");
    //FILE *c_file = fopen("output.c", "w");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.ml>\n", argv[0]);
        return 1;
    }

    FILE *ml_file = fopen(argv[1], "r");
    if (ml_file == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
        return 1;
    }

    FILE *c_file = fopen("output.c", "w");
    if (c_file == NULL) {
        fprintf(stderr, "Error: Cannot create output file\n");
        fclose(ml_file);
        return 1;
    }

    compiler(ml_file, c_file);

    fclose(ml_file);
    fclose(c_file);

    printf("Transpilation complete. 'output.c' generated.\n");

    return 0;
}
