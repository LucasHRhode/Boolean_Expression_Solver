/*
 * server.c – CGI-based C backend for the Boolean Expression Solver.
 *
 * This program extracts the Boolean expression from the query string,
 * decodes it, and then processes it. If the "mode" parameter is set to "tt",
 * it generates a truth table; otherwise, it simply evaluates the expression.
 *
 * The expression is expected to use the following operators:
 *   - '+' for logical OR
 *   - '·' for logical AND
 *   - '!' for logical NOT
 * Parentheses '(' and ')' are supported for grouping.
 *
 * For demonstration purposes, when evaluating an expression,
 * any alphabetic variable (e.g. A, B, C) is assumed to have the value 1.
 *
 * Compile with:
 *   gcc -o server.cgi server.c
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 
 /* ============================ */
 /* Utility functions for CGI  */
 /* ============================ */
 
 /**
  * Decodes a URL-encoded string.
  * Converts '+' to space and "%XX" sequences to their character values.
  *
  * @param src The URL-encoded source string.
  * @return A newly allocated decoded string (caller must free it).
  */
 char *url_decode(const char *src) {
     char *dest = malloc(strlen(src) + 1);
     if (!dest) return NULL;
     char *pDest = dest;
     while (*src) {
         if (*src == '+') {
             *pDest++ = ' ';
             src++;
         } else if (*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 2))) {
             char hex[3] = { *(src + 1), *(src + 2), '\0' };
             *pDest++ = (char) strtol(hex, NULL, 16);
             src += 3;
         } else {
             *pDest++ = *src++;
         }
     }
     *pDest = '\0';
     return dest;
 }
 
 /**
  * Extracts the value of a query parameter from the query string.
  *
  * @param query The full query string (e.g. "expr=A+·+B&mode=tt").
  * @param param The parameter name to extract (e.g. "expr").
  * @return A newly allocated string with the parameter value (caller must free it),
  *         or NULL if the parameter is not found.
  */
 char *get_query_param(const char *query, const char *param) {
     char *query_dup = strdup(query);
     if (!query_dup) return NULL;
     char *token = strtok(query_dup, "&");
     char *result = NULL;
     size_t param_len = strlen(param);
     while (token) {
         /* token should be in the form key=value */
         if (strncmp(token, param, param_len) == 0 && token[param_len] == '=') {
             result = strdup(token + param_len + 1);
             break;
         }
         token = strtok(NULL, "&");
     }
     free(query_dup);
     return result;
 }
 
 /* ============================ */
 /* Boolean Expression Parser  */
 /* ============================ */
 
 /*
  * The parser uses a recursive descent approach with the following grammar:
  *
  *   expression = term { '+' term }
  *   term       = factor { '·' factor }
  *   factor     = '!' factor | '(' expression ')' | literal
  *   literal    = '0' | '1' | variable
  *
  * For this CGI demonstration, any alphabetic variable is looked up
  * in a global mapping (g_var_mapping). In evaluation mode (without a provided
  * mapping) we assume every variable is true (1).
  */
 
 /* Global mapping for variables.
  * For each character (e.g., 'A' or 'b'), g_var_mapping[(int)ch] holds its Boolean value.
  */
 static int g_var_mapping[256] = {0};
 
 /* Forward declarations for parser functions */
 static void skip_whitespace(const char **s);
 static int parse_expression(const char **s);
 static int parse_term(const char **s);
 static int parse_factor(const char **s);
 
 /**
  * Skips whitespace characters.
  */
 static void skip_whitespace(const char **s) {
     while (**s && isspace(**s)) {
         (*s)++;
     }
 }
 
 /**
  * Parses an expression (handles OR operations).
  */
 static int parse_expression(const char **s) {
     int value = parse_term(s);
     skip_whitespace(s);
     while (**s == '+') {  /* '+' is used for OR */
         (*s)++;  /* consume '+' */
         skip_whitespace(s);
         int rhs = parse_term(s);
         value = (value || rhs) ? 1 : 0;
         skip_whitespace(s);
     }
     return value;
 }
 
 /**
  * Parses a term (handles AND operations).
  */
 static int parse_term(const char **s) {
     int value = parse_factor(s);
     skip_whitespace(s);
     while (**s == '·') {  /* '·' is used for AND */
         (*s)++;  /* consume '·' */
         skip_whitespace(s);
         int rhs = parse_factor(s);
         value = (value && rhs) ? 1 : 0;
         skip_whitespace(s);
     }
     return value;
 }
 
 /**
  * Parses a factor (handles NOT, parentheses, and literals).
  */
 static int parse_factor(const char **s) {
     skip_whitespace(s);
     int value = 0;
     if (**s == '!') {
         (*s)++;  /* consume '!' */
         int factor_val = parse_factor(s);
         value = (!factor_val) ? 1 : 0;
     } else if (**s == '(') {
         (*s)++;  /* consume '(' */
         value = parse_expression(s);
         skip_whitespace(s);
         if (**s == ')') {
             (*s)++;  /* consume ')' */
         } else {
             fprintf(stderr, "Error: Missing closing parenthesis.\n");
         }
     } else if (isdigit(**s)) {
         value = **s - '0';
         (*s)++;
     } else if (isalpha(**s)) {
         /* For any variable, look up its value from g_var_mapping */
         char var = **s;
         (*s)++;
         value = g_var_mapping[(int)var];
     }
     return value;
 }
 
 /**
  * Evaluates a Boolean expression.
  * Assumes that any variable is true (1) by default.
  *
  * @param expr The Boolean expression as a string.
  * @return The evaluated Boolean result (0 or 1).
  */
 int evaluate_boolean_expression(const char *expr) {
     /* Set default mapping: every variable is assumed true */
     for (int i = 0; i < 256; i++) {
         g_var_mapping[i] = 1;
     }
     const char *p = expr;
     int result = parse_expression(&p);
     return result;
 }
 
 /**
  * Evaluates a Boolean expression using a provided variable mapping.
  *
  * @param expr The Boolean expression as a string.
  * @param mapping An array of 256 ints mapping each character to its Boolean value.
  * @return The evaluated Boolean result (0 or 1).
  */
 int evaluate_expr_with_mapping(const char *expr, int mapping[256]) {
     memcpy(g_var_mapping, mapping, sizeof(g_var_mapping));
     const char *p = expr;
     int result = parse_expression(&p);
     return result;
 }
 
 /* ============================ */
 /* Truth Table Generation       */
 /* ============================ */
 
 /**
  * Generates an HTML truth table for the given Boolean expression.
  *
  * The function first extracts all unique alphabetic variables from the expression,
  * then iterates over all possible truth value combinations, evaluates the expression
  * for each combination, and outputs an HTML table.
  *
  * @param expr The Boolean expression.
  */
 void generate_truth_table(const char *expr) {
     /* Identify unique variables in the expression */
     char vars[100];
     int var_count = 0;
     memset(vars, 0, sizeof(vars));
     for (const char *p = expr; *p; p++) {
         if (isalpha(*p)) {
             char ch = *p;
             int already = 0;
             for (int i = 0; i < var_count; i++) {
                 if (vars[i] == ch) {
                     already = 1;
                     break;
                 }
             }
             if (!already && var_count < 100) {
                 vars[var_count++] = ch;
             }
         }
     }
 
     /* Output the start of an HTML table */
     printf("<table border='1' cellpadding='5' cellspacing='0'>");
     printf("<tr>");
     for (int i = 0; i < var_count; i++) {
         printf("<th>%c</th>", vars[i]);
     }
     printf("<th>Result</th></tr>");
 
     int total_rows = 1 << var_count;
     /* Iterate over every combination of truth values */
     for (int i = 0; i < total_rows; i++) {
         int mapping[256];
         /* Set default: all variables to true */
         for (int j = 0; j < 256; j++) {
             mapping[j] = 1;
         }
         /* For each unique variable, assign a value based on the current combination */
         for (int j = 0; j < var_count; j++) {
             /* The leftmost variable corresponds to the highest-order bit */
             mapping[(int)vars[j]] = ((i >> (var_count - j - 1)) & 1);
         }
         int result = evaluate_expr_with_mapping(expr, mapping);
         /* Output a table row */
         printf("<tr>");
         for (int j = 0; j < var_count; j++) {
             int val = mapping[(int)vars[j]];
             printf("<td>%d</td>", val);
         }
         printf("<td>%d</td>", result);
         printf("</tr>");
     }
     printf("</table>");
 }
 
 /* ============================ */
 /* Main CGI Function          */
 /* ============================ */
 
 int main(void) {
     /* Output the HTTP header */
     printf("Content-Type: text/html\n\n");
 
     /* Begin HTML output */
     printf("<html><head><title>Boolean Expression Solver Result</title></head><body>");
     printf("<h1>Boolean Expression Solver (C Backend)</h1>");
 
     /* Retrieve the QUERY_STRING from the environment */
     char *query = getenv("QUERY_STRING");
     if (query == NULL || strlen(query) == 0) {
         printf("<h2>Error: No query string provided.</h2>");
         printf("</body></html>");
         return 1;
     }
 
     /* Extract the "expr" parameter */
     char *expr_param = get_query_param(query, "expr");
     if (expr_param == NULL) {
         printf("<h2>Error: No expression provided.</h2>");
         printf("</body></html>");
         return 1;
     }
     /* URL-decode the expression */
     char *decoded_expr = url_decode(expr_param);
     free(expr_param);
     if (decoded_expr == NULL) {
         printf("<h2>Error: Failed to decode expression.</h2>");
         printf("</body></html>");
         return 1;
     }
 
     /* Check for an optional "mode" parameter:
      * If mode is "tt", then generate a truth table.
      * Otherwise, perform a simple evaluation.
      */
     char *mode_param = get_query_param(query, "mode");
 
     if (mode_param != NULL && strcmp(mode_param, "tt") == 0) {
         printf("<h2>Truth Table for Expression:</h2>");
         printf("<p>%s</p>", decoded_expr);
         generate_truth_table(decoded_expr);
         free(mode_param);
     } else {
         printf("<h2>Evaluation Result for Expression:</h2>");
         printf("<p>%s</p>", decoded_expr);
         int result = evaluate_boolean_expression(decoded_expr);
         printf("<p>Result: %d</p>", result);
     }
 
     free(decoded_expr);
     printf("</body></html>");
     return 0;
 }
 