#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

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

		run_command(tokens); // execute command for redirection

		free(input); // frees memory from input string
		free_tokens(tokens); // calls helper function that frees memory from token list
	}

	return 0;
}


char *find_command(char *command) { // function to find full path of command
	if (strchr(command, '/')) { // if the command has '/' in it, treat it as a path
		return command;	// return the command unchanged
	}

	char *path_env = getenv("PATH"); // get PATH env variable
	if (path_env == NULL) return NULL; // do not continue if there is no PATH env variable

	char *path_copy = strdup(path_env); // duplicate the path so the orignal is not destroyed
	char *dir = strtok(path_copy, ':'); // each dir in PATH is seperated by colons
	static char full_path[512]; // placeholder to build dir/cmd

	while (dir != NULL) { // go through each dir in PATH
		snprintf(full_path, sizeof(full_path), "%s/%s", dir, command); // create string 

		if (access(full_path, X_OK) == 0) { // check if the file exists and if it is executable
			free(path_copy); // destroy copied path
			return full_path; // return the working full path
		}

		dir = strtok(NULL, ":"); // otherwise check next directory
	}

	// if nothing is found, destroy copy and return NULL
	free(path_copy);
	return NULL;
}

void run_command(tokenlist *tokens) {
	if (tokens->size == 0) return; // nothing to run, just return to exit function

	// file descriptors for input and output redirecting 
	int in_fd = -1;
	int out_fd = -1;

	// loop through tokens to check for input and output (< or >)
	for (int i = 0; i<tokens->size; i++) {
		if (strcmp(tokens->items[i], ">") == 0 && i + 1 < tokens->size) {
			out_fd = open(tokens->items[i+1], O_CREAT | O_WRONLY | O_TRUNC, 0600); // open file for writing, create file if it does not exist, or overwrite a existing file
			if (out_fd < 0) { perror("open"); return;} // if there is error opening file, stop function
			tokens->items[i] = NULL; // end args here so execv know to stop
			break; // only do first output redirect 
		}
		if (strcmp(tokens->items[i], "<") == 0 && i + 1 < tokens->size) {
			in_fd = open(tokens->items[i+1], O_RDONLY); // open file for reading only
			if (in_fd < 0) { perror("open"); return;} // if there is error opening file, stop function
			tokens->items[i] = NULL; // stop args here
			break; // only do first input redirect
		}
	}

	char *cmd_path = find_command(tokens->items[0]); // find full path of command using PATH
	if (!cmd_path) { // if command path was not found
		printf("Command not found: %s\n", toeksn->items[0]);
		return; // exit function
	}

	pid_t pid = fork(); // create a new process
	if (pid == 0) { //child process
		if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd);} // redirect stdin if needed
		if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); } //redirect stdout if needed
		execv(cmd_path, tokens->items); // execute the command
		perror("execv failed"); // if execv fails, print error
		exit(1); // terminate child process
	} else if (pid > 0) {
		waitpid(pid, NULL, 0); // wait for child to finish
	} else { // fork fails
		perror("fork failed"); // print error
	}
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
