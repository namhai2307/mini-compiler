#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void compiler(FILE *ml_file, FILE *c_file){
    if (!ml_file || !c_file) {
        perror("Error opening file");
        return;
    }

    // Writing the headers and main function declaration to the C file
    char line[1024];
    char function_name[1024];
    fprintf(c_file, "#include <stdio.h>\n\n");

    // First pass: Handle function definitions
    while (fgets(line, sizeof(line), ml_file)) {
        char *token = strtok(line, " ");
        if (strcmp(token, "function") == 0) {
            // Parse the function name
            strcpy(function_name, strtok(NULL, " "));

            // Initialize an empty string to hold the parameters for the C function
            char parameters[256] = {0};
            char *arg = strtok(NULL, " \n");

            // Collect all function arguments
            while (arg) {
                strcat(parameters, "double ");
                strcat(parameters, arg);
                arg = strtok(NULL, " \n");
                if (arg) {
                    strcat(parameters, ", ");
                }
            }

            // Write the function definition
            fprintf(c_file, "double %s(%s) {\n", function_name, parameters);

            //write content of the function
            rewind(ml_file);
            fgets(line, sizeof(line), ml_file);
                char *token = strtok(line, " ");
                if (strcmp(token, "function") == 0){
                    continue;
                }
                //parsing the equal symbol
                char var_name[100];
                double value;
                if (sscanf(line, "%s <- %lf", var_name, &value) == 2) {
                    if (floor(value) == value) {
                        fprintf(c_file, "    double %s = %.0f;\n", var_name, value);
                    } else {
                        fprintf(c_file, "    double %s = %.6f;\n", var_name, value);
                    }
                }

                if (strcmp(token, "return") == 0){
                    token = strtok(NULL, "\n");
                    fprintf(c_file, "    return(\"%%.6f\\n\", %s);\n", token);
                    //fprintf(c_file, "   return ;");
                }
            }
            // Read the next line for the return statement
            fprintf(c_file, "}\n\n");
        
    }

    fprintf(c_file, "int main() {\n");

    // Rewind the file pointer to start from the beginning again
    rewind(ml_file);

    // Second pass: Handle variable assignments and print statements
    while (fgets(line, sizeof(line), ml_file)) {
        // Skip any lines that are comments or empty
        if (line[0] == '#' || strlen(line) < 3) continue;

        // Parse the assignment assuming no comments in the line
        char var_name[100];
        double value;

        if (sscanf(line, "%s <- %lf", var_name, &value) == 2) {
            if (floor(value) == value) {
                fprintf(c_file, "    double %s = %.0f;\n", var_name, value);
            } else {
                fprintf(c_file, "    double %s = %.6f;\n", var_name, value);
            }
        } else {
            // Handling the print statement
            char *token = strtok(line, " ");
            if (strcmp(token, "print") == 0) {
                token = strtok(NULL, "\n");
                fprintf(c_file, "    printf(\"%%.6f\\n\", %s);\n", token);
            }
        }
    }

    fprintf(c_file, "\n    return 0;\n}\n"); // Close the main function
}

int main(int argc, char *argv[]) {
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
