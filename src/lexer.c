#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	while (1) { // keeps running until the user manually exits 
		// Part 1: Prompt 
		char *user = getenv("USER"); 
		char *machine = getenv("MACHINE");
		char *pwd = getenv("PWD");

		printf("%s@%s:%s>", user, machine, pwd); // USER@MACHINE:PWD> format

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		// reading input
		char *input = get_input(); // calls helper function that reads line of input from the user 
		printf("whole input: %s\n", input); // prints out raw input
		// tokenization
		tokenlist *tokens = get_tokens(input); // calls helper function that splits input string into separate tokens

		// Part 2: Environment Variables - tokenlist is a struct and tokens is a pointer to the struct
		for(int i =0; i<tokens->size; i++) // iterates through all of the tokens
		{
			if(tokens->items[i][0] == '$')	// checks if token is prefixed with a "$"
			{
				char *var = getenv(tokens->items[i]+ 1); // determines expanded value of token
				if(var != NULL)
				{
					char *var_cpy = malloc(strlen(var) + 1); // allocates memory for pointer so it can be freed later 
    				strcpy(var_cpy, var); // makes a copy of expanded token
					tokens->items[i] = var_cpy; // replaces token with expanded value 
				}		
			}
		}

		// Part 3: Tilde Expansion
		for(int i =0; i<tokens->size; i++) // iterates through all of the tokens
		{
			char *home = getenv("HOME"); // determines path
			if(strcmp(tokens->items[i], "~") == 0) // standalone option
			{
				tokens->items[i] = home;
			}
			else if(tokens->items[i][0] == '~' && tokens->items[i][1] == '/') // starts with ~/ option
			{
				char *rest = tokens->items[i] + 1; // everything after the ~
				char *final = malloc(strlen(home) + strlen(rest) + 1); // allocates for size of final expanded version
				strcpy(final, home); // transfers home path to final
				strcat(final, rest); // concatenates everything after the ~ to the home path
				tokens->items[i] = final; // replaces token with expanded value
			}
		}

		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]); // prints each token with its index
		}

		free(input); // frees memory from input string
		free_tokens(tokens); // calls helper function that frees memory from token list
	}

	return 0;
}

char *get_input(void) { // helper function called on line 21
	char *buffer = NULL; // grows dynamically to hold entire input line
	int bufsize = 0; // keeps track of number of characters read 
	char line[5]; // temporary fixed buffer size
	while (fgets(line, 5, stdin) != NULL) // fgets reads up to 4 characters at a time into line from input stream (last index is for \0)
	{
		int addby = 0; // variable to increase size of buffer2
		char *newln = strchr(line, '\n'); // searches for first newline character in line and returns a pointer to newline if found, if not returns NULL
		if (newln != NULL)
			addby = newln - line; // calculates how much to increase buffer by
		else
			addby = 5 - 1; // if no newline character was found buffer must be full so entire new buffer must be added
		buffer = (char *)realloc(buffer, bufsize + addby); // expands buffer size
		memcpy(&buffer[bufsize], line, addby); // copies the characters to a different place in memory
		bufsize += addby; // updates variable
		if (newln != NULL) // breaks out of loop once end of line is reached
			break;
	}
	buffer = (char *)realloc(buffer, bufsize + 1); // makes space for one extra character
	buffer[bufsize] = 0; // writes null terminator at the end of the string 
	return buffer; // returns finishes string
}

tokenlist *new_tokenlist(void) {
	tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist)); // allocates memory for new tokenlist structure
	tokens->size = 0; // initializes size to zero
	tokens->items = (char **)malloc(sizeof(char *)); // allocates space for a pointer
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens; // returns empty token list
}

void add_token(tokenlist *tokens, char *item) {
	int i = tokens->size; // gets current number of tokens in list

	tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *)); // each string is one token of array, realloc grows array with one extra slot for new token and one for null
	tokens->items[i] = (char *)malloc(strlen(item) + 1); // allocates memory for new token string
	tokens->items[i + 1] = NULL; // marks end of array
	strcpy(tokens->items[i], item); // copy actual string from item into newly alllocated memory

	tokens->size += 1; // updates size 
}

tokenlist *get_tokens(char *input) {
	char *buf = (char *)malloc(strlen(input) + 1);
	strcpy(buf, input); // makes a copy of input
	tokenlist *tokens = new_tokenlist(); // creates new empty tokenlist
	char *tok = strtok(buf, " "); // splits buf into tokens separated by " "
	while (tok != NULL)
	{
		add_token(tokens, tok); // gives first token
		tok = strtok(NULL, " "); // keeps going until there are no more tokens
	}
	free(buf);
	return tokens; // returns final token list
}

void free_tokens(tokenlist *tokens) {
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]); // frees each string
	free(tokens->items); // frees array of pointers
	free(tokens); // frees tokenlist struct
}
