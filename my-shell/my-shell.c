#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>
#include <glob.h>

// preprocessor directives
#define MAX_TOKENS 100

#ifndef BUFSIZE
#define BUFSIZE 512
#endif

#define printError(msg) do { \
    char buffer[1024]; \
    int len = snprintf(buffer, sizeof(buffer), "my-shell: %s\n", msg); \
    write(STDERR_FILENO, buffer, len); \
} while(0)

#define printCommandError(cmdName, msg) do { \
    char buffer[1024]; \
    int len = snprintf(buffer, sizeof(buffer), "my-shell: `%s` %s\n", cmdName, msg); \
    write(STDERR_FILENO, buffer, len); \
} while(0)

// Built in commands
static const char PWD_COMMAND[] = "pwd";
static const char CD_COMMAND[] = "cd";
static const char EXIT_COMMAND[] = "exit";

// File redirects
static const char STDIN_REDIR[] = "STDIN";
static const char STDOUT_REDIR[] = "STDOUT";
static const char PIPE_WRITE_REDIR[] = "PIPE_WRITE";
static const char PIPE_READ_REDIR[] = "PIPE_READ";

// Command structures
struct Command {
    char command[500];
    char arguments[500][500];
    int argumentsCount;
    char input[500];
    char output[500];
};

struct ProcessedCommand {
    char command[500];
    char arguments[500][500];
    int argumentsCount;
    int input;
    int output;
};

// Messages
char prompt[5] = "mysh>";
char prompterr[6] = "!mysh>";
char hellomsg[50] = "hello! welcome to interactive mode!\n";
char byemsg[7] = "bye!\n";

// Bare name commands PATH
const char* BARE_PATHS[] = { "/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/sbin/", "/bin/" };
static const int BARE_PATH_SIZE = 6;

// Function declarations
int parseLineAndExecute();
int validateTokenArrangement(char** token, int tokenCount);
char** tokenize(char* line, int linepos, int* tokenCount);
struct Command** intoCommands(char** tokens, int tokenCount, int* commandCount);
int executeCommands(struct Command** commands, int commandsCount);
int executeCommand(struct Command** commands, struct ProcessedCommand** pCommands, int commandIndex);

int getCommandCount(char** tokens, int tokenCount);
char* evaluateCommandName(char* commandName);
void cleanup(struct Command** commands, char** tokens, int commandIndex, int tokenCount);
void cleanupPCommands(struct ProcessedCommand** pCommands, int commandCount);

void printCommand(struct Command* command);
void printPCommand(struct ProcessedCommand* pCommand);

char *lineBuffer;
int linePos, lineSize;

void append(char *, int);

int main(int argc, char **argv) {
    int fin, bytes, pos, lstart;
    char buffer[BUFSIZE];

    // open specified file or read from stdin
    if (argc > 1) {
	    fin = open(argv[1], O_RDONLY);
	    if (fin == -1) {
	        perror(argv[1]);
	        exit(EXIT_FAILURE);
	    }
    } else {
        write(1, hellomsg, 50);
	    fin = 0;
    }

    // remind user if they are running in interactive mode
    if (isatty(fin)) {
        
	    write(1, prompt, 5);
    }

    // set up storage for the current line
    lineBuffer = malloc(BUFSIZE);
    lineSize = BUFSIZE;
    linePos = 0;

    // read input
    while ((bytes = read(fin, buffer, BUFSIZE)) > 0) {
        // search for newlines
        lstart = 0;
        for (pos = 0; pos < bytes; ++pos) {
            if (buffer[pos] == '\n') {
                int thisLen = pos - lstart + 1;

                append(buffer + lstart, thisLen);
                int checkline = parseLineAndExecute();
                linePos = 0;

                if(fin == 0) {
                    if(checkline == 0) {
                        write(1, prompt, 5);
                    }
                    else {
                        write(1, prompterr, 6);
                    }
        
                }

                lstart = pos + 1;
            }
        }
        
        if (lstart < bytes) {
            // partial line at the end of the buffer
            int thisLen = pos - lstart;
            append(buffer + lstart, thisLen);
        }
    }

    // file ended with partial line
    if (linePos > 0) {
	    append("\n", 1);
	    parseLineAndExecute();
    }

    free(lineBuffer);
    close(fin);
    return EXIT_SUCCESS;
}

// add specified text the line buffer, expanding as necessary
// assumes we are adding at least one byte
void append(char *buf, int len)
{
    int newPos = linePos + len;
    if (newPos > lineSize) {
        lineSize *= 2;

        lineBuffer = realloc(lineBuffer, lineSize);
        if (lineBuffer == NULL) {
            perror("line buffer");
            exit(EXIT_FAILURE);
        }
    }
    memcpy(lineBuffer + linePos, buf, len);
    linePos = newPos;
}

int parseLineAndExecute() {
    int tokenCount;
    char** tokens = tokenize(lineBuffer, linePos, &tokenCount);

    if(tokens == NULL) {
        return 0;
    }
    
    if (validateTokenArrangement(tokens, tokenCount)) {
        cleanup(NULL, tokens, 0, tokenCount);
        return 1;
    }
    
    int commandCount;
    struct Command** commands = intoCommands(tokens, tokenCount, &commandCount);

    if(executeCommands(commands, commandCount)) {
        cleanup(commands, tokens, commandCount, tokenCount);
        return 1;
    }

    cleanup(commands, tokens, commandCount, tokenCount);

    return 0;
}

int getCommandCount(char** tokens, int tokenCount) {
    int count = 1;

    for(int i = 0; i < tokenCount; i++){
        if(strcmp(tokens[i], "|") == 0) {
            count++;
        }
    }

    return count;
}

struct Command** intoCommands(char** tokens, int tokenCount, int* commandCount) {
    *commandCount = getCommandCount(tokens, tokenCount);
    struct Command** commands = malloc(sizeof(struct Command*) * *commandCount);
    
    for(int i = 0; i < *commandCount; i++) {
        commands[i] = malloc(sizeof(struct Command));
        strcpy(commands[i]->input, STDIN_REDIR);
        strcpy(commands[i]->output, STDOUT_REDIR);
        commands[i]->argumentsCount = 0;
    }

    int commandIndex = 0;
    char isStartOfCommand = 1;

    for(int i = 0; i < tokenCount; i++) {
        if(isStartOfCommand) {
            if(tokens[i][0] == '~') {
                strcpy(commands[commandIndex]->command, getenv("HOME"));
                strcat(commands[commandIndex]->command, tokens[i] + 1);
            } else {
                strcpy(commands[commandIndex]->command, tokens[i]);
            }
            isStartOfCommand = 0;

            strcpy(commands[commandIndex]->arguments[commands[commandIndex]->argumentsCount], commands[commandIndex]->command);
            commands[commandIndex]->argumentsCount++;
        } else {
            if(strcmp(tokens[i], "|") == 0) {
                strcpy(commands[commandIndex]->output, PIPE_WRITE_REDIR);
                strcpy(commands[commandIndex + 1]->input, PIPE_READ_REDIR);
                isStartOfCommand = 1;
                commandIndex++;
            } else if (strcmp(tokens[i], "<") == 0) {
                i++;
                strcpy(commands[commandIndex]->input, tokens[i]);
            } else if (strcmp(tokens[i], ">") == 0) {
                i++;
                strcpy(commands[commandIndex]->output, tokens[i]);
            } else {
                if(tokens[i][0] == '~') {
                    strcpy(commands[commandIndex]->arguments[commands[commandIndex]->argumentsCount], getenv("HOME"));
                    strcat(commands[commandIndex]->arguments[commands[commandIndex]->argumentsCount], tokens[i] + 1);
                } else {
                    strcpy(commands[commandIndex]->arguments[commands[commandIndex]->argumentsCount], tokens[i]);
                }

                commands[commandIndex]->argumentsCount++;
            }
        }
    }

    return commands;
}

int executeCommands(struct Command** commands, int commandCount) {
    struct ProcessedCommand** pCommands = malloc(sizeof(struct ProcessedCommand*) * commandCount);
    // initialize the processed commands early
    for(int i = 0; i < commandCount; i++) {
        pCommands[i] = malloc(sizeof(struct ProcessedCommand));
    }

    for(int i = 0; i < commandCount; i++) {
        int result = executeCommand(commands, pCommands, i);

        if(result) {
            cleanupPCommands(pCommands, commandCount);
            return result;
        }
    }

    cleanupPCommands(pCommands, commandCount);
    return EXIT_SUCCESS;
}


int executeCommand(struct Command** commands, struct ProcessedCommand** pCommands, int commandIndex) {
    struct Command* command = commands[commandIndex];
    struct ProcessedCommand* pCommand = pCommands[commandIndex];
        
    // Evaluate command name to path or built in
    if(strcmp(command->command, PWD_COMMAND) == 0 || strcmp(command->command, CD_COMMAND) == 0 || strcmp(command->command, EXIT_COMMAND) == 0) {
        strcpy(pCommand->command, command->command);
    } else {
        struct stat stats;

        if(stat(command->command, &stats) == 0 && stats.st_mode & S_IXUSR) {
            realpath(command->command, pCommand->command);
            // strcpy(pCommand->command, command->command);
        } else {
            char testPath[100];
            int success = 0;

            for(int i = 0; i < BARE_PATH_SIZE; i++) {
                strcpy(testPath, BARE_PATHS[i]);
                strcat(testPath, command->command);

                if(stat(testPath, &stats) == 0 && stats.st_mode & S_IXUSR) {
                    strcpy(pCommand->command, testPath);
                    success = 1;
                    break;
                }
            }

            if(!success) {
                printCommandError(command->command, "is not a recognized command");
                return 1;
            }
        }
    }

    // process files
    if(strcmp(command->input, STDIN_REDIR) == 0) {
        pCommand->input = STDIN_FILENO;
    } else if(strcmp(command->input, STDOUT_REDIR) == 0) {
        pCommand->input = STDOUT_FILENO;
    } else if(strcmp(command->input, PIPE_READ_REDIR) == 0) {
        // do nothing since PIPE_READ_REDIR handled this early
    } else {
        char path[100];
        if(realpath(command->input, path) == NULL) {
            printCommandError(command->input, "does not exist");
            return 1;
        }
        int file = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);

        if(file == -1) {
            printCommandError(command->input, "cannot be accessed");
            return 1;
        }

        pCommand->input = file;
    }

    if(strcmp(command->output, STDOUT_REDIR) == 0) {
        pCommand->output = STDOUT_FILENO;
    } else if(strcmp(command->output, STDIN_REDIR) == 0) {
        pCommand->output = STDIN_FILENO;
    } else if(strcmp(command->output, PIPE_WRITE_REDIR) == 0) { 
        int fd[2];
        if(pipe(fd) == -1) {
            printError("cannot create pipe");
            return 1;
        }
        pCommand->output = fd[1];
        pCommands[commandIndex + 1]->input = fd[0];
    } else {
        int file = open(command->output, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
        if(file == -1) {
            printCommandError(command->output, "cannot be accessed");
            return 1;
        }
        pCommand->output = file;
    }

    // resolve wildcards
    pCommand->argumentsCount = 0;
    for(int i = 0; i < command->argumentsCount; i++) {
        int hasWildcard = strchr(command->arguments[i], '*') != NULL;

        if(hasWildcard) {
            glob_t results;
            int ret;            

            ret = glob(command->arguments[i], 0, NULL, &results);

            if (ret == 0) {
                strcpy(pCommand->arguments[i], command->arguments[i]);

                for (int j = 0; j < results.gl_pathc; j++) {
                    strcpy(pCommand->arguments[pCommand->argumentsCount], results.gl_pathv[j]);
                    pCommand->argumentsCount++;
                }

                globfree(&results);
            } else {
                strcpy(pCommand->arguments[pCommand->argumentsCount], command->arguments[i]);
                pCommand->argumentsCount++;
            }
        } else {
            strcpy(pCommand->arguments[pCommand->argumentsCount], command->arguments[i]);
            pCommand->argumentsCount++;
        }
    }

    // executing command
    if(strcmp(pCommand->command, EXIT_COMMAND) == 0) {
        write(1, byemsg, 7);
        exit(EXIT_SUCCESS);
    } else if(strcmp(pCommand->command, CD_COMMAND) == 0) {
        int og_stdout;
        int og_stdin;

        if(pCommand->input != STDIN_FILENO) {
            og_stdin = dup(STDIN_FILENO);
            dup2(pCommand->input, STDIN_FILENO);
            close(pCommand->input);
        }

        if(pCommand->output != STDOUT_FILENO) {
            og_stdout = dup(STDOUT_FILENO);
            dup2(pCommand->output, STDOUT_FILENO);
            close(pCommand->output);
        }

        if(pCommand->argumentsCount == 2) {
            if(chdir(command->arguments[1])) {
                printCommandError(command->command, "cannot change the current working directory");
                return 1;
            }
        } else if (pCommand->argumentsCount == 1) {
            if(chdir(getenv("HOME"))) {
                printCommandError(command->command, "cannot access the home environment variable");
                return 1;
            }
        } else {
            printCommandError(command->command, "invalid number of arguments");
            return 1;
        }

        if(pCommand->input != STDIN_FILENO) {
            dup2(og_stdin, STDIN_FILENO);
        }

        if(pCommand->output != STDOUT_FILENO) {
            dup2(og_stdout, STDOUT_FILENO);
        }

        return EXIT_SUCCESS;
    } else {
        pid_t pid = fork();
        if (pid == -1) {
            printError("failed to execute command");
            return 1;
        } else if (pid == 0) {
            if(pCommand->input != STDIN_FILENO) {
                dup2(pCommand->input, STDIN_FILENO);
                close(pCommand->input);
            }

            if(pCommand->output != STDOUT_FILENO) {
                dup2(pCommand->output, STDOUT_FILENO);
                close(pCommand->output);
            }

            if(strcmp(command->command, PWD_COMMAND) == 0) {
                char cwd[500];
                getcwd(cwd, sizeof(cwd));

                write(STDOUT_FILENO, cwd, strlen(cwd));
                write(STDOUT_FILENO, "\n", 1);

                exit(EXIT_SUCCESS);
            } else {
                char** args = malloc(sizeof(char*) * (pCommand->argumentsCount + 1));
                for(int i = 0; i < pCommand->argumentsCount; i++) {
                    args[i] = pCommand->arguments[i];
                }

                args[pCommand->argumentsCount] = NULL;

                execv(pCommand->command, args); 
            }

            exit(EXIT_FAILURE);
        } else {
            int status;
            wait(&status);
            
            if(strcmp(command->input, PIPE_READ_REDIR) == 0) {
                close(pCommand->input);
            }
            
            if(strcmp(command->output, PIPE_WRITE_REDIR) == 0) {
                close(pCommand->output);
            }

            return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
        }
    }

    return 0;
}

char** tokenize(char* line, int linepos, int* tokenCount) {
    line[linepos] = '\0';

    int count = 0;
    int tokencount = 0;
    int i = 0;
    char strtemp[500] = "";
    char ch;

    if(strlen(line) == 1 && line[0] == '\n') {
        return NULL;
    }

    char** tokens = malloc(sizeof(char*) *  100);

    while(line[i] != '\0') {
        //space delimiter or end of line
        if (line[i] == ' ' || line[i] == '\0') {
            if(count != 0) {
                tokens[tokencount] = malloc((count+1) * sizeof(char));
                strcpy(tokens[tokencount], strtemp);
                count = 0;
                tokencount++;
                memset(strtemp, 0, sizeof(strtemp));
            }
        }

        //special tokens
        else if (line[i] == '<' || line[i] == '>' || line[i] == '|') {
            if(count>0) {
                //part 1 (storing before special character)
                tokens[tokencount] = malloc((count+1) * sizeof(char));
                strcpy(tokens[tokencount], strtemp);
                memset(strtemp, 0, sizeof(strtemp));
                tokencount++;
                
                //part 2 (storing special character itself)
                ch = line[i];
                strncat(strtemp, &ch, 1);
                tokens[tokencount] = malloc(2 * sizeof(char));
                strcpy(tokens[tokencount], strtemp);
                tokencount++;
                memset(strtemp, 0, sizeof(strtemp));
                count = 0;
            }
            else {
                ch = line[i];
                strncat(strtemp, &ch, 1);
                tokens[tokencount] = malloc(2 * sizeof(char));
                strcpy(tokens[tokencount], strtemp);
                tokencount++;
                memset(strtemp, 0, sizeof(strtemp));
                count = 0;
            }  
        }

        //regular character
        else {
            ch = line[i];
            strncat(strtemp, &ch, 1);
            count++;
        }
        i++;
    }

    //flush out whats left into tokens array
    if(count > 1) {
         tokens[tokencount] = malloc((count+1) * sizeof(char));
         strcpy(tokens[tokencount], strtemp);
        tokencount++;
    }

    char nl = '\n';

    int n = strlen(tokens[tokencount-1]);
    if (strchr(tokens[tokencount-1], nl) != NULL) {
        tokens[tokencount-1][n-1] = 0;
    }

    *tokenCount = tokencount;
    return tokens;
}

int validateTokenArrangement(char** tokens, int tokenCount) {
    if(strcmp(tokens[0], "<") == 0 || strcmp(tokens[0], ">") == 0 || strcmp(tokens[0], "|") == 0 || strcmp(tokens[0], "*") == 0) { 
        printError("unexpected token at start of line");
        return 1;
    }

    if(strcmp(tokens[tokenCount-1], "<") == 0 || strcmp(tokens[tokenCount-1], ">") == 0 || strcmp(tokens[tokenCount-1], "|") == 0) { 
        printError("unexpected token at end of line");
        return 1;
    }

    int istoken = 0;

    for(int i = 0; i < tokenCount; i++) {
        if(strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "|") == 0) {
            if(istoken == 1) {
                printError("unexpected consecutive tokens");
                return 1;
            }
            istoken = 1;
        } else {
            istoken = 0;
        }
    }

    return 0;

}

void cleanup(struct Command** commands, char** tokens, int commandCount, int tokenCount) {
    for(int i = 0; i < commandCount; i++) {
        free(commands[i]);
    }
    free(commands);

    for(int i = 0; i < tokenCount; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

void cleanupPCommands(struct ProcessedCommand** pCommands, int commandCount) {
    for(int i = 0; i < commandCount; i++) {
        free(pCommands[i]);
    }
    free(pCommands);
}

void printCommand(struct Command* command) {
    printf("Command: %s\n", command->command);
    printf("%d arguments: \n", command->argumentsCount);
    for(int i = 0; i < command->argumentsCount; i++) {
        printf("%s ", command->arguments[i]);
    }
    printf("\n");
    printf("Input: %s\n", command->input);
    printf("Output: %s\n\n", command->output);
}

void printPCommand(struct ProcessedCommand* pCommand) {
    printf("Command: %s\n", pCommand->command);
    printf("%d Arguments: \n", pCommand->argumentsCount);
    for(int i = 0; i < pCommand->argumentsCount; i++) {
        printf("%s ", pCommand->arguments[i]);
    }
    printf("\n");
    printf("Input: %d\n", pCommand->input);
    printf("Output: %d\n\n", pCommand->output);
}
