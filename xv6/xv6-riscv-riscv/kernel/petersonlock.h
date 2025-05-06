#define NLOCKS 15

struct peterson_lock {
    int flags[2];       //each process sets its own flags[i] to 1 if it wants to enter the critical section
    int turn;          // if turn == i, it's process i's turn to enter the critical section
    int active;        // if active == 1, the lock is in use
  };
  