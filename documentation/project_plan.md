
-----

### 🚀 Feasible Functionalities & Difficulty Rating

| Functionality | Implementation Sketch (What you'd do) | Difficulty |
| :--- | :--- | :--- |
| **1. System Uptime** | Add a `sys_uptime()` system call. In the kernel, it just needs to read the global `ticks` variable (which increments on every timer interrupt) and return it. | **Easy** |
| **2. System Memory Info** | Add a `sys_meminfo()` system call. In the kernel, you'd write a function (e.g., `getfreemem()`) that counts the number of free pages in the `kalloc` free list. The syscall would return this count (and maybe the total physical memory). | **Medium** |
| **3. Basic Process List** | This is the classic `ps` project. Create a `sys_getprocs()` system call. It would loop over the `ptable` (process table), acquire the `ptable.lock`, and copy basic info (PID, state, name) for each process into a user-supplied buffer. | **Medium** |
| **4. Per-Process Memory** | You can add this to your `sys_getprocs()` call. For each process, just copy out the `p->sz` (size) field from its `proc` struct. This is its user-space memory size. | **Easy** (once \#3 is done) |
| **5. Per-Process CPU %** | This is the hardest part. `xv6` doesn't track this. You'd need to: <br> 1. Add a new field to the `proc` struct, like `cpu_ticks`. <br> 2. In the timer interrupt (`trap.c`), increment `p->cpu_ticks` for the currently `RUNNING` process. <br> 3. Your `sys_getprocs()` call returns this raw `cpu_ticks` value. <br> 4. Your user-space tool must calculate the *percentage* (see below). | **Hard** |
| **6. The `mini-top` Tool** | This is a user-space C program. It runs in a loop. Inside the loop, it: <br> 1. Calls all your new system calls. <br> 2. Formats and prints the data to the console. <br> 3. Calls `sleep()` for 1 second. <br> 4. To "clear" the screen, it would just print 25 newline characters, or you could add a simple console escape code. | **Medium** |

-----

### 💡 The Hardest Part: Calculating CPU %

The most complex feature is CPU percentage. You can't just *ask* the kernel for this, because it's a **rate**, not a static value. It's the amount of CPU time a process used over a *period of time*.

Your user-space `mini-top` tool will have to do this calculation:

1.  **Time 1:** Call `sys_getprocs()` and `sys_uptime()`. Store the `cpu_ticks` for every process (e.g., `pid_5_ticks_last = 150`) and the total system `ticks` (e.g., `total_ticks_last = 1000`).
2.  `sleep(1)` (which is 100 ticks by default in `xv6`).
3.  **Time 2:** Call `sys_getprocs()` and `sys_uptime()` again. Get the new values (e.g., `pid_5_ticks_now = 170` and `total_ticks_now = 1100`).
4.  **Calculate the difference:**
      * `proc_delta = pid_5_ticks_now - pid_5_ticks_last` (which is `170 - 150 = 20`)
      * `total_delta = total_ticks_now - total_ticks_last` (which is `1100 - 1000 = 100`)
5.  **Find the percentage:**
      * `cpu_percent = (proc_delta / total_delta) * 100` (which is `(20 / 100) * 100 = 20%`)

This logic (modifying the interrupt handler and making the user-space tool "stateful") is what makes this a challenging and very rewarding project.

### Recommended Project Phases

1.  **Phase 1:** Implement the easiest syscall: `uptime()`. Write a simple user program to call it and print the result. This proves you can add a syscall.
2.  **Phase 2:** Implement the `ps`-like syscall (`sys_getprocs()`) to get the process list (PID, name, state, memory size). Write a `ps` tool that prints this list once and exits.
3.  **Phase 3:** Tackle the CPU tracking. Modify the `proc` struct and the `trap.c` timer interrupt. Add the `cpu_ticks` data to your `sys_getprocs()` call.
4.  **Phase 4:** Write the final `mini-top` user-space tool that runs in a loop, calculates the CPU percentage, and formats all the data.

This is a fantastic project. You'll learn about system calls, kernel data structures, locking, interrupt handling, and the user/kernel boundary.

Would you like me to outline the basic steps for creating your very first system call, like the `uptime()` one?