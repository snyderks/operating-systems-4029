// Homework 1
// Operating Systems, Dr. Yiming Hu
// Completed by Kristian Snyder

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

// (2) Write a Unix program that does the following

//       You run the program in command line using the following syntax:

//                        you_program_name file_name

//        Where file_name is the name of a text file under the current directory

//       When the program starts, it does the following

//       If filename is not specified in the command line, or if there are too
//       many parameters, display the correct usage and then exit. Otherwise,

//       It forks three (3) child processes
//       The parent process then displays its own PID information only once,
//       then waits for its child processes die.
//       Let one child-process run the "ls -l" command (using the "execl" system
//       call);
//       Let another child-process run the "ps -ef" command;
//       Let the third child-process display the content of the file (specified
//       by file_name). You can use the program "more" or "cat" to display it.
//       After all child processes terminate, the main process displays "main
//       process terminates" then exits.

enum Action { parent = 0, ls, ps, contents, length };

void command_exec(enum Action a, char* path) {
  switch (a) {
    case ls: {
      execl("/bin/ls", "/bin/ls", "-l", (char*)NULL);
      break;
    }
    case ps: {
      printf("Entered ps");
      execl("/bin/ps", "/bin/ps", (char*)NULL);
      break;
    }
    case contents: {
      printf("Entered contents");
      execl("/bin/cat", "/bin/cat", path, (char*)NULL);
      break;
    }
    case parent: {
      printf("This is the main process, my PID is %i\n", getpid());
      break;
    }
    default: {}
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: fork_pid number_of_threads\n");
    return 0;
  }
  // Check if we can access the file
  if (access(argv[1], R_OK | W_OK) == -1) {
    printf(
        "Please enter a filename in the current directory that exists and "
        "the "
        "program has read/write permissions to.\n");
  }
  command_exec(parent, argv[1]);
  for (int i = 1; i < 4; i++) {
    if (fork() == 0) {
      command_exec(i, argv[1]);
    } else {
      wait(NULL);
      if (i == 3) {
        printf("Main process terminates\n");
      }
    }
  }
}