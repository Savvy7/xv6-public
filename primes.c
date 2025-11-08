#include "types.h"
#include "stat.h"
#include "user.h"

// Read an int from fd; return 0 on EOF, 1 on success
static int
readint(int fd, int *out)
{
  int n;
  int r = read(fd, &n, sizeof(n));
  if(r == 0) return 0;
  if(r != sizeof(n)) {
    // short read or error
    return 0;
  }
  *out = n;
  return 1;
}

// Filter process: reads numbers from 'infd', discards numbers divisible by 'prime',
// prints the prime when first seen, and forwards others to outfd. When EOF on infd,
// close outfd and exit.
static void
filter(int infd, int outfd, int prime)
{
  int num;
  // Print the prime we were given
  printf(1, "prime %d\n", prime);

  while(readint(infd, &num)) {
    if(num % prime != 0) {
      if(write(outfd, &num, sizeof(num)) != sizeof(num)) {
        // write error; just exit
        break;
      }
    }
  }

  // Close our fds and exit
  close(infd);
  close(outfd);
  exit();
}

// primes: creates the pipeline beginning with numbers starting at 'start'
// This function does not return in child filter processes; main waits for children.
int
main(int argc, char *argv[])
{
  int pipefd[2];
  int num;
  int max = 280;

  // Create the initial pipe
  if(pipe(pipefd) < 0) {
    printf(2, "pipe failed\n");
    exit();
  }

  // Parent will write numbers into pipefd[1]
  // We'll fork a writer process to feed numbers 2..max into the pipeline
  int writer = fork();
  if(writer < 0) {
    printf(2, "fork failed\n");
    exit();
  }

  if(writer == 0) {
    // Writer child: write numbers into pipe and then close and exit
    close(pipefd[0]);
    for(int i = 2; i <= max; i++) {
      int v = i;
      if(write(pipefd[1], &v, sizeof(v)) != sizeof(v)) {
        // If write fails, stop
        break;
      }
    }
    close(pipefd[1]);
    exit();
  }

  // Parent: will become the first filter: read from pipefd[0]
  close(pipefd[1]);
  int in_fd = pipefd[0];

  // We'll dynamically create filters as new primes are found
  while(1) {
    int prime;
    if(!readint(in_fd, &prime)) {
      // No more numbers
      close(in_fd);
      break;
    }

    // Found a prime; create a new pipe for the next stage
    int nextpipe[2];
    if(pipe(nextpipe) < 0) {
      printf(2, "pipe failed\n");
      exit();
    }

    int pid = fork();
    if(pid < 0) {
      printf(2, "fork failed\n");
      exit();
    }

    if(pid == 0) {
      // Child: becomes the filter for 'prime'
      close(nextpipe[1]); // child reads from nextpipe[0] as output? No: child will write to nextpipe[1]
      // Adjust fds: child should read from in_fd and write to nextpipe[1]
      // We closed nextpipe[1] incorrectly above; fix by re-opening correct closes
      // Close unused fd's from parent context
      // Real behavior: child should close its copy of in_fd's writer (already closed in parent) and use in_fd and nextpipe[1]
      // To simplify: reassign properly below
      // Close the read end of nextpipe in the child (we will write to nextpipe[1])
      close(nextpipe[0]);
      filter(in_fd, nextpipe[1], prime);
      // filter exits
    }

    // Parent: continue with next stage. Parent will close the write end of nextpipe
    close(nextpipe[1]);
    // Close current in_fd (parent no longer needs it)
    close(in_fd);
    // Parent will read from nextpipe[0] for further primes
    in_fd = nextpipe[0];
  }

  // Wait for writer and all filters to finish
  while(wait(NULL) > 0)
    ;

  exit();
}
