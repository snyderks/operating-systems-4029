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

  size_t filelen = 20;  // Size of the mapped file

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
  char* map_start = (char*)mmap(NULL, filelen, PROT_READ, MAP_SHARED, f, 0);

  if (map_start == MAP_FAILED) {
    perror("Mapping file failed.");
    exit(1);
  }

  // Begin the reading process
  for (int i = 0; i < 20; i++) {
    // Print the file
    fprintf(stdout, "%s\n", map_start);
    // Sleep for a bit
    sleep(2);
  }

  // Done using the file. Remove and close.
  close(f);
  remove(filename);
}