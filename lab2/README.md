# lab2：进程管理——getppid / sched_yield / wait4

新增 3 个进程管理 syscall：`getppid`（父 pid）/ `sched_yield`（主动让 CPU）/ `wait4`（等特定 pid）。链路预填，学生聚焦 `sysproc.c` 实现。

## 怎么开始

1. **先过 lab0 + lab1**（环境就绪 + 理解 syscall 链路）
2. 读 `lab2-实验指导书-v2.md` 了解 struct proc 和任务
3. 按 `AI-CHECKPOINT` 顺序做题：
   - **主线 CP①~③**（必做）：`sys_getppid` → `sys_sched_yield` → `sys_wait4`（都在 `kernel/sysproc.c`）
4. 用 Claude Code 辅助：进目录自动加载 `CLAUDE.md`，AI 先问后写
5. 验收：在 shell 跑 `proctest`，看到 "lab2 test passed"

## 文件说明

| 文件 | 用途 |
|------|------|
| `lab2-实验指导书-v2.md` | 任务说明 + 知识点 + 验收（学生主读） |
| `CLAUDE.md` | AI 行为约束（主线 8 硬规则） |
| `kernel/sysproc.c` | CP①②③ 挖空（sys_getppid / sys_sched_yield / sys_wait4） |
| `kernel/include/sysnum.h` | 3 个新 syscall 号（预填） |
| `kernel/syscall.c` | dispatch（预填） |
| `xv6-user/usys.pl` + `user.h` | stub + 原型（预填） |
| `xv6-user/proctest.c` | 验收程序（跑 `proctest`） |
| `Makefile` | 构建 + 跑 + fs 入口 |

## 退出 qemu

`Ctrl+A` 然后按 `X`。

## 验收

- **主线**：`-> / $ proctest` → "lab2 test passed"（CP①③ 对；CP② yield 手动测）
