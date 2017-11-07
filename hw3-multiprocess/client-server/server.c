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

int get_shared(int id, size_t size) {
  // Get the unique key
  key_t k = ftok(".", id);
  // Return error
  if (k == -1) {
    perror("Failed to create the key.");
    return -1;
  }

  // Create shared memory segment
  int s = shmget(k, size, IPC_CREAT | 0660);

  if (s == -1) {
    perror("Failed to create the shared memory segment.");
  }
  return s;
}

void delete_shared(int shmid) {
  struct shmid_ds* shmid_ds;
  int cmd = IPC_RMID;
  int result;
  if ((result = shmctl(shmid, cmd, shmid_ds)) == -1) {
    perror("Deletion failed");
  }
}

void read_buffer_to_file(FILE* f, char* shm_start, size_t shm_len,
                         const char* delim, int delim_len) {
  // Initialize our buffer
  char* buf = (char*)malloc(shm_len);

  char* curr = shm_start;
  char tmp = '\0';
  size_t index = 0;
  size_t buffer_len = 0;
  int delim_pos = 0;

  // Run through the shared memory segment.
  for (; index < shm_len; index++) {
    // Test reading
    fputc(curr[index], stdout);
    tmp = (curr[index]);

    // Current char doesn't match current delimiter char
    if ((buf[index] = tmp) != delim[delim_pos]) {
      // Make sure we copy the previously matched characters
      buffer_len += delim_pos + 1;
      delim_pos = 0;
    } else {
      // Fully matched the delimiter. Done!
      if (delim_pos == delim_len - 1) {
        delim_pos = 0;
        break;
      } else {
        // Matched a delimiter character
        delim_pos++;
      }
    }
  }

  // Somehow, we reached the end of the buffer but didn't match the delimiter.
  // Need to copy any matched delimiter characters at the end of the buffer too.
  if (delim_pos < delim_len - 1) {
    buffer_len += delim_pos;
  }

  fprintf(stdout, "%zu", buffer_len);

  // Copy shm to buffer
  strncpy(buf, shm_start, (size_t)buffer_len);
  buf[buffer_len] = '\0';
  fprintf(stdout, "%s", buf);
  // Copy buffer to file
  if (EOF == fputs(buf, f)) {
    perror("Failed to write to file.");
    exit(1);
  }
  fflush(f);

  // Clear out the first character to mark that the memory can be written to
  (*shm_start) = '\0';

  free(buf);
}

int main() {
  char* shm_start;

  int shmid = get_shared(SHM_KEY, SHM_SIZE);

  if (shmid == -1) {
    perror("get_shared()");
    exit(1);
  }

  if ((shm_start = shmat(shmid, NULL, 0)) == (char*)-1) {
    perror("Couldn't attach with shmat");
    delete_shared(shmid);
    exit(1);
  }

  // Null out the array
  memset(shm_start, 0, sizeof(char) * (SHM_SIZE - 1));

  ////
  //// Semaphore creation
  ////

  // create the semaphore
  key_t semkey = ftok(".", SHM_KEY);  // Use the same key as we were using
  int semid = semget(semkey, 1, 0600 | IPC_CREAT);

  // reset initial value
  short one = 1;
  if ((semctl(semid, 0, SETALL, &one)) == -1) {
    perror("Setting semaphores failed");
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

  ////
  //// Get a file
  ////
  FILE* f = fopen("output.txt", "a");

  ////
  //// At this point, the shared memory segment is ready to go.
  //// We now wait for a message to arrive.
  ////

  while (1) {
    fprintf(stdout, "Waiting...\n");
    semop(semid, WAIT, 1);
    // Don't do anything if null at the start.
    if (*shm_start != '\0') {
      fprintf(stdout, "Reading...\n");
      read_buffer_to_file(f, shm_start, SHM_SIZE, "STOP!!!", 7);
    }
    fprintf(stdout, "Signalling...\n");
    semop(semid, SIGNAL, 1);
  }

  return 0;
}
