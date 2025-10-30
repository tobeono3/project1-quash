Quash — A Custom UNIX Shell

Course: Operating Systems
Project: #1 — Create Your Own Shell
Student: Tobe Onyekwelu
Instructor: Dr. Legand Burge
Institution: Howard University

🧠 Overview

Quash is a simple UNIX-like command shell implemented in C.
It supports basic shell functionalities including:

Built-in commands (cd, pwd, echo, env, setenv, exit)

Process forking and execution using fork() and execvp()

Background processes with &

Signal handling (Ctrl + C)

Auto-killing long-running processes (>10s)

I/O redirection (< and >)

Piping (|) between commands

This project deepens understanding of process control, signals, and I/O redirection in Linux-based systems.

⚙️ Features and Design
1. Built-in Commands

Implemented directly in the shell:

cd [dir] — Change working directory

pwd — Print current directory

echo [args] — Print text or variables

env — List environment variables

setenv VAR VALUE — Create/update environment variable

exit — Exit the shell

2. Process Management

For non-built-in commands, fork() creates a child process and execvp() executes it.

The parent waits for the child to finish unless the command ends with & (background mode).

3. Signal Handling

SIGINT (Ctrl + C) is intercepted to prevent the shell from exiting.

Foreground child processes still respond normally to Ctrl + C.

4. Timeout Mechanism

Foreground processes running longer than 10 seconds are automatically terminated.

Implemented with alarm(10) and a SIGALRM handler.

5. I/O Redirection

command > file redirects stdout to a file.

command < file redirects stdin from a file.

Implemented using dup2() and open() system calls.

6. Piping

Supports one pipe (|) between two commands.

Example:

cat scores | grep Lakers

🧪 Example Usage
/home/codio/workspace> pwd
/home/codio/workspace

/home/codio/workspace> cd testDir1
/home/codio/workspace/testDir1> echo Hello Howard
Hello Howard

/home/codio/workspace/testDir1> sleep 5 &
[Background pid 1234 started]

/home/codio/workspace/testDir1> sleep 15
Process 1235 exceeded 10 seconds — killing it!

/home/codio/workspace/testDir1> cat shell.c > output.txt
/home/codio/workspace/testDir1> more < output.txt

/home/codio/workspace/testDir1> cat scores | grep 28
Indiana 45 28 .616
Utah 44 28 .611
Houston 44 28 .611
Oklahoma City 44 28 .611

🧩 Key System Calls Used
Function	Purpose
getcwd()	Get current directory
chdir()	Change directory
fork()	Create a new process
execvp()	Execute a command
waitpid()	Wait for child completion
signal()	Register signal handlers
alarm()	Trigger timeout
kill()	Terminate a process
dup2()	Duplicate file descriptors
open()	File I/O redirection
🧠 Learning Outcomes

By completing this project, I learned:

How command-line shells interpret and execute user input.

How Linux handles processes, signals, and interprocess communication.

Practical use of system calls and signal handlers in C.

Managing foreground/background processes safely.
