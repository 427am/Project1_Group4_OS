#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

/* ===== helper (parts 2–3) ===== */
static void replace_token(tokenlist *tokens, int i, const char *newval) {
	char *old = tokens->items[i];
	char *dup = strdup(newval ? newval : "");
	tokens->items[i] = dup;
	free(old);
}

/* ===== job control (part 8) ===== */
#define MAX_JOBS     10
#define MAX_CMDLINE  256

typedef struct {
	pid_t pid;
	int   job_id;
	int   active;
	char  cmdline[MAX_CMDLINE];
} job_t;

static job_t jobs[MAX_JOBS];
static int next_job_id = 1;

static void jobs_init(void) {
	memset(jobs, 0, sizeof(jobs));
	next_job_id = 1;
}

static int jobs_add(pid_t pid, const char *cmdline) {
	for (int i = 0; i < MAX_JOBS; i++) {
		if (!jobs[i].active) {
			jobs[i].active = 1;
			jobs[i].pid = pid;
			jobs[i].job_id = next_job_id++;
			strncpy(jobs[i].cmdline, cmdline ? cmdline : "", MAX_CMDLINE - 1);
			jobs[i].cmdline[MAX_CMDLINE - 1] = '\0';
			return jobs[i].job_id;
		}
	}
	return -1;
}

static void jobs_reap_finished(int verbose) {
	int status;
	pid_t pid;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		for (int i = 0; i < MAX_JOBS; i++) {
			if (jobs[i].active && jobs[i].pid == pid) {
				if (verbose) {
					printf("[%d] + done %s\n", jobs[i].job_id, jobs[i].cmdline);
				}
				jobs[i].active = 0;
				break;
			}
		}
	}
}

static void jobs_list(void) {
	int any = 0;
	for (int i = 0; i < MAX_JOBS; i++) {
		if (jobs[i].active) {
			any = 1;
			printf("[%d]+ %d %s\n", jobs[i].job_id, jobs[i].pid, jobs[i].cmdline);
		}
	}
	if (!any) {
		printf("no active background processes.\n");
	}
}

static void jobs_wait_all(void) {
	for (int i = 0; i < MAX_JOBS; i++) {
		if (jobs[i].active) {
			int status;
			waitpid(jobs[i].pid, &status, 0);
			jobs[i].active = 0;
		}
	}
}

/* ===== last-3 history (part 9: exit) ===== */
static char history_buf[3][MAX_CMDLINE];
static int hist_count = 0;

static void history_add(const char *line) {
	if (!line || !*line) return;
	int idx = hist_count % 3;
	strncpy(history_buf[idx], line, MAX_CMDLINE - 1);
	history_buf[idx][MAX_CMDLINE - 1] = '\0';
	hist_count++;
}

static void history_print_last_three(void) {
	if (hist_count == 0) {
		printf("no valid commands.\n");
		return;
	}
	int n = hist_count < 3 ? hist_count : 3;
	for (int i = 0; i < n; i++) {
		int idx = (hist_count - n + i) % 3;
		printf("%s\n", history_buf[idx]);
	}
}

/* ===== $PATH search (part 4) ===== */
static int is_executable(const char *path) { return access(path, X_OK) == 0; }

char *find_command(char *command) { // function to find full path of command
	if (!command || !*command) return NULL;

	if (strchr(command, '/')) { // if the command has '/' in it, treat it as a path
		return is_executable(command) ? strdup(command) : NULL;	// return a heap duplicate if executable
	}

	char *path_env = getenv("PATH"); // get PATH env variable
	if (path_env == NULL) return NULL; // do not continue if there is no PATH env variable

	char *path_copy = strdup(path_env); // duplicate the path so the orignal is not destroyed
	char *save = NULL;
	char *dir = strtok_r(path_copy, ":", &save); // each dir in PATH is seperated by colons
	while (dir != NULL) { // go through each dir in PATH
		size_t need = strlen(dir) + 1 + strlen(command) + 1;
		char *full_path = (char *)malloc(need); // placeholder to build dir/cmd
		snprintf(full_path, need, "%s/%s", dir, command); // create string 

		if (access(full_path, X_OK) == 0) { // check if the file exists and if it is executable
			free(path_copy); // destroy copied path
			return full_path; // return the working full path (heap)
		}

		free(full_path);
		dir = strtok_r(NULL, ":", &save); // otherwise check next directory
	}

	// if nothing is found, destroy copy and return NULL
	free(path_copy);
	return NULL;
}

/* ===== background/pipes & cmdline text helpers ===== */
static int strip_background(tokenlist *tokens) {
	if (tokens->size == 0) return 0;
	if (strcmp(tokens->items[tokens->size - 1], "&") == 0) {
		free(tokens->items[tokens->size - 1]);
		tokens->items[tokens->size - 1] = NULL;
		tokens->size -= 1;
		return 1;
	}
	return 0;
}

static int count_pipes(tokenlist *tokens) {
	int c = 0;
	for (int i = 0; i < tokens->size; i++) {
		if (strcmp(tokens->items[i], "|") == 0) c++;
	}
	return c;
}

static void build_cmdline(tokenlist *tokens, char *buf, size_t bufsz) {
	buf[0] = '\0';
	for (int i = 0; i < tokens->size; i++) {
		if (!tokens->items[i] || !*tokens->items[i]) continue;
		if (strlen(buf) + strlen(tokens->items[i]) + 2 >= bufsz) break;
		if (i) strcat(buf, " ");
		strcat(buf, tokens->items[i]);
	}
}

/* ===== built-ins (part 9) ===== */
static int is_builtin(tokenlist *tokens) {
	if (tokens->size == 0) return 0;
	char *cmd = tokens->items[0];
	return (!strcmp(cmd, "cd") || !strcmp(cmd, "jobs") || !strcmp(cmd, "exit"));
}

static int run_builtin(tokenlist *tokens) {
	char *cmd = tokens->items[0];
	if (!strcmp(cmd, "cd")) {
		// cd PATH
		// Changes the current working directory.
		// If no arguments are supplied, change the current working directory to $HOME.
		// Signal an error if more than one argument is present.
		// Signal an error if the target is not a directory.
		// Signal an error if the target does not exist.
		int argc = tokens->size;
		if (argc > 2) { fprintf(stderr, "cd: too many arguments\n"); return -1; }
		const char *target = NULL;
		if (argc == 1) {
			target = getenv("HOME");
			if (!target) { fprintf(stderr, "cd: HOME not set\n"); return -1; }
		} else {
			target = tokens->items[1];
		}
		if (chdir(target) != 0) { perror("cd"); return -1; }
		return 0;
	}
	if (!strcmp(cmd, "jobs")) {
		// jobs
		// Outputs a list of active background processes.
		// If there are no active background processes, say so.
		// Format:
		// [Job number]+ [CMD's PID] [CMD's command line]
		jobs_list();
		return 0;
	}
	if (!strcmp(cmd, "exit")) {
		// exit
		// If any background processes are still running, you must wait for them to finish.
		// You can assume that each command is less than 200 characters long.
		// Display the last three valid commands.
		// If there were less than three valid commands, print the last valid one.
		// If there are no valid commands, say so.
		history_print_last_three();
		jobs_wait_all();
		exit(0);
	}
	return -1;
}

/* ===== simple commands + I/O redirection (parts 5–6) ===== */
static int run_simple_with_redirection(tokenlist *tokens, int background, const char *cmdline) {
	if (tokens->size == 0) return 0;

	int in_fd  = -1;
	int out_fd = -1;

	char **argv = (char **)calloc((size_t)tokens->size + 1, sizeof(char *));
	int argc = 0;

	for (int i = 0; i < tokens->size; i++) {
		char *t = tokens->items[i];
		if (!t) continue;

		if (strcmp(t, "<") == 0) {
			if (i + 1 >= tokens->size) { fprintf(stderr, "syntax error: missing input file\n"); free(argv); return -1; }
			int fd = open(tokens->items[i + 1], O_RDONLY);
			if (fd < 0) { perror("open"); free(argv); return -1; }
			in_fd = fd;
			i++;
		} else if (strcmp(t, ">") == 0) {
			if (i + 1 >= tokens->size) { fprintf(stderr, "syntax error: missing output file\n"); if (in_fd != -1) close(in_fd); free(argv); return -1; }
			int fd = open(tokens->items[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0600);
			if (fd < 0) { perror("open"); if (in_fd != -1) close(in_fd); free(argv); return -1; }
			out_fd = fd;
			i++;
		} else {
			argv[argc++] = t;
		}
	}
	argv[argc] = NULL;

	if (argc == 0) {
		if (in_fd  != -1) close(in_fd);
		if (out_fd != -1) close(out_fd);
		free(argv);
		return 0;
	}

	char *cmd_path = find_command(argv[0]);
	if (!cmd_path) {
		printf("Command not found: %s\n", argv[0]);
		if (in_fd  != -1) close(in_fd);
		if (out_fd != -1) close(out_fd);
		free(argv);
		return -1;
	}

	pid_t pid = fork();
	if (pid == 0) {
		if (in_fd  != -1) { dup2(in_fd,  STDIN_FILENO);  close(in_fd); }
		if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }
		execv(cmd_path, argv);
		perror("execv failed");
		_exit(127);
	} else if (pid > 0) {
		free(cmd_path);
		if (in_fd  != -1) close(in_fd);
		if (out_fd != -1) close(out_fd);
		if (background) {
			int job_id = jobs_add(pid, cmdline);
			printf("[%d] %d\n", job_id, pid);
		} else {
			waitpid(pid, NULL, 0);
		}
		free(argv);
		return 0;
	} else {
		perror("fork failed");
		free(cmd_path);
		if (in_fd  != -1) close(in_fd);
		if (out_fd != -1) close(out_fd);
		free(argv);
		return -1;
	}
}

/* ===== pipelines (part 7, up to 2 pipes) ===== */
static int run_pipeline_up_to3(tokenlist *tokens, int npipes, int background, const char *cmdline) {
	if (npipes < 1 || npipes > 2) return -1;

	char **argvs[3] = {0};
	int argc[3] = {0};
	int cur = 0;

	for (int k = 0; k < npipes + 1; k++) {
		argvs[k] = (char **)calloc((size_t)tokens->size + 1, sizeof(char *));
	}

	for (int i = 0; i < tokens->size; i++) {
		if (strcmp(tokens->items[i], "|") == 0) {
			argvs[cur][argc[cur]] = NULL;
			cur++;
			continue;
		}
		argvs[cur][argc[cur]++] = tokens->items[i];
	}
	argvs[cur][argc[cur]] = NULL;

	int ncmds = npipes + 1;

	int pipes[2][2] = {{-1,-1},{-1,-1}};
	for (int i = 0; i < npipes; i++) {
		if (pipe(pipes[i]) < 0) {
			perror("pipe");
			for (int j = 0; j < i; j++) { close(pipes[j][0]); close(pipes[j][1]); }
			for (int k = 0; k < npipes + 1; k++) free(argvs[k]);
			return -1;
		}
	}

	pid_t pids[3] = {0};

	for (int i = 0; i < ncmds; i++) {
		if (!argvs[i][0]) { fprintf(stderr, "pipeline: empty command\n"); goto cleanup; }

		char *cmd_path = find_command(argvs[i][0]);
		if (!cmd_path) { fprintf(stderr, "%s: command not found\n", argvs[i][0]); goto cleanup; }

		pid_t pid = fork();
		if (pid < 0) { perror("fork"); free(cmd_path); goto cleanup; }

		if (pid == 0) {
			if (i > 0) {
				if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) { perror("dup2"); _exit(127); }
			}
			if (i < ncmds - 1) {
				if (dup2(pipes[i][1], STDOUT_FILENO) < 0) { perror("dup2"); _exit(127); }
			}
			for (int k = 0; k < npipes; k++) {
				if (pipes[k][0] != -1) close(pipes[k][0]);
				if (pipes[k][1] != -1) close(pipes[k][1]);
			}
			execv(cmd_path, argvs[i]);
			perror("execv");
			_exit(127);
		}

		pids[i] = pid;
		free(cmd_path);

		if (i > 0) {
			if (pipes[i-1][0] != -1) { close(pipes[i-1][0]); pipes[i-1][0] = -1; }
			if (pipes[i-1][1] != -1) { close(pipes[i-1][1]); pipes[i-1][1] = -1; }
		}
		if (i < ncmds - 1) {
			if (pipes[i][1] != -1) { close(pipes[i][1]); pipes[i][1] = -1; }
		}
	}

	if (background) {
		int job_id = jobs_add(pids[ncmds - 1], cmdline);
		printf("[%d] %d\n", job_id, pids[ncmds - 1]);
	} else {
		for (int i = 0; i < ncmds; i++) {
			if (pids[i] > 0) waitpid(pids[i], NULL, 0);
		}
	}

cleanup:
	for (int k = 0; k < npipes; k++) {
		if (pipes[k][0] != -1) { close(pipes[k][0]); pipes[k][0] = -1; }
		if (pipes[k][1] != -1) { close(pipes[k][1]); pipes[k][1] = -1; }
	}
	for (int k = 0; k < npipes + 1; k++) free(argvs[k]);
	return 0;
}

/* ===== front controller ===== */
void run_command(tokenlist *tokens, int background, const char *cmdline) {
	if (tokens->size == 0) return;

	int pipes = count_pipes(tokens);

	// Piping and I/O redirection will not occur together in a single command (assumption).
	if (pipes > 0) {
		if (pipes > 2) { fprintf(stderr, "too many pipes (max 2)\n"); return; }
		run_pipeline_up_to3(tokens, pipes, background, cmdline);
		return;
	}

	// Internal commands (built-ins) — only in foreground
	if (!background && is_builtin(tokens)) {
		run_builtin(tokens);
		return;
	}

	// External commands with optional redirection
	run_simple_with_redirection(tokens, background, cmdline);
}

int main()
{
	jobs_init();

	while (1) { // keeps running until the user manually exits 
		// Part 1: Prompt 
		char *user = getenv("USER"); 
		char *machine = getenv("MACHINE");
		char *pwd = getenv("PWD");

		printf("%s@%s:%s>", user, machine, pwd); // USER@MACHINE:PWD> format
		fflush(stdout);

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
				const char *name = tokens->items[i] + 1;
				const char *val = getenv(name);
				replace_token(tokens, i, val);
			}
		}

		// Part 3: Tilde Expansion
		for(int i =0; i<tokens->size; i++) // iterates through all of the tokens
		{
			char *home = getenv("HOME"); // determines path
			if(strcmp(tokens->items[i], "~") == 0) // standalone option
			{
				replace_token(tokens, i, home);
			}
			else if(tokens->items[i][0] == '~' && tokens->items[i][1] == '/') // starts with ~/ option
			{
				char *rest = tokens->items[i] + 1; // everything after the ~
				if (home) {
					size_t need = strlen(home) + strlen(rest) + 1;
					char *final = (char *)malloc(need);
					strcpy(final, home);
					strcat(final, rest);
					replace_token(tokens, i, final);
					free(final);
				} else {
					replace_token(tokens, i, rest);
				}
			}
		}

		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]); // prints each token with its index
		}

		/* parts 7–9 */
		int background = strip_background(tokens);
		char cmdline[MAX_CMDLINE];
		build_cmdline(tokens, cmdline, sizeof(cmdline));
		if (*cmdline) history_add(cmdline);
		jobs_reap_finished(1);

		int npipes = count_pipes(tokens);
		if (npipes > 0) {
			if (npipes > 2) {
				fprintf(stderr, "too many pipes (max 2)\n");
			} else {
				run_pipeline_up_to3(tokens, npipes, background, cmdline);
			}
		} else {
			if (!background && is_builtin(tokens)) {
				run_builtin(tokens);
			} else {
				run_simple_with_redirection(tokens, background, cmdline); // execute command for redirection and normal exec
			}
		}

		free(input); // frees memory from input string
		free_tokens(tokens); // calls helper function that frees memory from token list
	}

	return 0;
}


/* -------- the helpers ------- */

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
