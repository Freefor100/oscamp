# lab0：环境搭建

搭建 xv6-k210 的 qemu 开发环境，跑通内核看到 shell 提示符。

## 怎么开始

1. 读 `lab0-实验指导书-v2.md`，按第 1.2 节「三步验证」步骤搭环境
2. `make build` 构建内核
3. `make fs` 生成文件系统镜像（需 sudo）
4. `make run` 运行
5. 看到 `-> / $` 提示符 = 成功

## 本 lab 不做的

- **不挖空、不改代码**：本 lab 是环境搭建，没有代码填空任务
- **不要求读懂 xv6**：跑通即可，读懂从 lab1 开始

## 文件说明

| 文件 | 用途 |
|------|------|
| `lab0-实验指导书-v2.md` | 任务步骤（学生主读这份） |
| `CLAUDE.md` | AI 行为约束（用 Claude Code 辅助时自动生效） |
| `kernel/` | xv6-k210 内核源码 |
| `xv6-user/` | 用户态程序（sh/ls/cat...） |
| `Makefile` | 构建 + 跑 + 文件系统镜像入口 |
| `linker/` | 链接脚本（`qemu.ld` / `k210.ld`） |
| （无 `bootloader/`） | 本 lab 用 qemu 自带 OpenSBI，不依赖单独编译的 SBI 二进制 |
| `doc/` | xv6-k210 原始文档（参考） |

## 退出 qemu

`Ctrl+A` 然后按 `X`。

## 验收标准

1. `make build` 无报错
2. `make run` 启动后看到 `-> / $` 提示符
3. 在 shell 里输入 `ls` 有文件列表输出

做到这三条，lab0 过。
