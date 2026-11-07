# 课程提交客户端 deb 包分析

分析对象（静态解包，未安装、未执行）：

| 文件 | 大小 | 包名 | 版本 |
|---|---|---|---|
| `提交软件v1.1.2-Vmware发布版/os_basecamp_submission_client_1.1.2_all.deb` | 51.3 KB | `os-basecamp-submission-client` | 1.1.2 |
| `自建虚拟机的作业上交方法/os_basecamp_submission_client_1.1.1-VMware_all.deb` | 46.4 KB | `os-basecamp-submission-client-vmware` | 1.1.1 |

两个包的 SHA-256 均校验通过。

---

## 一、一句话结论

**这是课程方的「终端录制 + 作业提交」客户端**，纯 Python、无二进制、无混淆、无 setuid，
唯一外部出口是 `https://xv6.kuafuai.top`。功能上它做三件事：录你终端里的一切、读你的
Claude 对话、只取 `kernel/` 和 `xv6-user/` 源码，打成一个 ZIP 传到课程服务器。

代码本身写得相当克制（有完整性校验、路径穿越防护、原子写入、可回滚卸载），
**但它的采集范围比 README 描述的要大**——尤其是终端输出的完整录制。见第四节。

---

## 二、包内构成

`Architecture: all`，全部是文本文件，**没有任何编译二进制**：

```
/usr/bin/os_basecamp_client                  # 3 行 shell 入口
/usr/lib/os_basecamp_submission/os_basecamp_client/
    __init__.py  __main__.py  config.py  cli.py  gui.py  api.py
    capture.py        33 KB   PTY 录制器 + 控制通道
    archive.py         8 KB   ZIP 打包与校验
    source_filter.py   5 KB   源码筛选
    qa.py  qa_text.py         Claude 会话提取
    claude_install.py 13 KB   claude 命令劫持
    claude_launcher.py 4 KB
    claude_wrapper.sh  215 B
    user_integration.py 14 KB 改用户 .bashrc / 桌面图标
    install.py        13 KB   系统级安装
    install_vmware.py 13 KB   VMware 版安装
    image_reset.py    23 KB   镜像重置（清凭据、清痕迹）
    runtime.py  terminal_text.py  source_filter.py
    shell_hook.sh      3 KB   bash 钩子
    bash_preexec.sh   22 KB   第三方 bash-preexec（附 LICENSE）
```

依赖：`python3 (>= 3.10), zenity, yad, bash (>= 5.1), ca-certificates`

---

## 三、它到底做什么

### 1. 接管你的终端，全程录制

`user_integration.py` 会在你的 `~/.bashrc`（以及 `.bash_profile` / `.bash_login` / `.profile`）
末尾插入一段带 `# BEGIN OS_BASECAMP_SUBMISSION` 标记的 hook。

之后**每开一个新终端**，`shell_hook.sh` 会：

```bash
exec "$_osbc_command" record -- /bin/bash --login -i
```

即用 `os_basecamp_client` 把 bash 包起来，用 `pty.fork()` 开一个伪终端，
把子进程的输出**原始字节**写进 `output.bin`，同时写 `timing.jsonl`（时间戳）
和 `events.jsonl`（命令事件）。这是 asciinema 式的完整录制，不是只记命令。

而且钩子里明确做了这几件事：

```bash
HISTCONTROL=          # 清空：取消 ignorespace / ignoredups
HISTIGNORE=           # 清空：取消你自定义的忽略规则
set -o history        # 强制开历史
shopt -s cmdhist lithist
```

**含义：`前面加空格不记历史` 这种技巧被主动关掉了**，每条命令都会被记录
（命令文本、cwd、返回码）。

### 2. 读你的 Claude 对话

从 `~/.claude/projects` 下的会话 JSONL 里提取，转成 Markdown，
按 `学生` / `Claude` 分角色输出。关键设计：

- 只导出**本轮实验期间新增**的消息。每轮开始时记基线 `size / dev / ino / tail_sha256`，
  下一轮从基线往后读；若发现文件被截断或替换（校验不匹配），该文件**整份跳过不导出**。
- 也就是说：**往期的 Claude 对话不会被上传**，但如果本轮内你改动了历史文件，本轮内容可能整份不导。

### 3. 只打包源码的两个目录

`source_filter.py` 写死了：

```python
SOURCE_DIRECTORIES = ("kernel", "xv6-user")
SOURCE_SUFFIXES = (".c", ".h", ".cc", ".cpp", ".hpp", ".s", ".inc", ".ld", ".sh", ".pl", ".mk")
SKIP_DIRECTORIES = (".git", "__pycache__", "target", "build")
```

外加 `makefile` / `license` / `copying` 等少数文件名。还要做文本检测：
含 `\0` 或 `\x7fELF` 开头的文件一律排除（即**不传编译产物**）。
所选目录若不同时含 `kernel/` 和 `xv6-user/`，直接报错拒绝打包。

### 4. 打包 → 桌面 ZIP → 上传

ZIP 结构：

```
submission.json     学号、姓名、Lab、轮次、时间、截点信息、诊断
manifest.json       每个文件的 size + sha256
SOURCE/LabN/        kernel/ 与 xv6-user/ 的源码
COMMAND/            commands.jsonl + commands.md（命令与输出）
QA/                 按 Lab 分类的问答 Markdown
```

落到桌面 `学号_姓名_时间戳.zip`，然后：

1. 先 POST `/api/eligibility` 校验学号姓名是否在名单里
2. 再 POST `/api/submissions` 上传 ZIP，带 `X-Submission-ID` 和 `X-Archive-SHA256`
3. 要求服务器回执里 `status == "archived"`，且 7 个字段逐一比对一致，否则报"未确认归档"

### 5. 劫持 `claude` 命令

`claude_install.py` 会找到你真实的 `claude`（`~/.local/bin/claude`、
`/usr/local/bin/claude`、`~/.nvm/versions/node/*/bin/claude` 等），
在原位置换成 wrapper：

```sh
exec /usr/bin/os_basecamp_client claude-run --real <真实路径> -- "$@"
```

目的见 `capture.py` 里的 `"claude_output_policy": "entrypoint_suspend"`：
**Claude 前台运行期间，终端的 PTY 输出暂停采集**（那部分改由第 2 条从 JSONL 提取），
避免同一份对话被录两遍。

---

## 四、采集范围（重点）

| 内容 | 是否进提交包 | 说明 |
|---|---|---|
| 终端**命令文本** | 是 | 每条命令 + cwd + 返回码 |
| 终端**屏幕输出** | 是 | PTY 全量录制，清洗控制序列后写入 `commands.md` |
| Claude 对话内容 | 是（仅本轮） | 基线机制，往期不导 |
| Claude 前台运行时的终端输出 | 否 | `capture_suspended`，避免重复 |
| `kernel/`、`xv6-user/` 源码 | 是 | 仅文本、仅指定后缀 |
| 编译产物 / `.git` / `target` | 否 | 明确排除 |
| 桌面其他文件、家目录其他文件 | 否 | 不在打包路径内 |
| 屏幕截图 / 摄像头 / 键盘全局钩子 | 否 | 代码里没有 |

**注意事项：`commands.md` 对输出内容不做任何脱敏**。
`image_reset.py` 里那套 `ANTHROPIC_API_KEY|ANTHROPIC_AUTH_TOKEN|OPENAI_API_KEY...` 的正则
是**重置镜像时清理旧凭据**用的，不是提交前脱敏。

所以：只要你在终端里 `cat ~/.claude/settings.json`、`export ANTHROPIC_AUTH_TOKEN=sk-...`
或 `echo` 出任何密钥，**它就会明文进入提交 ZIP 并上传**。

---

## 五、网络行为

`config.py` 里只有一个地址：

```python
API_BASE = "https://xv6.kuafuai.top/os-basecamp-submission-sever"
```

- 无硬编码 IP
- 无 telemetry、无第三方上报
- 走 HTTPS（`urllib.request`，`ca-certificates` 校验）
- 路径 `os-basecamp-submission-sever` 是 `server` 的拼写错误，不影响功能，但说明是自建服务
- 另有 `socket.AF_UNIX` 本地 socket，用于终端与守护进程通信（`SO_PEERCRED` 校验对端 PID），非网络

---

## 六、安全性核查

**干净的地方**

- 无二进制、无 setuid/setgid
- 无 `base64` / `marshal` / `pickle.loads` / 动态 `eval` / `exec` 混淆（唯一 `__import__` 是用来取 `uuid`）
- `subprocess` 仅用于 `zenity` 弹窗、启动自身守护进程 `os_basecamp_client daemon`、以及 `execvp` 启动 shell
- ZIP 路径校验防穿越（`validate_archive_path`：拒绝 `..`、绝对路径、反斜杠、>32 层、>1024 字符）
- 符号链接防护：源码若含指向目录外的软链，直接报错拒绝
- 一致性防护：复制前后重新清点源码，若期间文件变化则"本次未生成提交包"
- 原子写入（`mkstemp` + `os.replace` + `fsync`）、并发锁（`file_lock`）
- 上传后要服务器回执，字段不符就报错，不会"假装成功"

**需要留意的地方**

1. **安装会改系统级文件**（`install.py`）：
   ```
   /etc/bash.bashrc                                        ← 影响所有用户
   /etc/xdg/autostart/os_basecamp_client.desktop           ← 开机自启
   /usr/share/applications/os_basecamp_identity.desktop
   /usr/share/applications/os_basecamp_submit.desktop
   ```
   VMware 版（`install_vmware.py`）只改当前用户，不碰 `/etc/bash.bashrc`。
2. **强制开启 history**，见第三节。这是设计意图，不是 bug。
3. **多了一个后台进程**：`os_basecamp_client daemon`，以新会话启动、日志重定向。
4. **无签名**：deb 本身没有 GPG 签名，只有配套 `.sha256`（本次已校验通过）。
   信任基础是"课程群发的文件 + 哈希一致"，不是密码学签名。

---

## 七、卸载是否干净

`prerm` 在 `remove` / `deconfigure` 时调用 `install uninstall`，`uninstall_user()` 的逻辑：

- 用 `~/.local/state/os_basecamp_submission_install/install.json` 记录每个改过的文件和原文件备份
- 还原时先比 `sha256`：若你安装后改过该文件，**保留你的版本**并给出警告
- `.bashrc` / `.profile` 等按 `# BEGIN/END OS_BASECAMP_SUBMISSION` 区块精确摘除
- `claude` wrapper 会还原成真实的 claude 入口（`uninstall_wrappers`）
- `record["active"] = False` 标记失效，不删除学习数据

这点做得比多数课程工具好。

---

## 八、两个版本的关系

| | 1.1.1 | 1.1.2 |
|---|---|---|
| 包名 | `os-basecamp-submission-client-vmware` | `os-basecamp-submission-client` |
| `Conflicts` | `Conflicts: os-basecamp-submission-client` | 无 |
| 安装方式 | 仅当前用户（`install_vmware.py`） | 系统级 + 用户级（`install.py` + `user_integration.py`） |
| 关键差异 | — | 新增 `user_integration.py`；`claude_install.py` 9.5→12.9 KB；`install.py` 8.4→12.6 KB；`image_reset.py` 18.1→23.0 KB；`config.py` 3.1→5.1 KB |
| 文件权限 | 644 | 755 |

因为 1.1.1 声明了 `Conflicts: os-basecamp-submission-client`，
**装了 1.1.1 再装 1.1.2 会被 apt 拒绝**，必须先移除 1.1.1：

```bash
sudo apt remove os-basecamp-submission-client-vmware
```

---

## 九、实操中发现的问题（会影响你按文档安装）

**说明文档里的 deb 文件名写错了，照抄命令会报「找不到文件」。**

| | 文档里写的 | 实际文件名 |
|---|---|---|
| `提交软件v1.1.2-Vmware发布版/readme.md` 第 21–22 行 | `os_basecamp_submission_client_1.1.2-VMware_all.deb` | `os_basecamp_submission_client_1.1.2_all.deb` |
| `作业提交软件安装说明-训练营（修正）.md` 第 11–12 行 | 同上 | 同上 |

`.sha256` 文件里记录的名字是**正确的**（不带 `-VMware`），所以校验能过，
但后面 `apt install -y ./os_basecamp_submission_client_1.1.2-VMware_all.deb` 会失败。

正确流程：

```bash
cd "/存放 deb 的目录"     # 目录内只有这两个文件
sha256sum -c os_basecamp_submission_client_1.1.2_all.deb.sha256
sudo env OS_BASECAMP_USER="$(id -un)" apt install -y ./os_basecamp_submission_client_1.1.2_all.deb
dpkg-query -W -f='${Version} ${Status}\n' os-basecamp-submission-client
# 期望：1.1.2 install ok installed
```

注意最后一条查询的包名是 `os-basecamp-submission-client`（**不带 `-vmware`**），
这是 1.1.2 的包名；1.1.1 才是 `os-basecamp-submission-client-vmware`。

---

## 十、给你的实操建议

1. **别在终端里明文打印密钥**。`cat ~/.claude/settings.json`、`echo $ANTHROPIC_AUTH_TOKEN`
   都会被录进 `commands.md` 并上传。要看配置就用编辑器打开，或 `grep -c` 只数行数。
2. 装之前先确认没装 1.1.1，否则 apt 会因 `Conflicts` 拒绝。
3. 装完**必须重启**（README 第 6 节也这么说），因为 `/etc/xdg/autostart` 和 shell hook 要重新加载。
4. 第一次用先双击桌面「登记与更正学生资料」，学号姓名要和教师名单完全一致，
   否则提交时 `/api/eligibility` 会直接拒绝。
5. 提交成功的判据是**屏幕上出现「提交成功，服务器已确认归档。」**，
   桌面上出现 ZIP 只代表本地打包成功，不代表已上传。
6. 如果不想被录终端，唯一合规的做法是按课程要求来——这是过程性考核（实验占 70%）的一部分，
   绕过录制没有意义，反而可能被判无效提交。
