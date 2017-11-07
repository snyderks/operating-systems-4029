// Homework 3, Part B
// Operating Systems, Dr. Yiming Hu
// Completed by Kristian Snyder

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include "key.h"  // Get the id used for key generation

void writeToMem(char* shm_start, int shm_len) {
  const char* endStr = "STOP!!!";
  if (shm_len < 8) {
    perror("Shared memory space is too small.");
    exit(1);
  }

  size_t bufLen = (size_t)shm_len - strlen(endStr);

  char* buf = (char*)malloc(bufLen);

  const char msg[] = "Enter a message to send:\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);

  if (fgets(buf, bufLen, stdin) == NULL) {
    perror("Failed to read input.");
    exit(1);
  }

  // Put the data in the shared memory buffer
  sprintf(shm_start, "%s%s", buf, endStr);
}

int main() {
  key_t k = ftok(".", SHM_KEY);

  if (k == -1) {
    perror("Couldn't get key");
    exit(1);
  }

  int shmid = shmget(k, SHM_SIZE, 0);

  if (shmid == -1) {
    perror("Failed to get shared memory space");
    exit(1);
  }

  char *shm_start, c;

  if ((shm_start = shmat(shmid, NULL, 0)) == (char*)-1) {
    perror(
        "Couldn't attach with shmat - make sure the shared memory space "
        "exists");
    exit(1);
  }

  ////
  //// Semaphore access
  ////

  // get the semaphore
  key_t semkey = ftok(".", SHM_KEY);  // Use the same key as we were using
  int semid = semget(semkey, 1, 0600);

  if (semid == -1) {
    perror("Semaphore access failed");
    exit(1);
  }

  // Create the operations
  struct sembuf WAIT[1];
  WAIT[0].sem_num = 0;
  WAIT[0].sem_op = -1;
  WAIT[0].sem_flg = 0;

  // Create the operations
  struct sembuf SIGNAL[1];

  SIGNAL[0].sem_num = 0;
  SIGNAL[0].sem_op = 1;
  SIGNAL[0].sem_flg = 0;

  // Infinite loop for the client to write to shared memory
  while (1) {
    fprintf(stdout, "Waiting...\n");
    semop(semid, WAIT, 1);
    // Need the first character of shared memory to be null. One message at a
    // time in the buffer.
    if (*shm_start == '\0') {
      fprintf(stdout, "Writing...\n");
      writeToMem(shm_start, SHM_SIZE);
    }
    fprintf(stdout, "Signalling...\n");
    semop(semid, SIGNAL, 1);
  }
}