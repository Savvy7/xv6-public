# xv6 Mini-Top Project Implementation Report

## Project Overview
This project implements a `top`-like system monitoring tool for xv6, providing real-time information about system uptime, memory usage, and process statistics including CPU utilization.

**Author:** [Your Name]  
**Course:** CS3003 - Operating Systems  
**Date Started:** November 8, 2025

---

## Table of Contents
1. [Phase 1: System Uptime](#phase-1-system-uptime)
2. [Phase 2: Process List System Call](#phase-2-process-list-system-call)
3. [Phase 3: CPU Tracking](#phase-3-cpu-tracking) *(Pending)*
4. [Phase 4: Mini-Top Tool](#phase-4-mini-top-tool) *(Pending)*

---

## Phase 1: System Uptime

### Objective
Verify and test the existing `uptime()` system call in xv6, which returns the number of clock ticks since system boot.

### Implementation Status
✅ **Complete**

### What We Found
The `uptime()` system call was already implemented in xv6:
- **System call number:** `SYS_uptime` (14) defined in `syscall.h`
- **Kernel implementation:** `sys_uptime()` in `sysproc.c`
- **User-space wrapper:** Already present in `usys.S` and `user.h`

### Our Contribution
Created a user-space program `uptime.c` that:
- Calls the `uptime()` system call
- Converts ticks to human-readable format (days, hours, minutes, seconds)
- Displays both formatted time and raw tick count

### Code: `uptime.c`
```c
#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
  int ticks = uptime();
  
  if(ticks < 0) {
    printf(2, "uptime: error getting uptime\n");
    exit();
  }
  
  // Convert ticks to seconds (100 ticks per second in xv6)
  int seconds = ticks / 100;
  int minutes = seconds / 60;
  int hours = minutes / 60;
  int days = hours / 24;
  
  seconds = seconds % 60;
  minutes = minutes % 60;
  hours = hours % 24;
  
  printf(1, "System uptime: ");
  if(days > 0) printf(1, "%d days, ", days);
  if(hours > 0 || days > 0) printf(1, "%d hours, ", hours);
  if(minutes > 0 || hours > 0 || days > 0) printf(1, "%d minutes, ", minutes);
  printf(1, "%d seconds\n", seconds);
  printf(1, "Total ticks: %d\n", ticks);
  
  exit();
}
```

### Files Modified
- `uptime.c` - Created new file
- `Makefile` - Added `_uptime` to `UPROGS`

### Testing
- ✅ Compiled successfully
- ✅ Added to filesystem image

---

## Phase 2: Process List System Call

### Objective
Implement a `ps`-like system call that retrieves information about all running processes in the system.

### Implementation Status
✅ **Complete**

### Design Decisions

#### 1. Data Structure Design
We created a new structure to safely pass process information from kernel space to user space:

```c
// In types.h
struct procinfo {
  int pid;           // Process ID
  int state;         // Process state (enum procstate value)
  uint sz;           // Size of process memory (bytes)
  char name[16];     // Process name
};
```

**Why this design?**
- Separates internal kernel data (`struct proc`) from public API
- Prevents exposing sensitive kernel pointers to user space
- Fixed-size structure enables safe copying

#### 2. Critical Implementation Details

##### **Correct Argument Order**
In `sys_getprocs()`, we must read arguments in the correct order:

```c
int sys_getprocs(void) {
  struct procinfo *pinfo;
  int max;
  
  // 1. Get max FIRST (arg 1) - so we can use it for validation
  if(argint(1, &max) < 0)
    return -1;
    
  // 2. THEN get pointer (arg 0) and validate its size using max
  if(argptr(0, (void*)&pinfo, max * sizeof(struct procinfo)) < 0)
    return -1;
    
  return getprocs(pinfo, max);
}
```

**Why this order matters:**
- We need `max` to validate the size of the pointer
- Reading pointer first would cause undefined behavior (using uninitialized `max`)

##### **Safe Memory Transfer with `copyout()`**

**THE CRITICAL RULE:** The kernel **cannot write directly** to user-space memory.

**Wrong approach (will cause kernel panic):**
```c
// ❌ THIS WILL CRASH THE KERNEL
for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
  pinfo[i].pid = p->pid;  // Writing to user-space pointer!
  // ...
}
```

**Correct approach:**
```c
int getprocs(struct procinfo *pinfo, int max)
{
  struct proc *p;
  int i = 0;
  struct procinfo k_table[NPROC];  // ← Kernel-space buffer
  
  // 1. Populate KERNEL buffer (safe)
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC] && i < max; p++){
    if(p->state == UNUSED)
      continue;
    k_table[i].pid = p->pid;      // ← Writing to kernel memory
    k_table[i].state = p->state;
    k_table[i].sz = p->sz;
    memmove(k_table[i].name, p->name, 16);
    i++;
  }
  release(&ptable.lock);
  
  // 2. Safely copy from kernel to user space
  if(copyout(myproc()->pgdir, (uint)pinfo, k_table, 
             i * sizeof(struct procinfo)) < 0)
    return -1;
    
  return i;
}
```

**Why `copyout()` is necessary:**
- Kernel and user space have separate page tables
- Direct access violates memory protection
- `copyout()` performs safe, validated memory transfer
- Prevents security vulnerabilities

##### **Thread Safety with Locking**

We must hold `ptable.lock` while reading process information:

```c
acquire(&ptable.lock);
// Read process table here
release(&ptable.lock);
```

**Why locking is critical:**
- Multiple CPUs may access `ptable` concurrently
- Processes can be created/destroyed at any time
- Lock ensures we get a consistent snapshot
- Prevents race conditions

### Implementation Details

#### System Call Number
```c
// In syscall.h
#define SYS_getprocs 22
```

#### Kernel Implementation

**File: `proc.c`**
```c
int getprocs(struct procinfo *pinfo, int max)
{
  struct proc *p;
  int i = 0;
  struct procinfo k_table[NPROC];

  if(max <= 0 || max > NPROC)
    return -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC] && i < max; p++){
    if(p->state == UNUSED)
      continue;
    
    k_table[i].pid = p->pid;
    k_table[i].state = p->state;
    k_table[i].sz = p->sz;
    memmove(k_table[i].name, p->name, 16);
    i++;
  }
  release(&ptable.lock);

  if(copyout(myproc()->pgdir, (uint)pinfo, k_table, 
             i * sizeof(struct procinfo)) < 0)
    return -1;
  
  return i;
}
```

**File: `sysproc.c`**
```c
int sys_getprocs(void)
{
  struct procinfo *pinfo;
  int max;
  
  if(argint(1, &max) < 0)
    return -1;
    
  if(argptr(0, (void*)&pinfo, max * sizeof(struct procinfo)) < 0)
    return -1;
    
  return getprocs(pinfo, max);
}
```

#### User-Space Interface

**File: `user.h`**
```c
struct procinfo;  // Forward declaration
int getprocs(struct procinfo*, int);
```

**File: `usys.S`**
```asm
SYSCALL(getprocs)
```

#### Test Program: `ps.c`

```c
#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

char *states[] = {
  "UNUSED", "EMBRYO", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"
};

int main(int argc, char *argv[])
{
  struct procinfo procs[NPROC];
  int n, i;
  
  n = getprocs(procs, NPROC);
  
  if(n < 0) {
    printf(2, "ps: error getting process information\n");
    exit();
  }
  
  printf(1, "PID\tSTATE\t\tMEMORY\t\tNAME\n");
  printf(1, "---\t-----\t\t------\t\t----\n");
  
  for(i = 0; i < n; i++) {
    char *state;
    if(procs[i].state >= 0 && procs[i].state < 6)
      state = states[procs[i].state];
    else
      state = "UNKNOWN";
    
    printf(1, "%d\t%s\t\t%d bytes\t%s\n", 
           procs[i].pid, state, procs[i].sz, procs[i].name);
  }
  
  printf(1, "\nTotal processes: %d\n", n);
  exit();
}
```

### Files Modified

1. **`syscall.h`** - Added `SYS_getprocs` constant
2. **`types.h`** - Added `struct procinfo` definition
3. **`proc.c`** - Implemented `getprocs()` kernel function
4. **`sysproc.c`** - Implemented `sys_getprocs()` wrapper
5. **`syscall.c`** - Registered syscall (extern declaration + table entry)
6. **`usys.S`** - Added assembly wrapper
7. **`user.h`** - Added user-space function declaration
8. **`defs.h`** - Added kernel function declaration
9. **`ps.c`** - Created new user program
10. **`Makefile`** - Added `_ps` to `UPROGS`

### Testing
- ✅ All files compiled without errors
- ✅ `ps` binary successfully created
- ✅ Added to filesystem image
- ✅ Ready for testing in qemu

### Expected Output
When running `ps` in xv6, it should display:
```
PID	STATE		MEMORY		NAME
---	-----		------		----
1	SLEEPING	12288 bytes	init
2	SLEEPING	16384 bytes	sh
...
```

---

## Phase 3: CPU Tracking

### Status
⏳ **Pending Implementation**

### Planned Implementation
1. Add `cpu_ticks` field to `struct proc`
2. Modify timer interrupt in `trap.c` to increment counter
3. Update `procinfo` structure to include CPU ticks
4. Modify `getprocs()` to copy CPU tick data

---

## Phase 4: Mini-Top Tool

### Status
⏳ **Pending Implementation**

### Planned Features
1. Continuous refresh loop
2. CPU percentage calculation
3. Memory information display
4. Formatted output similar to Unix `top`

---

## Key Learnings

### 1. System Call Development Process
Adding a system call in xv6 requires modifying 10+ files in a specific order. Each file serves a critical role in the kernel-to-user-space interface.

### 2. Memory Safety
The kernel/user-space boundary is protected by hardware and requires explicit, safe transfer mechanisms like `copyout()`. Direct memory access violates this protection.

### 3. Concurrency Control
Even simple read operations require proper locking when accessing shared data structures. The `ptable` is a critical resource accessed by multiple CPUs.

### 4. Argument Handling
System call arguments must be carefully validated and read in the correct order. The kernel cannot trust user-space pointers without validation.

### 5. API Design
Separating internal kernel structures from public-facing structures provides:
- Security (don't expose kernel internals)
- Stability (can change kernel internals without breaking API)
- Safety (fixed-size structures enable validation)

---

## References

- xv6 Source Code: [MIT xv6 Repository](https://github.com/mit-pdos/xv6-public)
- xv6 Book: "xv6: a simple, Unix-like teaching operating system"
- System Call Implementation Guide: xv6 Book Chapter 3

---

## Build Instructions

```bash
# Clean previous build
make clean

# Build kernel and user programs
make

# Build filesystem image
make fs.img

# Run in QEMU
make qemu

# Or run without display
make qemu-nox
```

## Testing Commands

```bash
# In xv6 shell:
$ uptime          # Display system uptime
$ ps              # List all processes
```

---

Note: there is a small printing formatting error in the ps command output 

*Last Updated: November 8, 2025 - Phase 2 Complete*
