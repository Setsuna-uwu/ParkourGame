# 雪原跑酷 · 三人交付说明

**QQ 群派任务，GitHub 交代码，组长验收并合并。** 每项任务有一位作者。组长自己的改动也走 PR，试玩或核对文档后自行合并。组员随时可以提意见；交付不需要等另一位在线点 Approve。

## 看懂一次交付

从最新 `main` 创建任务分支 → 在任务分支改文件 → **Commit** 保存一批改动到本机 → **Push** 上传这个分支 → **Create pull request** 请求合入 `main` → 组长查看差异、验收 → **Merge** 后改动进入远端 `main`。其他人的电脑切回 `main` 并 **Pull**，才会拿到这一版。

一个打开的 PR 可以包含多次提交。被要求补改时，回到同一任务分支修改、Commit、Push，原 PR 会自动更新。文档任务读一遍改动、点通链接即可交付。

仓库的 [Protect main 规则](https://github.com/Setsuna-uwu/ParkourGame/rules/27609982) 限制更新默认分支，管理员可通过 PR 合并。组员正常推送自己的任务分支。当前规则没有设置必须获得其他组员的 Approve；组长核对改动后在 PR 页确认合并。

## QQ 群里怎么走完一单

| 时机 | 谁发 | 可以照着填 |
|---|---|---|
| 开工 | 组长 @负责人 | 【任务】修复起跳偶尔漏按。【范围】输入和主循环。【验收】只绘图的那一帧按下空格，下一次更新能跳；长按不会凭空多跳一次。【时间】周五晚前给可试玩版，赶不上提前说。 |
| 接单 | 负责人 | 收到，预计……交。要动另一人负责的文件时我先在群里说。 |
| 遇到问题 | 负责人 | 我做到……；执行……后看到了……；已试过……；需要……。附报错原文或截图。 |
| 交付 | 负责人 | 【PR 链接】……。【怎么验】打开游戏后……。【我实际试过】……。【还没验证/已知问题】……。 |
| 收尾 | 组长 | 通过，已合进 main；或：按……操作时出现……，预期是……，请在原 PR 补改。 |

能运行、能复现约定的效果，而且相关旧功能仍然正常，组长就可以合。文档任务检查文字、链接和操作步骤。完成进度以 PR 里的文件和验证结果为准。时间紧时只交能验收的一小块，剩下的单独约。

## 第一次准备（每人一次）

1. 登录自己的 GitHub 账号，接受组长从仓库 Settings → Collaborators 发来的邀请。
2. 安装 [GitHub Desktop](https://desktop.github.com/download/)，用同一账号登录。游戏构建选一种编译器即可：将 x64 MinGW-w64 的 gcc 和 g++ 加入 PATH，或在 Visual Studio Installer 中安装“使用 C++ 的桌面开发”。详细入口见[项目首页](../README.md)。
3. Desktop 点 File → Clone repository → URL，填 https://github.com/Setsuna-uwu/ParkourGame.git，选好本地目录，点 Clone。已有带 .git 的克隆就点 File → Add local repository 选现成目录。下载的 ZIP 解压目录另存，之后用克隆目录工作。
4. Repository → Show in Explorer 打开文件夹，双击 scripts 里的 run.bat。第一次等它编译，看见游戏并能跳跃、重开，就具备了开工条件。脚本一闪即退，可以在项目目录打开 PowerShell，输入下方运行命令看完整报错。

## 收到任务后，可以交给 AI 操作

把 QQ 任务原文和以下这段一起发给能操作本地项目的 AI：

> 在本机 ParkourGame 克隆里做这项任务。先确认仓库、当前分支、工作区和远端最新 main；保留我未提交的其他工作。查本任务有无打开的 PR，有就用它的来源分支继续；新任务从最新 main 单开分支。只修改验收所需文件。代码改完运行 scripts/test.bat Debug，告诉我怎么启动并试玩成功和失败两种情况。文档任务检查内容和链接。让我知道验证结果，再把本任务改动 commit、push，创建或更新到 main 的 PR，把 PR 链接和待验证事项给我。留给组长合并。

本人至少试玩自己的改动一次，能说清改了哪里、怎样验证。AI 没有推送权限时，在自己的 Desktop 里完成下面的提交步骤即可。

## 自己点按钮完成一次交付

1. **拿最新版。** Desktop 顶部 Current branch 选 main，点 Fetch origin；若随后显示 Pull origin，再点一下。若 Changes 有没保存进 Git 的旧工作，先把它留在原任务分支，拿不准就截屏问组长。
2. **进入自己的任务分支。** 新任务点 Current branch → New branch，名字写任务，例如 fix/jump-input，基于 main 创建。组长退回的旧任务直接选原来的分支。
3. **在这个目录里写。** 用编辑器打开刚才克隆的文件夹。代码改完双击 scripts/run.bat 试玩，再运行核心测试。在项目目录 PowerShell 执行：
   ~~~powershell
   .\scripts\run.bat
   .\scripts\test.bat Debug
   ~~~
4. **保存并上传。** 切回 Desktop，在 Changes 勾选本次要交的文件，左下 Summary 写具体改动，点 Commit to 当前任务分支。第一次上传点 Publish branch，以后点 Push origin。Commit 只保存到本机，Push 后队友才看得到。
5. **发起 PR。** 在仓库网页 Pull requests → New pull request，base 选 main，compare 选你的任务分支，核对文件后 Create pull request。写怎么验，复制此页链接发 QQ。有 GitHub Desktop 的预览 PR 按钮也可以从那里进入。
6. **根据反馈补改。** 保持同一分支，改文件 → Commit → Push，回到原 PR 的 Files changed 看最新内容，在群里回一句“已更新，请按……复验”。一项任务中途可以提交多次。

## 想让 AI 带着学命令行，可以跟这一个例子

在克隆目录打开 PowerShell，一段一段执行。下面用修跳跃举例，分支名和文件名换成自己的。本段和上面的按钮流程任选一条。

~~~powershell
git status                     # 查看当前分支和未提交文件
git switch main                # 切换到本地 main
git pull --ff-only             # 把远端 main 的更新拿到本地
git switch -c fix/jump-input   # 从此处新建并切到任务分支；一项任务只做一次
~~~

改完并试玩后：

~~~powershell
git diff                       # 看这次具体改了哪些行
git add src/main.c             # 选进本次提交；按实际改动选择文件
git diff --cached              # 再核对将要提交的内容
git commit -m "修复偶发跳跃输入丢失"
git push -u origin fix/jump-input   # 首推，建立本地与远端分支的对应关系
~~~

然后按上文第 5 步开 PR。PR 已打开时，补改仍在原分支执行 add、commit、push 即可。同一功能做一条分支，交付结束后可删除已合并分支，下次从最新 main 开工。

## 组长怎么验、怎么知道已经进 main

在 PR 页确认目标是 main，Files changed 里看本次改了什么。代码任务可在干净的 Desktop 工作区点 Fetch origin，在 Current branch 中选作者的任务分支，本机按 QQ 的步骤试玩，并补试一个旧功能。有问题给作者结论；通过后在 PR 页点 Merge pull request，再点 Confirm merge。网页出现紫色 Merged 后，到仓库 Code 页选择 main，打开这次改的具体文件核对。本机也要切到 main 点 Fetch origin → Pull origin。PR 页的 Approve 或 Close 都不会把代码并入 main。

**冲突由组长协调。** 同一文件的不同段通常能自动合并；若两项任务都要改角色落地的同一段，派活时就定好谁先交，谁接着改。GitHub 提示冲突时，作者把 PR 链接和冲突文件发群里，组长可让 AI 在该任务分支合入最新 main，说明两边各自要保留的行为，再解决冲突、提交、推送。相关两项功能放在同一局试玩通过才合。遇到含义不清的改动找作者确认，避免直接整份覆盖文件。

## 课设和分工

老师要求三至四人一组，小组人均有效代码量至少 200 行。本组三人至少 600 行；每人认领能独立解释的实际改动。现成原型当共同起点，记录实际作者与贡献。第三方库、机器生成的任务单和来回格式化不拿来凑数。PR 方便查工作记录，Git 增删行数本身不能认定有效代码。

可派的三类任务是滑行与落地（能在不同坡面起跳、落地）、随机地形（沟和障碍按当前速度可通过）、单局流程（拾取额外生命、摔倒扣命、结算与重开）。具体负责人以 QQ 派单为准。每人都负责实际程序、验证和报告里对应的一节；改公共结构体前先发群里对齐字段和用途。先把一小段跑通，再扩内容，最后一轮集中做验收。

交付前对照老师原件：演示实际文件读写；能用例子解释智能策略；程序界面能看到每位制作者的院系、学号、姓名。报告包含个人信息、分工、程序框架图、模块流程图、智能特性、调试案例和体会。手机号、邮箱等报告信息走课程提交渠道。约 5 分钟演示全部功能、分工和难点，每人都要熟悉整体。按老师指定名称将报告、工程、源码和资源打包，交班级 FTP。完整细则见仓库根目录的课程设计要求原件。
