# lab1：syscall + Trap 链路——加 SYS_halt 关机

新增一个关机 syscall（`SYS_halt`），走完整的 syscall 链路：用户态 `ecall` → trap → 内核 dispatch → 实现 → SBI 关机。

## 怎么开始

1. **先过 lab0**（环境搭好，能 `make run` 看到 `-> / $`）
2. 读 `lab1-实验指导书-v2.md` 了解 syscall 链路和任务
3. 按 `AI-CHECKPOINT` 顺序做题：
   - **主线 CP①~④**（必做）：`sysnum.h` → `usys.pl` → `syscall.c` → `sysproc.c`
4. 用 Claude Code 辅助：进目录自动加载 `CLAUDE.md`，AI 会先反问你、答对才写代码
5. 验收：在 shell 跑 `halt` 命令，qemu 真的退出

## 文件说明

| 文件 | 用途 |
|------|------|
| `lab1-实验指导书-v2.md` | 任务说明 + 知识点 + 验收标准（学生主读这份） |
| `CLAUDE.md` | AI 行为约束（主线 9 条硬规则 + 分层引导） |
| `kernel/include/sysnum.h` | CP①：syscall 号定义 |
| `xv6-user/usys.pl` | CP②：用户态 stub 生成器 |
| `kernel/syscall.c` | CP③：dispatch 表 |
| `kernel/sysproc.c` | CP④：sys_halt 实现 |
| `xv6-user/halt.c` | 主线验收程序（跑 `halt` 命令） |
| `Makefile` | 构建 + 跑 + fs 入口 |

## 退出 qemu

`Ctrl+A` 然后按 `X`。

## 验收

- **主线**：`-> / $ halt` → qemu 退出（CP①~④ 全过）
