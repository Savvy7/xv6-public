#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int ticks;
  
  ticks = uptime();
  
  if(ticks < 0) {
    printf(2, "uptime: error getting uptime\n");
    exit();
  }
  
  // Convert ticks to seconds (assuming 100 ticks per second)
  int seconds = ticks / 100;
  int minutes = seconds / 60;
  int hours = minutes / 60;
  int days = hours / 24;
  
  seconds = seconds % 60;
  minutes = minutes % 60;
  hours = hours % 24;
  
  printf(1, "System uptime: ");
  if(days > 0)
    printf(1, "%d days, ", days);
  if(hours > 0 || days > 0)
    printf(1, "%d hours, ", hours);
  if(minutes > 0 || hours > 0 || days > 0)
    printf(1, "%d minutes, ", minutes);
  printf(1, "%d seconds\n", seconds);
  printf(1, "Total ticks: %d\n", ticks);
  
  exit();
}
