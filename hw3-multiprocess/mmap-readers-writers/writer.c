// Homework 3, Part C
// Operating Systems, Dr. Yiming Hu
// Completed by Kristian Snyder

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    perror("Please enter a filename to share.");
    exit(1);
  }

  // Filename to use
  char* filename = argv[1];

  size_t filelen = getpagesize();  // Size of the mapped file

  int f = open(filename, O_RDWR | O_CREAT, (mode_t)0600);

  // Need to do some checks to make sure we can actually use this file.
  struct stat st;
  fstat(f, &st);
  off_t size = st.st_size;

  // Need to lengthen the file.
  if (size < filelen) {
    if (lseek(f, filelen - 1, SEEK_SET) == -1) {
      perror("Couldn't seek in file.");
      exit(1);
    }
    if (write(f, "", 1) == -1) {
      close(f);
      perror("Error writing last character of file");
      exit(1);
    }
  }

  // Open mapped memory of length 20
  char* map_start =
      (char*)mmap(NULL, filelen, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);

  if (map_start == MAP_FAILED) {
    perror("Mapping file failed.");
    exit(1);
  }

  // Create the digit string XXXXXXXXXX (10 characters)
  // with each X being the last digit of the PID
  int pidstrlen = 11;  // (10 characters plus \0)
  pid_t pid = getpid();
  int last_digit = pid % 10;
  // Convert to character by using 0 as the basis for an offset
  char digit_char = last_digit + '0';

  char buf[pidstrlen + 1];
  for (int i = 0; i < pidstrlen - 1; i++) {
    buf[i] = digit_char;
  }

  buf[pidstrlen - 1] = '\0';

  fprintf(stdout, "Will be printing %s\n", buf);

  // Begin the writing process
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < pidstrlen; j++) {
      // Write to memory
      map_start[j] = buf[j];
    }
    // Sleep for a bit
    sleep(2);
  }

  close(f);
}