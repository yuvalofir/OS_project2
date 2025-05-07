#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: tournament <number_of_processes>\n");
    printf("Number of processes must be a power of 2 (2, 4, 8, or 16)\n");
    exit(1);
  }
  
  int num_processes = atoi(argv[1]);
  printf("Starting tournament with %d processes\n", num_processes);
  // (num_processes & (num_processes - 1)) == 0 checks if num_processes is a power of 2
  if(num_processes > 16 || num_processes < 2 || (num_processes & (num_processes - 1)) != 0) {
    printf("Number of processes must be a power of 2 (2, 4, 8, or 16)\n");
    exit(1);
  }
  //make sure all the locks are destroyed
  for (int i = 0; i < 15; i++) {
      peterson_destroy(i);
  }


  // Create tournament tree
  int id = tournament_create(num_processes);
  if (id < 0) {
    printf("Create tournament tree was failed\n");
    exit(1);
  }
  
  // Acquire tournament lock
  if (tournament_acquire() < 0) {
    printf("Process %d: failed to acquire tournament lock\n", id);
    exit(1);
  }
  
  // Critical section
  printf("Process PID %d with tournament ID %d is in the critical section\n", getpid(), id);
  
  
  // Release tournament lock
  if (tournament_release() < 0) {
    printf("Process %d: failed to release tournament lock\n", id);
    exit(1);
  }

  if(id == 0) {
    // Parent process
    for (int i = 0; i < num_processes; i++)
    {
      wait(0);
    }
    
  } 
  
  exit(0);
}