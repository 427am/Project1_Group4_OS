# Shell

This project involves designing and developing a shell interface that can support features including but not limited to environment variable expansion, $PATH command lookup, and I/O redirection. The project emphasizes command-line parsing, user iteraction, and error-handling. 

## Group Members
- **Jaliah Meade**: jkm23c@fsu.edu
- **Maddy Burns**: -@fsu.edu
- **Samuel Yoder**: sdy22@fsu.edu
## Division of Labor

### Part 1: Prompt
- **Responsibilities**: Display a prompt showing the current user, machine, and working directory
- **Assigned to**: Jaliah Meade

### Part 2: Environment Variables
- **Responsibilities**: Expand tokens starting with $ to their corresponding environment variable values
- **Assigned to**: Jaliah Meade

### Part 3: Tilde Expansion
- **Responsibilities**: Replace ~ or ~/ with the user's home directory path
- **Assigned to**: Jaliah Meade

### Part 4: $PATH Search
- **Responsibilities**: [Description]
- **Assigned to**: Maddy Burns

### Part 5: External Command Execution
- **Responsibilities**: [Description]
- **Assigned to**: Maddy Burns

### Part 6: I/O Redirection
- **Responsibilities**: [Description]
- **Assigned to**: Maddy Burns

### Part 7: Piping
- **Responsibilities**: [Description]
- **Assigned to**: Maddy Burns

### Part 8: Background Processing
- **Responsibilities**: [Description]
- **Assigned to**: Samuel Yoder

### Part 9: Internal Command Execution
- **Responsibilities**: [Description]
- **Assigned to**: Samuel Yoder

### Part 10: External Timeout Executable
- **Responsibilities**: -
- **Assigned to**: -

### Extra Credit
- **Responsibilities**: [Description]
- **Assigned to**: Jaliah Meade, Maddy Burns, Samuel Yoder

## File Listing
```
shell/
│
├── src/
│ ├── main.c
│ └── shell.c
│
├── include/
│ └── shell.h
│
├── README.md
└── Makefile
```
## How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.
- **Dependencies**: List any libraries or frameworks necessary (rust only).

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Development Log
Each member records their contributions here.

### Jaliah Meade

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-10 | Set up local project in VS Code, added starter files, and implemented the prompt users are greeted with  |
| 2025-09-14 | Began to implement environment variable expansion and analyzed starter file to understand its logic|
| 2025-09-19 | Finished implementing environment variables, implemented tilde expansion, and tested code on linprog|

### Maddy Burns

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-DD | [Description of task]  |
| 2025-09-DD | [Description of task]  |
| 2025-09-DD | [Description of task]  |


### Samuel Yoder

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-DD | [Description of task]  |
| 2025-09-DD | [Description of task]  |
| 2025-09-DD | [Description of task]  |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| 2025-09-12 | Jaliah, Maddy, Samuel| Current progress made  | Continue implementing shell features |
| 2025-09-19 | Jaliah, Maddy, Samuel| Finalizing the timeline of completion  | Parts 1-9 will be finished separately by Sep. 26, then extra credit will be worked on together  |


## Bugs
- **Bug 1**: -
- **Bug 2**: -
- **Bug 3**: -

## Extra Credit
- **Extra Credit 1**: Support unlimited number of pipes [2]
- **Extra Credit 2**: Support piping and I/O redirection in a single command [2]
- **Extra Credit 3**: Shell-ception: Execute your shell from within a running shell process repeatedly [1]

## Considerations
- Confirm that tilde and environment variable expansions work consistently across commands.
