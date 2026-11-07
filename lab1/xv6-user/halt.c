#include "kernel/include/types.h"
#include "user.h"

// lab1 主线验收程序：调 halt() syscall 关机。
// 学生完成 CP①~④ 后，运行本程序应直接退出 qemu（不打印下面那行）。
// 若看到 "should not reach here"——说明 halt 没真正关机，回去检查 CP①~④。
int
main(void)
{
  halt();
  printf("should not reach here\n");
  return 0;
}
