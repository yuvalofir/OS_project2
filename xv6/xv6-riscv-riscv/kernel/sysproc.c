#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "petersonlock.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
extern struct peterson_lock peterson_locks[NLOCKS];
uint64
sys_peterson_create(void)
{
  for (int i = 0; i < NLOCKS; i++) {
    if (__sync_lock_test_and_set(&peterson_locks[i].active, 1) == 0) {
      peterson_locks[i].flags[0] = 0;
      peterson_locks[i].flags[1] = 0;
      peterson_locks[i].turn = 0;

      __sync_synchronize(); 

      return i;
    }
  }

  return -1;
}
uint64
sys_peterson_acquire(void)
{
  int lock_id, role;
  argint(0, &lock_id);
  argint(1, &role);

  if (lock_id < 0 || lock_id >= NLOCKS || (role != 0 && role != 1)) // lock identifier or the role is invalid
    return -1;

  struct peterson_lock *lock = &peterson_locks[lock_id];
  // Check if the lock is active, if not return -1
  if (!lock->active)
    return -1;

  lock->flags[role] = 1; //similar to true
  __sync_synchronize(); //using after write

  lock->turn = role; // set turn to the current process
  
  __sync_synchronize();//using after write and before read

  while (lock->flags[1 - role] && lock->turn == role)
    yield(); // instead of busy waiting, yield gives up the CPU

  return 0;
}

uint64
sys_peterson_release(void)
{
  int lock_id, role;
  argint(0, &lock_id);
  argint(1, &role);
  
  if (lock_id < 0 || lock_id >= NLOCKS || (role != 0 && role != 1)) // lock identifier or the role is invalid
    return -1;

  struct peterson_lock *lock = &peterson_locks[lock_id]; // get the lock

  if (!lock->active)
    return -1;

  lock->flags[role] = 0; //false

  __sync_synchronize(); 

  return 0;
}

uint64
sys_peterson_destroy(void)
{
  int lock_id;
  argint(0, &lock_id);
  
  if (lock_id < 0 || lock_id >= NLOCKS) // lock identifier is invalid
    return -1;

  struct peterson_lock *lock = &peterson_locks[lock_id];

  if (!lock->active)
    return -1;
    
  //reset all the values of the lock 
  lock->flags[0] = 0;
  lock->flags[1] = 0;
  lock->turn = 0;

  __sync_synchronize();  // make sure all operations are completed before releasing the lock

  __sync_lock_release(&lock->active);  // release the lock safely and atomically

  return 0;
}