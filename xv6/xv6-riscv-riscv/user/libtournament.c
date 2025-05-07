#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include <stddef.h>
#define MAX_PROCESSES 16

// Global variables
static int *locks = NULL;       // Array to store lock IDs
static int num_processes = 0;   // Number of processes in tournament
static int  tournament_id = -1;  // ID of the current process in the tournament
static int num_levels = 0;      // Number of levels in tournament tree


static int is_power_of_2(int n)  {
  return n > 0 && (n & (n - 1)) == 0;
}
// Calculate log base 2 of n (where n is a power of 2)
static int 
log2_pow2(int n) {
  int result = 0;
  while (n > 1) {
    n >>= 1;
    result++;
  }
  return result;
}
// Get the role for a process at a specific level
static int 
get_role(int index, int level) {
  return (index & (1 << (num_levels - level - 1))) >> (num_levels - level - 1);
}

// Get the lock index in the level
static int 
get_lock_level_index(int index, int level) {
  return index >> (num_levels - level);
}

// Get the absolute lock index in the array
static int 
get_lock_array_index(int level_index, int level) {
  return level_index + (1 << level) - 1;
}

// Free all resources associated with the tournament
static void 
cleanup_tournament() {
  if (locks) {
    // Destroy all locks
    for (int i = 0; i < (num_processes - 1); i++) {
      if (locks[i] >= 0) {
        peterson_destroy(locks[i]);
      }
    }
    free(locks);
    locks = NULL;
  }
}

// Create a new tournament tree
int 
tournament_create(int processes) {
  // Validate number of processes
  if (!is_power_of_2(processes) || processes > MAX_PROCESSES || processes <= 0) {
    return -1;
  }

  // Calculate number of levels
  num_levels = log2_pow2(processes);
  num_processes = processes;

  // Calculate total number of locks needed
  int total_locks = processes - 1; // we need n-1 locks for n processes

  // Allocate memory for lock array
  locks = malloc(total_locks * sizeof(int));
  if (!locks) {
    return -1;
  }

  // Initialize all locks to -1 (invalid)
  for (int i = 0; i < total_locks; i++) {
    locks[i] = -1;
  }

  // Create all Peterson locks
  for (int i = 0; i < total_locks; i++) {
    locks[i] = peterson_create();
    if (locks[i] < 0) {
      cleanup_tournament();
      return -1;
    }
  }

  // Parent process has index 0
   tournament_id = 0;

  // Fork processes and assign indices
  for (int i = 1; i < processes; i++) {
    int pid = fork();
    if (pid < 0) {
      // Fork failed
      return -1;
    } else if (pid == 0) {
      // Child process
       tournament_id = i;
       return  tournament_id;
    }
  }

  return  tournament_id;
}

// Acquire the tournament lock
int 
tournament_acquire(void) {
  if ( tournament_id < 0 ||  tournament_id >= num_processes || !locks) {
    return -1;
  }

  // Acquire locks from bottom to top
  for (int level = num_levels - 1; level >= 0; level--) {
    int role = get_role( tournament_id, level);
    int lock_level_index = get_lock_level_index( tournament_id, level);
    int lock_array_index = get_lock_array_index(lock_level_index, level);
    
    if (peterson_acquire(locks[lock_array_index], role) < 0) {
      // Failed to acquire lock at this level
      // We need to release all locks acquired so far
      for (int l = num_levels - 1; l > level; l--) {
        int r = get_role( tournament_id, l);
        int ll_idx = get_lock_level_index( tournament_id, l);
        int la_idx = get_lock_array_index(ll_idx, l);
        peterson_release(locks[la_idx], r);
      }
      return -1;
    }
  }

  return 0;
}

// Release the tournament lock
int 
tournament_release(void) {
  if ( tournament_id < 0 ||  tournament_id >= num_processes || !locks) {
    return -1;
  }

  // Release locks from top to bottom (reverse order of acquisition)
  for (int level = 0; level < num_levels; level++) {
    int role = get_role( tournament_id, level);
    int lock_level_index = get_lock_level_index( tournament_id, level);
    int lock_array_index = get_lock_array_index(lock_level_index, level);
    
    if (peterson_release(locks[lock_array_index], role) < 0) {
      printf("Failed to release lock at level %d\n", level);
      return -1;
    }
  }

  return 0;
}
  