// Homework 1
// Operating Systems, Dr. Yiming Hu
// Completed by Kristian Snyder

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

// A list of possible actions to take. Length is the number of actions.
enum Action { parent = 0, ls, ps, contents, length };

void command_exec(enum Action a, char* path) {
  switch (a) {
    case ls: {
      execl("/bin/ls", "/bin/ls", "-l", (char*)NULL);
      break;
    }
    case ps: {
      printf("Entered ps");
      execl("/bin/ps", "/bin/ps", "-ef", (char*)NULL);
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
    printf("Usage: fork_cmd path_to_file\n");
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
