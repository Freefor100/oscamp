#include "kernel/include/types.h"
#include "user.h"

// lab2 主线验收：CP① getppid + CP② sched_yield + CP③ wait4。
// 只断言稳定语义，不假设 yield 后一定先运行其他进程。
int
main(void)
{
  int failed = 0;
  int parent_pid = getpid();
  int parent_ppid = getppid();

  if(parent_ppid <= 0){
    printf("getppid FAILED: parent ppid=%d\n", parent_ppid);
    failed = 1;
  }

  if(sched_yield() != 0){
    printf("sched_yield FAILED: expected 0\n");
    failed = 1;
  }

  int pid = fork();
  if(pid < 0){
    printf("fork FAILED\n");
    exit(1);
  }

  if(pid == 0){
    int child_ppid = getppid();
    if(child_ppid != parent_pid){
      printf("getppid FAILED: child ppid=%d, expected %d\n",
             child_ppid, parent_pid);
      exit(41);
    }
    if(sched_yield() != 0){
      printf("sched_yield FAILED in child\n");
      exit(41);
    }
    exit(42);
  }

  int status = -1;
  int reaped = wait4(pid, &status);
  if(reaped != pid || status != 42){
    printf("wait4 FAILED: reaped=%d expected=%d, status=%d expected=42\n",
           reaped, pid, status);
    failed = 1;
  }

  if(failed){
    printf("lab2 test FAILED\n");
    exit(1);
  }

  printf("lab2 test passed\n");
  exit(0);
}
