typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

// Structure to hold process information for user-space
struct procinfo {
  int pid;           // Process ID
  int state;         // Process state (enum procstate value)
  uint sz;           // Size of process memory (bytes)
  char name[16];     // Process name
  uint cpu_ticks;    // Total CPU ticks used by this process
};
