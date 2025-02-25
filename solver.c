/*
 * solver.c - Boolean Expression Solver in C
 *
 * This file implements a recursive descent parser for Boolean expressions,
 * along with functions to evaluate expressions and generate truth tables.
 *
 * Supported Boolean Operators:
 *   '+'  => Logical OR
 *   '·'  => Logical AND
 *   '!'  => Logical NOT
 *
 * Parentheses '(' and ')' are supported for grouping.
 *
 * Literals: '0' and '1'
 * Variables: Any alphabetic character (A, B, C, etc.). By default, each variable is assumed to be true (1).
 *
 * Compilation:
 *   gcc -o solver solver.c
 *
 * Usage:
 *   ./solver
 *     - Prompts for a Boolean expression, evaluates it, and displays the result.
 *
 *   ./solver --truth-table
 *     - Prompts for a Boolean expression, then generates and prints its truth table.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 
 /* -------------------------------------------------------------------------
  * Global Variable Mapping:
  * Each index in this array corresponds to an ASCII character.
  * For any alphabetic variable (e.g., 'A'), g_var_mapping[(int)'A'] holds its Boolean value.
  * By default, every variable is set to true (1) unless overridden.
  * ------------------------------------------------------------------------- */
 static int g_var_mapping[256] = {0};
 
 /* -------------------------------------------------------------------------
  * Function Prototypes
  * ------------------------------------------------------------------------- */
 static void skip_whitespace(const char **s);
 static int parse_expression(const char **s);
 static int parse_term(const char **s);
 static int parse_factor(const char **s);
 int evaluate_boolean_expression(const char *expr);
 int evaluate_expr_with_mapping(const char *expr, int mapping[256]);
 void generate_truth_table(const char *expr);
 void print_usage(const char *progname);
 
 /* -------------------------------------------------------------------------
  * skip_whitespace:
  *   Advances the pointer past any whitespace characters.
  * ------------------------------------------------------------------------- */
 static void skip_whitespace(const char **s) {
     while (**s && isspace(**s)) {
         (*s)++;
     }
 }
 
 /* -------------------------------------------------------------------------
  * parse_expression:
  *   Parses an expression which may include one or more terms separated by '+'
  *   (logical OR). Grammar: expression = term { '+' term }
  * ------------------------------------------------------------------------- */
 static int parse_expression(const char **s) {
     int value = parse_term(s);
     skip_whitespace(s);
     while (**s == '+') { // '+' denotes OR
         (*s)++;  // Consume '+'
         skip_whitespace(s);
         int term_value = parse_term(s);
         value = (value || term_value) ? 1 : 0;
         skip_whitespace(s);
     }
     return value;
 }
 
 /* -------------------------------------------------------------------------
  * parse_term:
  *   Parses a term which may include one or more factors separated by '·'
  *   (logical AND). Grammar: term = factor { '·' factor }
  * ------------------------------------------------------------------------- */
 static int parse_term(const char **s) {
     int value = parse_factor(s);
     skip_whitespace(s);
     while (**s == '·') { // '·' denotes AND
         (*s)++;  // Consume '·'
         skip_whitespace(s);
         int factor_value = parse_factor(s);
         value = (value && factor_value) ? 1 : 0;
         skip_whitespace(s);
     }
     return value;
 }
 
 /* -------------------------------------------------------------------------
  * parse_factor:
  *   Parses a factor, which may be:
  *     - A NOT operation: '!' factor
  *     - A grouped expression: '(' expression ')'
  *     - A literal ('0' or '1') or variable (alphabetic character)
  *   Grammar: factor = '!' factor | '(' expression ')' | literal
  * ------------------------------------------------------------------------- */
 static int parse_factor(const char **s) {
     skip_whitespace(s);
     int value = 0;
 
     if (**s == '!') {
         // Handle NOT: !factor
         (*s)++;  // Consume '!'
         int factor_value = parse_factor(s);
         value = (!factor_value) ? 1 : 0;
     } else if (**s == '(') {
         // Handle grouping: ( expression )
         (*s)++;  // Consume '('
         value = parse_expression(s);
         skip_whitespace(s);
         if (**s == ')') {
             (*s)++;  // Consume ')'
         } else {
             fprintf(stderr, "Error: Missing closing parenthesis.\n");
         }
     } else if (isdigit(**s)) {
         // Literal: 0 or 1
         value = **s - '0';
         (*s)++;
     } else if (isalpha(**s)) {
         // Variable: Look up its value in g_var_mapping.
         char var = **s;
         (*s)++;
         value = g_var_mapping[(int)var];
     } else {
         // Skip unrecognized characters (could add error handling here).
         (*s)++;
     }
 
     return value;
 }
 
 /* -------------------------------------------------------------------------
  * evaluate_boolean_expression:
  *   Evaluates a Boolean expression using the default variable mapping,
  *   which assumes all alphabetic variables are true (1).
  *
  *   Parameters:
  *     expr - The Boolean expression as a string.
  *
  *   Returns:
  *     The evaluated result (0 or 1).
  * ------------------------------------------------------------------------- */
 int evaluate_boolean_expression(const char *expr) {
     // Set default mapping: every variable is assumed true.
     for (int i = 0; i < 256; i++) {
         g_var_mapping[i] = 1;
     }
     const char *p = expr;
     int result = parse_expression(&p);
     return result;
 }
 
 /* -------------------------------------------------------------------------
  * evaluate_expr_with_mapping:
  *   Evaluates a Boolean expression using a provided variable mapping.
  *
  *   Parameters:
  *     expr    - The Boolean expression as a string.
  *     mapping - An array of 256 ints mapping each character to its Boolean value.
  *
  *   Returns:
  *     The evaluated result (0 or 1).
  * ------------------------------------------------------------------------- */
 int evaluate_expr_with_mapping(const char *expr, int mapping[256]) {
     memcpy(g_var_mapping, mapping, sizeof(g_var_mapping));
     const char *p = expr;
     int result = parse_expression(&p);
     return result;
 }
 
 /* -------------------------------------------------------------------------
  * generate_truth_table:
  *   Generates and prints a truth table for the provided Boolean expression.
  *   The function scans the expression for unique alphabetic variables, then
  *   iterates over every possible combination of truth values for these variables,
  *   evaluates the expression for each combination, and prints the results.
  *
  *   Parameters:
  *     expr - The Boolean expression.
  * ------------------------------------------------------------------------- */
 void generate_truth_table(const char *expr) {
     // Identify unique variables in the expression.
     char vars[100];
     int var_count = 0;
     memset(vars, 0, sizeof(vars));
     for (const char *p = expr; *p; p++) {
         if (isalpha(*p)) {
             char ch = *p;
             int already_present = 0;
             for (int i = 0; i < var_count; i++) {
                 if (vars[i] == ch) {
                     already_present = 1;
                     break;
                 }
             }
             if (!already_present && var_count < 100) {
                 vars[var_count++] = ch;
             }
         }
     }
 
     // Print table header.
     printf("\nTruth Table:\n");
     for (int i = 0; i < var_count; i++) {
         printf("%c\t", vars[i]);
     }
     printf("Result\n");
 
     // Total number of rows: 2^(number of variables)
     int total_rows = 1 << var_count;
 
     // Iterate over all combinations.
     for (int i = 0; i < total_rows; i++) {
         int mapping[256];
         // Set default: all variables to true.
         for (int j = 0; j < 256; j++) {
             mapping[j] = 1;
         }
         // Assign truth values for each variable based on the current combination.
         for (int j = 0; j < var_count; j++) {
             mapping[(int)vars[j]] = ((i >> (var_count - j - 1)) & 1);
             printf("%d\t", mapping[(int)vars[j]]);
         }
         int result = evaluate_expr_with_mapping(expr, mapping);
         printf("%d\n", result);
     }
 }
 
 /* -------------------------------------------------------------------------
  * print_usage:
  *   Prints usage instructions for the solver.
  * ------------------------------------------------------------------------- */
 void print_usage(const char *progname) {
     printf("Usage: %s [--truth-table]\n", progname);
     printf("If --truth-table is provided, a truth table for the given expression is generated.\n");
 }
 
 /* -------------------------------------------------------------------------
  * main:
  *   Provides a simple command-line interface to the Boolean Expression Solver.
  *
  *   Prompts the user for a Boolean expression, then either evaluates it (default)
  *   or generates a truth table if the '--truth-table' flag is provided.
  * ------------------------------------------------------------------------- */
 int main(int argc, char *argv[]) {
     char expression[256];
 
     printf("Boolean Expression Solver\n");
     printf("-------------------------\n");
     printf("Enter a Boolean expression (use '+' for OR, '·' for AND, '!' for NOT):\n");
 
     if (fgets(expression, sizeof(expression), stdin) == NULL) {
         fprintf(stderr, "Error reading expression.\n");
         return 1;
     }
 
     // Remove trailing newline, if any.
     size_t len = strlen(expression);
     if (len > 0 && expression[len - 1] == '\n') {
         expression[len - 1] = '\0';
     }
 
     if (argc > 1 && strcmp(argv[1], "--truth-table") == 0) {
         // Generate and print the truth table for the provided expression.
         generate_truth_table(expression);
     } else {
         // Evaluate the expression with default variable values (true).
         int result = evaluate_boolean_expression(expression);
         printf("\nEvaluation Result: %d\n", result);
     }
 
     return 0;
 }
 