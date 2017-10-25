// Homework 3
// Operating Systems, Dr. Yiming Hu
// Completed by Kristian Snyder

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// The shortest email address must include a name, @, and a domain, each of
// which can be 1 character. Therefore, the minimum length is 3 characters.
const int min_email_addr_len = 3;

// Validate the arguments
// If successful, returns the argument containing the email address.
// Returns -1 on failure.
int checkArgs(int argc, char** argv);

// Reads all files in the current directory and stores them in the returned list
// of strings.
// The length of the list is stored in the first element of the returned list.
// Size is -1 on failure.
// If failure occurs, return value will be freed and set to NULL.
// You are responsible for freeing the returned linked list.
struct linked_string* getFiles();

// Looks up all running processes and stores them in the returned list
// of strings.
// The length of the list is stored in the first element of the returned list.
// Size is -1 on failure.
// If failure occurs, return value will be freed and set to NULL.
// You are responsible for freeing the returned linked list.
struct linked_string* getProcesses();

// Frees all memory pointed to by the linked list.
void free_list(struct linked_string* root);

////
//// Linked List
////

// A struct to hold an element of a linked list.
struct linked_string {
  struct linked_string* next;
  char* content;
  int size;
};

// Add an element to the end of a linked list.
void add_el(struct linked_string* head, const char* str, int len);

////
//// Implementations
////

int main(int argc, char** argv) {
  int result = checkArgs(argc, argv);
  if (result == -1) {
    exit(1);
  }

  char* email = argv[result];

  int test = 0;

  struct linked_string* files = getFiles();
  struct linked_string* procs = getProcesses();

  // Open a file to write to
  FILE* f = fopen("draft.txt", "w");

  struct linked_string* curr = files;

  while ((curr = curr->next) != NULL) {
    // Write a string and newline
    fputs(curr->content, f);
    putc('\n', f);
  }

  putc('\n', f);

  curr = procs;

  while ((curr = curr->next) != NULL) {
    // Write a string and newline
    fputs(curr->content, f);
    putc('\n', f);
  }

  fclose(f);

  // Holds subject line
  char subject[256] = {'\0'};

  int len = sprintf(subject, "There are %i files and %i processes", files->size,
                    procs->size);

  execl("/bin/mailx", "/bin/mailx", "-s", subject, email, "<", "draft.txt",
        (char*)NULL);

  remove("draft.txt");

  free_list(files);
  files = NULL;
  free_list(procs);
  procs = NULL;

  return 0;
}

int checkArgs(int argc, char** argv) {
  // Should have only
  if (argc != 2) {
    printf("mailer should be executed in the format: mailer email_address.\n");
    return -1;
  }

  // Only real check that can be done to check the email for validation.
  if (strlen(argv[1]) <= min_email_addr_len) {
    printf("Your email address is too short. Please try again.\n");
    return -1;
  }

  // Success!
  return 1;
}

////
//// Helper functions
////

// Reads all files in the current directory and stores them in the returned list
// of strings.
// The length of the list is stored in the first element of the returned list,
// which is a header.
// Size is -1 on failure.
// If failure occurs, return value will be freed and set to NULL.
// You are responsible for freeing the returned linked list.
struct linked_string* getFiles() {
  // The directory stream we're enumerating through
  DIR* d;
  // Contains information about the directory
  struct dirent* directory;

  // Access the current directory
  d = opendir(".");

  // Create an initial linked list
  struct linked_string* root =
      (struct linked_string*)malloc(sizeof(struct linked_string));

  root->content = "List of files";
  root->size = 0;
  root->next = NULL;

  // Make sure we opened it successfully
  if (d) {
    // Keep going until we've gone through the whole directory
    while ((directory = readdir(d)) != NULL) {
      if (directory->d_type == DT_REG) {
        // Add the current file to the list.
        add_el(root, directory->d_name, strlen(directory->d_name));
      }
    }
  } else {
    free(root);
    return NULL;
  }

  return root;
}

// Looks up all running processes and stores them in the returned list
// of strings.
// The length of the list is stored in the first element of the returned list.
// Size is -1 on failure.
// If failure occurs, return value will be freed and set to NULL.
// You are responsible for freeing the returned linked list.
struct linked_string* getProcesses() {
  // Open a piped command using popen()
  FILE* f = popen("ps -A -o pid,command=", "r");

  // Create an initial linked list
  struct linked_string* root =
      (struct linked_string*)malloc(sizeof(struct linked_string));

  root->content = "List of process PIDs and names";
  root->size = 0;
  root->next = NULL;

  size_t buffer_size = 128;
  char* buffer = (char*)malloc((sizeof(char)) * buffer_size);
  if (buffer == NULL) {
    free(root);
    return NULL;
  }
  int line_len = -1;

  while ((line_len = getline(&buffer, &buffer_size, f)) != -1) {
    // Allocate a string 1 character smaller than the line read
    char* line = (char*)malloc((sizeof(char)) * (line_len));
    // Copy everything but the new line and null-terminator
    //(dest string will be null-terminated) by manpage of strlcpy
    strlcpy(line, buffer, line_len);
    // Append to linked list and free the line buffer
    add_el(root, line, line_len - 1);
    free(line);
  }

  pclose(f);
  free(buffer);

  return root;
}

void add_el(struct linked_string* head, const char* str, int len) {
  struct linked_string* curr = head;
  curr->size++;
  while (curr->next != NULL) {
    curr = curr->next;
  }
  // Copy the string
  struct linked_string* el_copy =
      (struct linked_string*)malloc(sizeof(struct linked_string));
  size_t slen = strlen(str);
  if (len != slen) {
    printf("Mismatch: %i != %zu", len, slen);
    return;
  }
  el_copy->content = (char*)malloc(sizeof(char) * slen + 1);
  strlcpy(el_copy->content, str, slen + 1);

  el_copy->size = 0;
  el_copy->next = NULL;

  // Add the element to the end
  curr->next = el_copy;
}

void free_list(struct linked_string* root) {
  if (root != NULL) {
    // temporarily store the next element
    struct linked_string* next = root->next;
    // free up the current element's memory
    free(root);
    // move to the next element
    free_list(next);
  }
}