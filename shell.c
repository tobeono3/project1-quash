#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

extern char **environ;
pid_t fg_pid = -1; // current foreground process

void sigint_handler(int sig) {
    printf("\nCaught Ctrl+C — returning to shell\n");
}

void sigalrm_handler(int sig) {
    if (fg_pid > 0) {
        printf("\nProcess %d exceeded 10 seconds — killing it!\n", fg_pid);
        kill(fg_pid, SIGKILL);
        fg_pid = -1;
    }
}

int main() {
    char command_line[MAX_COMMAND_LINE_LEN];
    char *arguments[MAX_COMMAND_LINE_ARGS];
    char cwd[1024];

    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalrm_handler);

    while (true) {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s> ", cwd);
        else
            printf("> ");
        fflush(stdout);

        if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
            perror("fgets failed");
            exit(EXIT_FAILURE);
        }
        if (feof(stdin)) { printf("\n"); break; }

        command_line[strcspn(command_line, "\n")] = '\0';
        if (strlen(command_line) == 0) continue;

        // ---- PIPE DETECTION ----
        char *pipe_pos = strchr(command_line, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';
            char *cmd1 = command_line;
            char *cmd2 = pipe_pos + 1;

            // tokenize both sides
            char *args1[MAX_COMMAND_LINE_ARGS], *args2[MAX_COMMAND_LINE_ARGS];
            int a1 = 0, a2 = 0;

            char *tok = strtok(cmd1, " \t");
            while (tok && a1 < MAX_COMMAND_LINE_ARGS - 1)
                args1[a1++] = tok, tok = strtok(NULL, " \t");
            args1[a1] = NULL;

            tok = strtok(cmd2, " \t");
            while (tok && a2 < MAX_COMMAND_LINE_ARGS - 1)
                args2[a2++] = tok, tok = strtok(NULL, " \t");
            args2[a2] = NULL;

            int pipefd[2];
            pipe(pipefd);

            pid_t pid1 = fork();
            if (pid1 == 0) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]); close(pipefd[1]);
                execvp(args1[0], args1);
                perror("execvp failed (pipe left)");
                exit(1);
            }

            pid_t pid2 = fork();
            if (pid2 == 0) {
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]); close(pipefd[1]);
                execvp(args2[0], args2);
                perror("execvp failed (pipe right)");
                exit(1);
            }

            close(pipefd[0]); close(pipefd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            continue;
        }

        // ---- Tokenize (no pipe) ----
        int argc = 0;
        char *token = strtok(command_line, " \t");
        while (token && argc < MAX_COMMAND_LINE_ARGS - 1) {
            arguments[argc++] = token;
            token = strtok(NULL, " \t");
        }
        arguments[argc] = NULL;
        if (argc == 0) continue;

        // ---- Built-in Commands ----
        if (strcmp(arguments[0], "exit") == 0) break;
        else if (strcmp(arguments[0], "pwd") == 0) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
        }
        else if (strcmp(arguments[0], "cd") == 0) {
            if (argc < 2) fprintf(stderr, "cd: missing argument\n");
            else if (chdir(arguments[1]) != 0) perror("cd failed");
        }
        else if (strcmp(arguments[0], "echo") == 0) {
            for (int i = 1; i < argc; i++) printf("%s ", arguments[i]);
            printf("\n");
        }
        else if (strcmp(arguments[0], "env") == 0) {
            if (argc == 1)
                for (char **env = environ; *env != NULL; env++) printf("%s\n", *env);
            else {
                char *val = getenv(arguments[1]);
                if (val) printf("%s\n", val);
                else printf("Environment variable not found\n");
            }
        }
        else if (strcmp(arguments[0], "setenv") == 0) {
            if (argc != 3)
                fprintf(stderr, "Usage: setenv VAR VALUE\n");
            else if (setenv(arguments[1], arguments[2], 1) != 0)
                perror("setenv failed");
        }
        else {
            // ---- Background Check ----
            bool background = false;
            if (strcmp(arguments[argc - 1], "&") == 0) {
                background = true;
                arguments[argc - 1] = NULL;
            }

            // ---- Check for I/O redirection ----
            int in_fd = -1, out_fd = -1;
            for (int i = 0; i < argc; i++) {
                if (strcmp(arguments[i], "<") == 0 && i + 1 < argc) {
                    in_fd = open(arguments[i + 1], O_RDONLY);
                    arguments[i] = NULL;
                    break;
                } else if (strcmp(arguments[i], ">") == 0 && i + 1 < argc) {
                    out_fd = open(arguments[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    arguments[i] = NULL;
                    break;
                }
            }

            pid_t pid = fork();
            if (pid < 0) perror("fork failed");
            else if (pid == 0) {
                signal(SIGINT, SIG_DFL);
                if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
                if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }
                if (execvp(arguments[0], arguments) == -1) {
                    perror("execvp failed");
                    exit(1);
                }
            } else {
                if (!background) {
                    fg_pid = pid;
                    alarm(10);
                    int status;
                    waitpid(pid, &status, 0);
                    alarm(0);
                    fg_pid = -1;
                } else {
                    printf("[Background pid %d started]\n", pid);
                }
            }
        }
    }

    return 0;
}
