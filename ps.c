#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

// State names corresponding to enum procstate in proc.h
char *states[] = {
  "UNUSED",   // 0
  "EMBRYO",   // 1
  "SLEEPING", // 2
  "RUNNABLE", // 3
  "RUNNING",  // 4
  "ZOMBIE"    // 5
};

int
main(int argc, char *argv[])
{
  struct procinfo procs[NPROC];
  int n, i;
  
  // Call the getprocs system call
  n = getprocs(procs, NPROC);
  
  if(n < 0) {
    printf(2, "ps: error getting process information\n");
    exit();
  }
  
  // Print header
  printf(1, "PID\tSTATE\t\tCPU_TICKS\tMEMORY\t\tNAME\n");
  printf(1, "---\t-----\t\t---------\t------\t\t----\n");
  
  // Print each process
  for(i = 0; i < n; i++) {
    char *state;
    
    // Get state string
    if(procs[i].state >= 0 && procs[i].state < 6)
      state = states[procs[i].state];
    else
      state = "UNKNOWN";
    
    // Print process info
    printf(1, "%d\t%s\t\t%d\t\t%d bytes\t%s\n", 
           procs[i].pid, 
           state,
           procs[i].cpu_ticks,
           procs[i].sz, 
           procs[i].name);
  }
  
  printf(1, "\nTotal processes: %d\n", n);
  
  exit();
}
