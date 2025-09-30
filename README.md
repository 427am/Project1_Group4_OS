# Shell

This project involves designing and developing a shell interface that can support features including but not limited to environment variable expansion, $PATH command lookup, and I/O redirection. The project emphasizes command-line parsing, user interaction, and error-handling. 

## Group Members
- **Jaliah Meade**: jkm23c@fsu.edu
- **Maddy Burns**: mrb23g@fsu.edu
- **Samuel Yoder**: sdy22@fsu.edu

## Division of Labor

### Part 1: Prompt
- **Responsibilities**: Display a prompt showing the current user, machine, and working directory.
- **Assigned to**: Jaliah Meade

### Part 2: Environment Variables
- **Responsibilities**: Expand tokens starting with $ to their corresponding environment variable values.
- **Assigned to**: Jaliah Meade

### Part 3: Tilde Expansion
- **Responsibilities**: Replace ~ or ~/ with the user's home directory path.
- **Assigned to**: Jaliah Meade

### Part 4: $PATH Search
- **Responsibilities**: Search through the folders listed in the $PATH variable to find where the command is stored.
- **Assigned to**: Maddy Burns

### Part 5: External Command Execution
- **Responsibilities**: Run programs entered by the user by creating a new process for each.
- **Assigned to**: Maddy Burns

### Part 6: I/O Redirection
- **Responsibilities**: Allow commands to read input from a file or send outputs to a file.
- **Assigned to**: Maddy Burns

### Part 7: Piping
- **Responsibilities**: Support up to two pipes in a single command.
- **Assigned to**: Samuel Yoder

### Part 8: Background Processing
- **Responsibilities**: Implement background job execution (&) and job tracking.
- **Assigned to**: Samuel Yoder

### Part 9: Internal Command Execution
- **Responsibilities**: Implement built-ins (cd, jobs, exit).
- **Assigned to**: Samuel Yoder

### Part 10: Extra Credit
- **Responsibilities**: [Not attempted]
- **Assigned to**: Jaliah Meade, Maddy Burns, Samuel Yoder

## File Listing
```
shell/
│
├── src/
│   └── lexer.c
│
├── include/
│   └── lexer.h
│
├── README.md
└── Makefile
```

## How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C, `rustc` for Rust.
- **Dependencies**: None (C only).

### Compilation
For C:
```bash
make
```
This will build the executable in `bin/shell`.

### Execution
```bash
./bin/shell
```
This will launch the shell.

## Development Log
Each member records their contributions here.

### Jaliah Meade

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-10 | Set up local project in VS Code, added starter files, and implemented the prompt users are greeted with. |
| 2025-09-14 | Began implementing environment variable expansion and analyzed starter file logic. |
| 2025-09-19 | Finished environment variables, implemented tilde expansion, and tested code on linprog. |

### Maddy Burns

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-20 | Implemented $PATH search logic to locate executables in system directories. |
| 2025-09-22 | Added external command execution using fork() and execv(); ensured argument handling. |
| 2025-09-25 | Completed I/O redirection and piping (≤ 2 pipes); tested commands and error handling. |

### Samuel Yoder

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-26 | Implemented background job execution (&), including job numbering and PID tracking. |
| 2025-09-27 | Implemented built-in commands: cd, jobs, and exit (with waiting for jobs). |
| 2025-09-28 | Conducted integration testing across Mac and WSL; finalized README and documentation. |

## Meetings
| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| 2025-09-12 | Jaliah, Maddy, Samuel | Current progress made on Part 1, made rough timelines plans. | Continue implementing shell features. |
| 2025-09-19 | Jaliah, Maddy, Samuel | Finalized timeline of completion. | Parts 1–9 will be finished separately by Sep. 28. |
| 2025-09-28 | Jaliah, Maddy, Samuel | Tested the completed project. | Our project successfully meets all criteria. |

## Bugs
- **Bug 1**: Prompt displays `(null)` for machine name on some systems. This is cosmetic only and does not affect grading. |
- **Bug 2**: Tokenizer splits on spaces, so `/usr/bin/printf "ok %d\n" 123` must be entered without spaces. |
- **Bug 3**: N/A. |

## Considerations
- Confirmed: tilde and environment variable expansions work consistently.
- Confirmed: all features tested across linprog.
- Confirmed: the assumptions and restrictions in prompt are all followed.

