#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function to check for unrecognized commands
int check_unrecognized_command(FILE *ml_file) {
    char line[256];
    int line_number = 0;
    int success = 1;  // Track if there is any syntax error

    while (fgets(line, sizeof(line), ml_file)) {
        line_number++;

        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;

        // Skip comment lines
        if (line[0] == '#') continue;

        // Check for empty or whitespace-only lines (except valid code lines)
        if (strlen(line) < 3 || line[0] == '\0') continue;

        // Add more syntax checks as needed (for example, valid identifiers, etc.)
    }

    if (success == 0) {
        // Exit with error code if any syntax error was detected
        exit(1);
        return 1;
    } else {
        rewind(ml_file);
        return 0;
    }
}

// Function to manage how a variable is printed (handles integer/float formatting)
int result_index = 1; //index for the middle variable inside the main function
void print_manager(char *var_name, FILE *c_file) {
    // Check if the variable's value is an integer or floating-point and print accordingly
    fprintf(c_file, "    double Middle_resulT_%d = %s;\n", result_index, var_name); //middle result is to make sure a function only called one time and not multiple during floor() test
    //the werid name is to avoid matching with possible variable in the ml-file
    fprintf(c_file, "    if (floor(Middle_resulT_%d) == Middle_resulT_%d) {\n", result_index, result_index);
    fprintf(c_file, "        printf(\"%%.0f\\n\", Middle_resulT_%d);  // Print as integer\n", result_index);
    fprintf(c_file, "    } else {\n");
    fprintf(c_file, "        printf(\"%%.6f\\n\", Middle_resulT_%d);  // Print as floating-point\n", result_index);
    fprintf(c_file, "    }\n");
    result_index += 1;
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
            continue;  // Ignore comment lines and empty lines
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
    }
        declare_count += 1;

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

        while (fgets(line, sizeof(line), ml_file)) {
            // Skip empty lines and comments
            if (line[0] == '#' || strlen(line) <= 1) {
                continue;  // Ignore comment lines and empty lines
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
                assign_variable(line, c_file);  // Expressions
            }
        }
    }

    fprintf(c_file, "\n    return 0;\n}\n"); // Close the main function
}

int main(int argc, char *argv[]) {
    int arg_index = 0;
    if (argc < 2) {
        fprintf(stderr, "@Usage: %s <file.ml>\n", argv[0]);
        return 1;
    }

    FILE *ml_file = fopen(argv[1], "r");
    if (ml_file == NULL) {
        fprintf(stderr, "@Error: Cannot open file %s\n", argv[1]);
        return 1;
    }

    FILE *c_file = fopen("output.c", "w");
    if (c_file == NULL) {
        fprintf(stderr, "@Error: Cannot create output file\n");
        fclose(ml_file);
        return 1;
    }

    //compiler(ml_file, c_file);
    if (check_unrecognized_command(ml_file)) {
        fprintf(stderr, "!\n");
        fclose(ml_file);
        fclose(c_file);
        return 1;
    }
    else {

        //this part is for the requirement "the variables arg0, arg1, and so on, provide access to the program's command-line arguments which provide real-valued numbers"
        if (argc > 2) {
            for (int i = 2; i < argc; i ++) {
                fprintf(c_file, "double arg%d = %.6f;\n", arg_index, atof(argv[i]));
                arg_index ++;
            }
        }
        compiler(ml_file, c_file);

        fclose(ml_file);
        fclose(c_file);

        system("gcc -o output output.c -lm");
        system("if [ $? -eq 0 ]; then ./output; else echo \"!\"; fi");
    }
    return 0;
}