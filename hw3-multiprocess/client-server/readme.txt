Compile normally, with:

clang -o server.out server.c
clang -o client.out client.c
or
gcc -o server.out server.c
gcc -o client.out client.c

NOTE: If you choose to run it, start the server first, then the client.

The shared memory segment and semaphores may have to be deleted after use manually.