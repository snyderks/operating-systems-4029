#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILE_SIZE 11
#define NO_PROC 10

int DelayCount = 0;

int readerID = 0;
int writerID = 0;

int parent = 1;  // True = 1

char* shared_buffer;
int* reader_count;

int Delay100ms = 0;

int semaid = -1;

/*-------------------------------------------
    Delay routines
 --------------------------------------------*/
void basic_delay() {
  long i, j, k;
  for (i = 0; i < 200L; i++) {
    for (j = 0; j < 400L; j++) {
      k = k + i;
    }
  }
}

/* do some delays (in 100 ms/tick) */
void delay(int delay_time) {
  int i, j;

  for (i = 0; i < delay_time; i++) {
    for (j = 0; j < Delay100ms; j++) {
      basic_delay();
    }
  }
}

int stop_alarm = 0;

static void sig_alrm(int signo) { stop_alarm = 1; }

/*------------------------------------------
 * Since the speed of different systems vary,
 * we need to calculate the delay factor
 *------------------------------------------
 */
void calculate_delay() {
  int i;
  struct tms t1;
  struct tms t2;
  clock_t t;
  long clktck;
  double td;

  printf(".... Calculating delay factor ......\n");
  stop_alarm = 0;
  if (signal(SIGALRM, sig_alrm) == SIG_ERR) perror("Set SIGALRM");
  alarm(5); /* stop the following loop after 5 seconds */

  times(&t1);
  while (stop_alarm == 0) {
    DelayCount++;
    basic_delay();
  }
  times(&t2);
  alarm(0); /* turn off the timer */

  /* Calcluate CPU time */
  t = t2.tms_utime - t1.tms_utime;

  /* fetch clock ticks per second */
  if ((clktck = sysconf(_SC_CLK_TCK)) < 0) perror("sysconf error");

  /* actual delay in seconds */
  td = t / (double)clktck;

  Delay100ms = DelayCount / td / 10;

  if (Delay100ms == 0) Delay100ms++;

  printf(".... End calculating delay factor\n");
}

// Union semaphore structure
// union semun {
//      int val; /* used for SETVAL only */
//      struct semid_ds *buf; /* for IPC_STAT and IPC_SET, not discussed here */
//      ushort *array; /* used for GETALL and SETALL */
//};

// Semaphore operation structure
// struct sembuf {
//      int sem_num; /* member # in {0, . . . , numsems - 1} */
//      short sem_op; /* operation: negative, zero, positive */
//      short sem_flag; /* IPC_NOWAIT or SEM_UNDO */
//};

// Define operations
struct sembuf READ_WAIT[1], READ_SIGNAL[1], WRITE_WAIT[1], WRITE_SIGNAL[1];

/*-------------------------------------------
   The reader
 --------------------------------------------*/
void reader() {
  int i, j, n;
  char results[FILE_SIZE];

  srand(2);
  for (i = 0; i < 1; i++) {
    printf("Reader %d (pid = %d) arrives\n", readerID, getpid());

    // Lock the writers
    // ENTER CRITICAL SECTION
    semop(semaid, READ_WAIT, 1);
    (*reader_count)++;
    // If we're the first reader, lock the writers
    if (*reader_count == 1) {
      semop(semaid, WRITE_WAIT, 1);
    }
    // EXIT CRITICAL SECTION
    semop(semaid, READ_SIGNAL, 1);

    /* read data from shared data */
    for (j = 0; j < FILE_SIZE; j++) {
      results[j] = shared_buffer[j];
      delay(4);
    }

    // Unlock the writers
    // ENTER CRITICAL SECTION
    semop(semaid, READ_WAIT, 1);
    (*reader_count)--;
    // If we were the only reader, unlock the writers
    if (*reader_count == 0) {
      semop(semaid, WRITE_SIGNAL, 1);
    }
    // EXIT CRITICAL SECTION
    semop(semaid, READ_SIGNAL, 1);

    /* display result */
    results[j] = 0;
    printf("      Reader %d gets results = %s\n", readerID, results);
  }
}

/*-------------------------------------------
   The writer. It tries to fill the buffer
   repeatedly with the same digit
 --------------------------------------------*/
void writer() {
  int i, j, n;
  char data[FILE_SIZE];

  srand(1);

  for (j = 0; j < FILE_SIZE - 1; j++) {
    data[j] = writerID + '0';
  }
  data[j] = 0;

  for (i = 0; i < 1; i++) {
    printf("Writer %d (pid = %d) arrives, writing %s to buffer\n", writerID,
           getpid(), data);

    // Enter critical section
    semop(semaid, WRITE_WAIT, 1);
    printf("%i", semaid);
    printf("Currently writing. Number of readers is %i\n", *reader_count);
    union semun arg;
    printf("%i\n", semctl(semaid, 1, GETVAL, arg));

    /* write to shared buffer */
    for (j = 0; j < FILE_SIZE - 1; j++) {
      shared_buffer[j] = data[j];
      delay(3);
    }

    // Exit critical section
    semop(semaid, WRITE_SIGNAL, 1);

    printf("Writer %d finishes\n", writerID);
  }
}

/*-------------------------------------------

      Routines for creating readers and writers

*-------------------------------------------*/
void create_reader() {
  if (0 == fork()) {
    parent = 0;
    reader();
    exit(0);
  }

  readerID++;
}

void create_writer() {
  if (0 == fork()) {
    parent = 0;
    writer();
    exit(0);
  }

  writerID++;
}

/*-------------------------------------------
 --------------------------------------------*/
int main() {
  int return_value;
  char InitData[] = "0000000000\n";
  int i;
  int fd;

  calculate_delay();

  /* Create the semaphore structure
   * (first will be for readers, second for writers)
   */
  key_t key = ftok(".", 0);
  semaid = semget(key, 2, IPC_CREAT | 0600);

  if (semaid == -1) {
    // error occurred
    printf("Failed to create the semaphores.\n");
    exit(1);
  }
  printf("semaid: %i\n", semaid);

  union semun arg;
  arg.val = 1;

  // Set the semaphores to 1
  if (semctl(semaid, 0, SETVAL, arg)) {
    printf("Failed to write to semaphore 0\n");
    exit(1);
  }
  if (semctl(semaid, 1, SETVAL, arg)) {
    printf("Failed to write to semaphore 1\n");
    exit(1);
  }

  int sval = semctl(semaid, 0, IPC_RMID, arg);
  int sval2 = semctl(semaid, 1, IPC_RMID, arg);
  printf("Initial value of read: %i\nAnd write: %i\n", sval, sval2);

  // Defining WAIT
  READ_WAIT[0].sem_num = 0;
  READ_WAIT[0].sem_op = -1;
  READ_WAIT[0].sem_flg = SEM_UNDO;

  WRITE_WAIT[0].sem_num = 1;
  WRITE_WAIT[0].sem_op = -1;
  WRITE_WAIT[0].sem_flg = SEM_UNDO;

  // Defining SIGNAL
  READ_SIGNAL[0].sem_num = 0;
  READ_SIGNAL[0].sem_op = 1;
  READ_SIGNAL[0].sem_flg = SEM_UNDO;

  WRITE_SIGNAL[0].sem_num = 1;
  WRITE_SIGNAL[0].sem_op = 1;
  WRITE_SIGNAL[0].sem_flg = SEM_UNDO;

  /*-------------------------------------------------------

       The following code segment creates a memory
     region shared by all child processes
       If you can't completely understand the code, don't
     worry. You don't have to understand the detail
     of mmap() to finish the homework

  -------------------------------------------------------*/

  fd = open("race.dat", O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    perror("race.dat ");
    exit(1);
  }

  write(fd, InitData, FILE_SIZE);

  unlink("race.dat");

  shared_buffer = mmap(0, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  reader_count = mmap(0, sizeof(*reader_count), PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *reader_count = 0;
  if (shared_buffer == (caddr_t)-1 || reader_count == (caddr_t)-1) {
    perror("mmap");
    exit(2);
  }

  /*-------------------------------------------------------

       Create some readers and writes (processes)

  -------------------------------------------------------*/
  create_reader();
  delay(1);
  create_writer();
  delay(1);
  create_reader();
  create_reader();
  create_reader();
  delay(1);
  create_writer();
  delay(1);
  create_reader();

  /* delay 15 seconds so all previous readers/writes can finish.
   * This is to prevent writer starvation
   */
  delay(150);

  create_writer();
  delay(1);
  create_writer();
  delay(1);
  create_reader();
  create_reader();
  create_reader();
  delay(1);
  create_writer();
  delay(1);
  create_reader();

  /*-------------------------------------------------------

      Wait until all children terminate

  --------------------------------------------------------*/
  for (i = 0; i < (readerID + writerID); i++) {
    wait(NULL);
  }
  // if (parent == 1) {
  //   semctl(semaid, 0, IPC_RMID, arg);
  // }
  return 0;
}
