#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Function to manage how a variable is printed (handles integer/float formatting)
void print_manager(char *var_name, FILE *c_file) {
    // Check if the variable's value is an integer or floating-point and print accordingly
    fprintf(c_file, "    if (floor(%s) == %s) {\n", var_name, var_name);
    fprintf(c_file, "        printf(\"%%.0f\\n\", %s);  // Print as integer\n", var_name);
    fprintf(c_file, "    } else {\n");
    fprintf(c_file, "        printf(\"%%.6f\\n\", %s);  // Print as floating-point\n", var_name);
    fprintf(c_file, "    }\n");
}

// Function to handle print statements
void handle_print_statement(char *line, FILE *c_file) {
    char *token = strtok(line, " ");
    if (strcmp(token, "print") == 0) {
        token = strtok(NULL, "\n");
        // Delegate the printing job to print_manager
        print_manager(token, c_file);  // Pass the variable to print_manager for formatting
    }
}

// Function to handle global variable declarations (with default value 0.0)
void declare_variable(char *var_name, FILE *c_file) {
    fprintf(c_file, "double %s = 0.0;\n", var_name);  // Declare globally with default value
}

// Function to handle variable assignments inside main
void assign_variable(char *line, FILE *c_file) {
    char var_name[100];
    double value;
    char expression[1024];

    // Handle numeric value assignment (e.g., x <- 1)
    if (sscanf(line, "%s <- %lf", var_name, &value) == 2) {
        // Check if the value is an integer or a floating-point number
        if (floor(value) == value) {
            // Assign integer value
            fprintf(c_file, "    %s = %.0f;\n", var_name, value);  // No decimals for integers
        } else {
            // Assign floating-point value with 6 decimal places
            fprintf(c_file, "    %s = %.6f;\n", var_name, value);
        }
    }
    // Handle expression assignment (e.g., z <- x + y)
    else if (sscanf(line, "%s <- %[^\n]", var_name, expression) == 2) {
        // Copy the expression (but do not print)
        fprintf(c_file, "    %s = %s;\n", var_name, expression);
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

    // Read the next line for the return statement
    fgets(line, 1024, ml_file);
    char *token = strtok(line, " \t");

    if (strcmp(token, "print") == 0) {
    token = strtok(NULL, "\n");
    fprintf(c_file, "    printf(%s);\n", token);
    }

    if (strcmp(token, "return") == 0) {
        token = strtok(NULL, "\n");
        fprintf(c_file, "    return %s;\n", token);
    }
    fprintf(c_file, "}\n\n");
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

// The main compiler function
void compiler(FILE *ml_file, FILE *c_file) {
    char line[1024];
    char var_name[100];
    if (contains_function(ml_file)){
        if (!ml_file || !c_file) {
            perror("Error opening file");
            return;
        }

        // First pass: Declare variables globally with default value
        fprintf(c_file, "#include <stdio.h>\n\n");
        fprintf(c_file, "#include <math.h>\n");
        
        while (fgets(line, sizeof(line), ml_file)) {
            if (strstr(line, "<-")) {
                // Extract variable name and declare it with default value
                sscanf(line, "%s <-", var_name);
                declare_variable(var_name, c_file);  // Declare with 0.0
            }
        }

        // Rewind the file pointer to start from the beginning again
        rewind(ml_file);

        while (fgets(line, sizeof(line), ml_file)) {
        char *token = strtok(line, " ");
        if (strcmp(token, "function") == 0) {
            handle_function_definition(line, ml_file, c_file);
        }
    }
 }

    // Start the main function
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
            assign_variable(line, c_file);
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