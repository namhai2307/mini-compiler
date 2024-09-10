#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function to check for incomplete assignment errors
void check_assignment_error(char *line, int line_number, FILE *stderr) {
    if (strstr(line, "<-")) {
        char var_name[100], value[100];
        // Check if the line contains both a variable and a value
        if (sscanf(line, "%s <- %s", var_name, value) != 2) {
            fprintf(stderr, "! Error: Incomplete assignment on line %d\n", line_number);
        }
    }
}

// Function to check for incomplete or invalid print statements
void check_print_error(char *line, int line_number, FILE *stderr) {
    if (strstr(line, "print")) {
        char var_to_print[100];
        // Check if there is something to print after 'print'
        if (sscanf(line, "print %s", var_to_print) != 1) {
            fprintf(stderr, "! Error: Incomplete print statement on line %d\n", line_number);
        }
    }
}

// Function to check for unrecognized commands
void check_unrecognized_command(char *line, int line_number, FILE *stderr) {
    char command[100];
    // Extract the first word of the line to identify the command
    sscanf(line, "%s", command);

    // List of recognized commands (print, assignment, function)
    if (strcmp(command, "print") != 0 && !strstr(line, "<-") && strcmp(command, "function") != 0) {
        fprintf(stderr, "! Error: Unrecognized command on line %d\n", line_number);
    }
}

// Function to handle all error checks for a line of ml code
void check_syntax_errors(char *line, int line_number, FILE *stderr) {
    // Check for assignment errors
    check_assignment_error(line, line_number, stderr);

    // Check for print errors
    check_print_error(line, line_number, stderr);

    // Check for unrecognized commands
    check_unrecognized_command(line, line_number, stderr);
}

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
    char *token = strtok(line, " \t");
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
        fprintf(c_file, "    %s = %.6f;\n", var_name, value);
    }
    // Handle expression assignment (e.g., z <- x + y)
    else if (sscanf(line, "%s <- %[^\n]", var_name, expression) == 2) {
        // Copy the expression (but do not print)
        fprintf(c_file, "    %s = %s;\n", var_name, expression);
    }
}

// Function to handle variable assignments (variable <- value) (for no function case ONLY)
void handle_variable_assignment_v1(char *line, FILE *c_file) {
    char var_name[100];
    double value;

    // Handle numeric value assignment (e.g., x <- 1)
    if (sscanf(line, "%s <- %lf", var_name, &value) == 2) {
        fprintf(c_file, "double %s = %.6f;\n", var_name, value);
    }
}


// Function to handle variable assignments (variable <- variable +-*/ variable) (for no function case ONLY)
void handle_variable_assignment_v2(char *line, FILE *c_file) {
    char var_name[100];
    char expression[1024];
    // Handle expression assignment (e.g., z <- a + b)
    if (sscanf(line, "%s <- %[^\n]", var_name, expression) == 2) {
        fprintf(c_file, "double %s = %s;\n", var_name, expression);
    }
}

// Handle function recall (i.e., function calls with parentheses)
void handle_function_recall(char *line, FILE *c_file) {
    // Check if the line contains a function call (look for '(' and ')' character)
    if (strchr(line, '(') && strchr(line, ')') && !strchr(line, '+') && !strchr(line, '-') && !strchr(line, '*') && !strchr(line, '/')) {  // strchr checks for the '(' character
        fprintf(c_file, "    %s;\n", line);
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
        if (strstr(line, "<-")) {
            assign_variable(line, c_file);
        } else if (strstr(line, "return")) {
            char *token = strtok(line, " \t\n");
            token = strtok(NULL, "\n");
            fprintf(c_file, "    return %s;\n", token);
        } else if (strstr(line, "print")){
            handle_print_statement(line, c_file);
        }
    }

    fprintf(c_file, "}\n\n");  // Close the function
}

// Function to check if there's a "function" keyword in the file
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
    int line_number = 1; // Track the line number for error reporting
    int error_detected = 0;  // Flag to indicate if an error was detected
    int declare_count = 0; //count for keeping track of the declaration time(avoid declaring a variable twice or more)
    char variable_name[50][50];
    int declare_index = 0;
    int is_duplicate = 0;
    if (contains_function(ml_file)){
        if (!ml_file || !c_file) {
            perror("Error opening file");
            return;
        }

        // First pass: Check syntax errors
        fprintf(c_file, "#include <stdio.h>\n");
        fprintf(c_file, "#include <math.h>\n");
        
    while (fgets(line, sizeof(line), ml_file)) {
        // Skip empty lines and comments
        if (line[0] == '#' || strlen(line) <= 1) {
            line_number++;
            continue;  // Ignore comment lines and empty lines
        }

        check_syntax_errors(line, line_number, stderr);

        // If an error is found, set the error flag
        if (strstr(line, "Error")) {
            error_detected = 1;
        }
        
        if (strstr(line, "<-") && declare_count == 0) {
            
            // Extract variable name from the line
            sscanf(line, "%s <-", var_name);
                
            // Check if the variable is already declared
            is_duplicate = 0;  // Reset duplicate flag
            for (int i = 0; i < declare_index; i++) {
                if (strcmp(var_name, variable_name[i]) == 0) {
                    is_duplicate = 1;  // Mark as duplicate if found
                    break;
                }
            }

                // Declare the variable only if it's not a duplicate
            if (!is_duplicate) {
                declare_variable(var_name, c_file);  // Declare variable in C file with default value
                // Store the variable name in the variable_name list
                strcpy(variable_name[declare_index], var_name);
                declare_index++;
            }
        }
        line_number ++;
    }
        declare_count += 1;
        // If there were errors, stop the process and don't generate C code
        if (error_detected) {
            fprintf(stderr, "! Errors detected. Halting code generation.\n");
            return;  // Exit the function without generating C code
        }

        // If there were errors, stop the process and don't generate C code
        if (error_detected) {
            fprintf(stderr, "! Errors detected. Halting code generation.\n");
            return;  // Exit the function without generating C code
        }

        // Rewind the file pointer to start from the beginning again
        rewind(ml_file);

        while (fgets(line, sizeof(line), ml_file)) {
        char *token = strtok(line, " ");
        if (strcmp(token, "function") == 0) {
            handle_function_definition(line, ml_file, c_file);
        }
    }

    // Start the main function
    fprintf(c_file, "int main() {\n");

    // Rewind the file pointer to start from the beginning again
    rewind(ml_file);

    // Second pass: Handle print statements and remaining variable assignments
    while (fgets(line, sizeof(line), ml_file)) {
        char original_line[1024];  // Copy to store the original line
        strcpy(original_line, line);  // Preserve original line before tokenizing

        char *token = strtok(line, " ");
        
        // Skip any lines that are comments or empty
        if (original_line[0] == '#' || strlen(original_line) < 3) continue;

        // Skip any lines inside of a function in the ML code
        if (isspace(original_line[0]) || original_line[0] == '\t') continue;
        
        // Handle print statements
        if (strcmp(token, "print") == 0) {
            handle_print_statement(original_line, c_file);  // Use original line
        }

        // Handle assignment 
        if (strstr(original_line, "<-")) {
            assign_variable(original_line, c_file);  // Use original line
        }

        // Handle function recall (not print, not return, and no assignment)
        if (strcmp(token, "print") != 0 && strcmp(token, "return") != 0 && !strstr(original_line, "<-")) {
            handle_function_recall(original_line, c_file);  // Use original line
        }
    }

 }
    else {
        fprintf(c_file, "#include <stdio.h>\n");
        fprintf(c_file, "#include <math.h>\n");
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
                if (strchr(line, '+') || strchr(line, '-') || strchr(line, '*') || strchr(line, '/')) {
                    handle_variable_assignment_v2(line, c_file);  // Expressions
                } else {
                    handle_variable_assignment_v1(line, c_file);  // Direct values
                }
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

    system("gcc -o output output.c -lm");
    system("./output");


    printf("Transpilation complete. 'output.c' generated.\n");

    return 0;
}
