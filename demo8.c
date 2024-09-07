#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

// Function to handle print statements
void handle_print_statement(char *line, FILE *c_file) {
    char *token = strtok(line, " ");
    if (strcmp(token, "print") == 0) {
        token = strtok(NULL, "\n");
        fprintf(c_file, "    printf(\"%%.6f\\n\", %s);\n", token);
    }
}

// Function to handle variable assignments (variable <- value)
void handle_variable_assignment_v1(char *line, FILE *c_file) {
    char var_name[100];
    double value;
    char expression[1024];

    // Handle numeric value assignment (e.g., x <- 1)
    if (sscanf(line, "%s <- %lf", var_name, &value) == 2) {
        // Check if it's an integer or a floating-point number
        if (floor(value) == value) {
            fprintf(c_file, "double %s = %.0f;\n", var_name, value);
        } else {
            fprintf(c_file, "double %s = %.6f;\n", var_name, value);
        }
    }
}

// Function to handle variable assignments (variable <- variable +- variable)
void handle_variable_assignment_v2(char *line, FILE *c_file) {
    char var_name[100];
    double value;
    char expression[1024];
    // Handle expression assignment (e.g., z <- a + b)
    if (sscanf(line, "%s <- %[^\n]", var_name, expression) == 2) {
        fprintf(c_file, "double %s = %s;\n", var_name, expression);
    }
}


// Function to handle function definitions
void handle_function_definition(char *line, FILE *ml_file, FILE *c_file) {
    char function_name[1024];
    char parameters[256] = {0};

    // Parse the function name
    strcpy(function_name, strtok(NULL, " "));

    // Collect all function arguments
    char *arg = strtok(NULL, " \n");
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

    // Process the function body
    while (fgets(line, 1024, ml_file)) {
        if (line[0] != '\t') break;  // Exit when indentation ends (i.e., no tab at the start)

        // Inside the function, handle statements
        if (strstr(line, "print")) {
            handle_print_statement(line, c_file);
        } else if (strstr(line, "<-")) {
            handle_variable_assignment_v2(line, c_file);
        } else if (strstr(line, "return")) {
            char *token = strtok(line, " \t\n");
            token = strtok(NULL, "\n");
            fprintf(c_file, "    return %s;\n", token);
        }
    }

    fprintf(c_file, "}\n\n");  // Close the function
}


// Function to check if there's a function keyword in the file
int contains_function(FILE *ml_file) {
    char line[1024];
    while (fgets(line, sizeof(line), ml_file)) {
        if (strstr(line, "function")) {
            rewind(ml_file);  // Rewind the file pointer to the beginning
            return 1;         // "function" found
        }
    }
    rewind(ml_file);          // Rewind the file pointer to the beginning if not found
    return 0;                 // "function" not found
}


//
// The main compiler function
void compiler(FILE *ml_file, FILE *c_file) {
    char line[1024];


    if (contains_function(ml_file)){
        if (!ml_file || !c_file) {
            perror("Error opening file");
            return;
        }

        // Writing the headers to the C file
        fprintf(c_file, "#include <stdio.h>\n\n");

        // First pass: Handle variable assignments before functions
        while (fgets(line, sizeof(line), ml_file)) {
            if (strstr(line, "<-")) {
                handle_variable_assignment_v1(line, c_file);
            }
        }

        // Rewind the file pointer to start from the beginning again
        rewind(ml_file);

        // Handle function definitions
        while (fgets(line, sizeof(line), ml_file)) {
            char *token = strtok(line, " ");
            if (strcmp(token, "function") == 0) {
                handle_function_definition(line, ml_file, c_file);
            }
        }
        
        fprintf(c_file, "int main() {\n");

        // Rewind the file pointer to start from the beginning again
        rewind(ml_file);

        // Second pass: Handle print statements and remaining variable assignments
        while (fgets(line, sizeof(line), ml_file)) {
        // Skip any lines that are comments or empty
        if (line[0] == '#' || strlen(line) < 3) continue;

        //skip any line inside of a function in the ml code
        if (isspace(line[0]) || line[0] == '\t') continue;

        // Handle print statements
        if (strstr(line, "print")) {
            handle_print_statement(line, c_file);
        }

        //handle assignment
        if (strstr(line, "<-")) {
            //fprintf(c_file, "\t");
            handle_variable_assignment_v2(line, c_file);
        }
    }
    fprintf(c_file, "\n    return 0;\n}\n"); // Close the main function
    }

//

    else{
        // Start the main function
        fprintf(c_file, "#include <stdio.h>\n");
        fprintf(c_file, "int main() {\n");

        // Rewind the file pointer to start from the beginning again
        rewind(ml_file);

        // Second pass: Handle print statements and remaining variable assignments
        while (fgets(line, sizeof(line), ml_file)) {
            // Skip any lines that are comments or empty
            if (line[0] == '#' || strlen(line) < 3) continue;

            // Handle print statements
            if (strstr(line, "print")) {
                handle_print_statement(line, c_file);
            }

            //handle assignment
            if (strstr(line, "<-")) {
                //fprintf(c_file, "\t");
                handle_variable_assignment_v1(line, c_file);
                handle_variable_assignment_v2(line, c_file);
            }

        }
        fprintf(c_file, "\n    return 0;\n}\n"); // Close the main function
    }
}

//

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


    system("gcc -o output output.c");
    system("output.exe");

    printf("Transpilation complete. 'output.c' generated.\n");

    return 0;
}
