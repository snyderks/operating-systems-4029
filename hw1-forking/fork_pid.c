// Homework 1
// Operating Systems, Dr. Yiming Hu
// Completed by Kristian Snyder

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void print_pid(bool is_parent, int repeats) {
  for (int i = 0; i < repeats; i++) {
    if (is_parent) {
      printf("This is the main process, my PID is %i\n", getpid());
    } else {
      printf(
          "This is a child process. My PID is %i and my parent's PID is %i.\n",
          getpid(), getppid());
    }
    // Wait before we print this again
    sleep(2);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: fork_pid number_of_threads\n");
    return 0;
  }
  int repeats = atoi(argv[1]);
  if (repeats == 0) {
    printf("number_of_threads must be greater than 0.\n");
    return 0;
  }

  // used to prevent duplicate calls of print_pid
  // by one parent process
  bool parent_printed = false;

  // Forking three processes here
  for (int i = 0; i < 3; i++) {
    if (fork() == 0) {
      print_pid(false, repeats);
      break; // don't let children fork more processes
    } else {
      // parent only gets to call the below once
      if (!parent_printed) {
        parent_printed = true;
        print_pid(true, repeats);
      }
      wait(NULL);
    }
  }
}
