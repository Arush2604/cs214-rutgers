# my-shell

## Collaborators

- Arush Verma (aav87)
- Smail Barkouch (sb2108)

## Fulfilled Extensions

1. Home directory
2. Directory wildcards
3. Multiple pipes

## Implementation

There are two modes of execution to consider:
1. Batch mode
2. Interactive mode

To make a distinction between the two, we just need to check the number of command line arguments.
- 1 command line argument -> interactive mode
- 2 command line arguments -> batch mode

### Batch Mode

We will get the file from the command line arguments, and read its contents in a loop. Every line is executed sequentially.

### Interactive Mode

In this mode, we accept standard input. We prompt the user that input is ready to be handled, and they provide commands and arguments.

We achieve this flow by running a loop that constantly accepts input, and writes a prompt before input using `write()`. In cases where the last command failed, we write `!mysh>`.

### Tokenizing a command

The main logic for tokenizing a line involves creating a tokens array that holds all tokens for a line. We create this by parsing a line everytime a newline occurs (this method works for both interactive and batch modes).

We create a `tokenize` function for this. We malloc a `char** array` and have written a custom parsing functions that loops through a given string and tokenizes appropriately. The result is an array of strings, with each string being a token (either a part of a command such as cd, or a special character like `<`). 

The parsing algorithm sequentially checks each character in the line, and creates tokens appropriately for when it counters a ' ' (space) character, or special tokens. It handles cases involving multiple spaces together, spaces before/after special characters and error checks later on for unexpected token ordering (see test1).
 
This `tokenize` function returns the tokens array and feeds it into our function that creates the commands. Once a set of commands have been created and run, all necessary `frees` are called and the process repeats for the next line in the specified file for batch mode, or the next typed line in interactive mode.

### Generating a Command

To generate a command from a token, we need to sequentally go through all the tokens. We will have a struct that defines what a command will be:
```
struct Command {
    char command[500];
    char arguments[500][500];
    int argumentsCount;
    char input[500];
    char output[500];
};
```

This `struct` is everything that needs to be known to evaluate it. 

Note, the `input` and `output` lines are not paths, nor file descriptions, just whatever token was determined to be the input or output. If no token was, then the default is `STDIN` and `STDOUT`. This in itself is not enough information for when we actually run the command using `execv`. The fully evaluated command `struct` is:
```
struct ProcessedCommand {
    char command[500];
    char arguments[500][500];
    int argumentsCount;
    int input;
    int output;
};
```

This will be created in the subsequent steps.

### Path Checks and Evaluation

For a command to be run, we need to verify its existance. That means a command must be specified in one of these three ways.
1. built in command: For a command to be built-in, it needs to either be `cd`, `pwd`, `exit`.
2. path names: For a command to be a path name, it needs to be an existing file.
    - we can verify existance using `stat`.
3. bare names: For a command to be a bare name, it needs to exist in the basic directories.
    - we can verify existance of the bare name by appending it to the basic directories we check, and then running `stat`.

### Wildcards

We can use wildcards to provide a variable amount of arguments than initially specified. Wildcards can be used in the beginning, middle, and end of a string to represent a variable amount of characters. We will use `glob.h` to evaluate wildcards. It will provide us with the files matching the wildcard description, and we will simply append these files in our arguments to be used in execution.

### Redirection and Pipes

We redirect `STDIN` and `STDOUT` using `dup2` in the child process.

Pipes are a convient way to match the standard output of one program as the standard input as another. We can do this by calling `pipe` to create a pipe, and using `dup2` to modify `STDIN` and `STDOUT`. `pipe` is used before the creation of the child process, and `dup2` is mainly used in the child process. After a child process finishes and it used a file descriptior created by `pipe`, we need to close it in the parent process.

## Testing Strategy

**Test Cases**:

We want to test 3 different scenarios:
1. test delimiter tokens: unexpected token ordering
    - delimiter token at start of line
    - delimiter token at end of line
    - consecutive delimiter tokens
2. test failing command parsing: cannot create the command
    - command does not exist
    - input redirection file does not exist
3. test failing command execution: the executed command gives its own error
Test with `ls`, `cd`, and `/bin/cat` (one of each type of command specification)
    - too many arguments for a command
    - improper arguments for a command
4. test basic commands work
    - `ls`
    - `cd`
    - `pwd`
    - `exit`

These tests will happen over batch mode since they can be better automated. Both interactive and batch mode use the same parsing and executing logic, so they will have the same output.

## Test Document and expected outputs 

** The test document **

There will be 5 documents used for testing purposes
Each document will be inputted into the program 'my-shell' and each line will be run sequentially.

1. test1.sh - tests unexpected token ordering 

2. test2.sh - tests non-existent commands and input redirection issues

3. test3.sh - tests commands with too many, too little or improper arguments

4. test4.sh - tests regular commands with no errors (no lines should print an error message)

5. test5.sh - tests extensions 


### Expected Outputs

test1.sh:
```
my-shell: unexpected token at start of line
my-shell: unexpected token at end of line
my-shell: unexpected token at start of line
my-shell: unexpected consecutive tokens
my-shell: unexpected consecutive tokens
my-shell: unexpected token at start of line
```

test2.sh:
```
my-shell: `uh` is not a recognized command
my-shell: `fakecommand` is not a recognized command
my-shell: `apple` does not exist
```

test3.sh
```
my-shell: `cd` invalid number of arguments
ls: cannot access 'testls': No such file or directory
my-shell: `cd` cannot change the current working directory
rmdir: failed to remove 'apple': No such file or directory
my-shell: `cd` cannot change the current working directory
/bin/cat: apple.sh: No such file or directory
```

test4.sh
```
Makefile  my-shell.c  my-shell.o  README.md  test1.sh  test2.sh  test3.sh  test4.sh  test5.sh
apple  Makefile  my-shell.c  my-shell.o  README.md  test1.sh  test2.sh  test3.sh  test4.sh  test5.sh
Makefile  my-shell.c  my-shell.o  README.md  test1.sh  test2.sh  test3.sh  test4.sh  test5.sh
```

test5.sh
```
test1.sh
cookie.l
cookie.md
cookie.r
Makefile  my-shell.c  my-shell.o  README.md  test1.sh  test2.sh  test3.sh  test4.sh  test5.sh
```