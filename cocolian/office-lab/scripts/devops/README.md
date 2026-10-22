# codex-dev 自动构建与部署

按项目独立触发与部署：

| main 分支发生更新的项目 | 重新构建、推送、部署的模块 |
| --- | --- |
| cocolian | wopi（门户） |
| cocolian-office | collabora、collabora-wordmac、libreoffice |
| online.mirror | collabora、collabora-wordmac、libreoffice |

任意代码提交都会触发对应模块，不依赖外层项目更新子模块固定提交号；同时变更取模块并集。未选中的模块不构建、不更新、不重启，并核对其镜像、模板和 Pod 保持不变。手动 --force 且提交未变时可重建全部模块。

监听 GitHub 的 `cocolian/cocolian`、`cocolian/cocolian-office`、`cocolian/online.mirror` 三个库的 `main`。服务器主动每分钟拉取远程提交号，稳定 45 秒后触发；无需公网 Webhook、GitHub Actions Runner 或 GitHub 写入 Token。首次启用会构建当前版本；之后只有新提交或手动强制运行才构建。

源码归属：门户使用 cocolian/main；渲染引擎、coolwsd 和 browser 使用 online.mirror/main；字体配置、编辑辅助标记和转换适配器使用 cocolian-office/main。这里明确采用引擎 main，不使用外层旧的 submodule 提交号；每次运行的 commits.json 锁定三个实际提交。其他 SDK/office-core 子模块不参与现有服务构建。

流程：独立自动化工作区拉取固定提交 → 按源文件变更复用 Debian 13 构建缓存增量编译（3 CPU / 12 GiB，执行六个布局回归测试）→ 构建本次选中的运行时镜像 → 经加密 SSH 隧道推送 res.cocolian.cn → 在现有 k3s 节点串行启动无 Service 标签的候选 Pod 并转换合成 DOCX → 按不可变镜像摘要串行更新 office-lab → 检查登录、浏览器入口、普通/WordMac 编辑器及转换器导出、WOPI 锁定保存读取解锁 → 比较其他命名空间 Pod → 记录成功。未变化的引擎或浏览器复用已验证的编译结果，仍重新打包和发布镜像。发布不向 GitHub 回写，避免循环触发。

只修改 office-lab 中本次选定模块的镜像，保留 gateway 配置、Secret、文档 PVC、字体及既有运行参数。不会自动应用仓库中的任意 Kubernetes YAML 或修改命名空间/入口配置。新建另一套服务需要单独维护部署模板。失败不更新 last_success；部署后失败自动恢复本次已修改的 Deployment 模板；存在并发修改时停止覆盖并记录待人工处理原因。候选 Pod 为本流程临时资源，验收后删除；仅删除本次创建的 UUID 命名测试文件，不修改业务文档。不会删除历史镜像/PVC，导出中间 tar 在推送后删除。

服务器安装位置：

- systemd: `/etc/systemd/system/cocolian-devops.service`、`.timer`，以 codex 用户执行，开机自动启动。
- 监听器：`~/.local/lib/cocolian-devops/watch.py`；仅在成功发布后采用仓库中的监听器更新。
- 状态和日志：`~/.local/share/cocolian-devops/state.json`、`runs/<时间-提交摘要>/build.log`、`commits.json`、`images.json`、`deployments-before.json`、`checks.json`、`release.json`。失败还有 `rollback.json`。
- 私有配置：`~/.config/cocolian-devops/config.json`（初始引擎基线 053c8491238）；registry-auth.json 为 0600，复用现有仓库凭据，永不提交 Git。集群操作使用现有 office-lab 受限 kubeconfig；不新增集群管理员凭据。
- 编译缓存：`~/projects/cocolian-office/build/source-19b4efae7fb8`。自动化不 reset 人工工作树；人工编译同一缓存前先停 timer 并确认 service 已结束。不能在 make 链接过程中复制运行库。

```sh
sudo systemctl status cocolian-devops.timer cocolian-devops.service
journalctl -u cocolian-devops.service -n 80 --no-pager
cat ~/.local/share/cocolian-devops/state.json
# 检查当前 main（仍有防抖、无变化不重复发布）
sudo systemctl start cocolian-devops.service
# 强制重试当前提交；与定时任务共用互斥锁
python3 ~/.local/lib/cocolian-devops/watch.py --force
# 停止未来的触发，不中断已运行的发布
sudo systemctl stop cocolian-devops.timer
```

失败后同一组提交 30 分钟后重试；新提交会重新防抖。GitHub 无新提交时保持安静。此流程不是 GitHub Actions，因此 GitHub Actions 页面不会出现运行记录，应查看上述服务器状态和日志。

安装（仅 codex-dev）：将 watch.py 复制到上述用户目录，将 `cocolian-devops-system.service` 安装为系统 `cocolian-devops.service`、`cocolian-devops-system.timer` 安装为 `.timer`，准备私有配置后执行 `sudo systemctl daemon-reload && sudo systemctl enable --now cocolian-devops.timer`。不要重复安装草稿用户级服务。

镜像传输：公网反向代理下载大层速度较慢，宿主机安装 `cocolian-registry-tunnel.service`，使用现有宿主机 SSH 身份连接 VM，将 VM 的 `127.0.0.1:15000` 反向转发到宿主机回环流式代理，再访问现有 registry Service。仅本机回环监听，公网不开放端口，传输受 SSH 加密；该回环端点使用 HTTP，公共 res.cocolian.cn 的 TLS 校验保持启用。两端重启后 systemd 自动重连。私有 config.json 设置 registry_endpoint=127.0.0.1:15000；业务镜像始终记录公共域名与不可变摘要。

宿主机的 `cocolian-registry-proxy.service` 仅监听 127.0.0.1:15001，将仓库返回的绝对上传 Location 地址改回 VM 回环端口，避免上传绕回公网；请求和响应以有界分块转发，支持 Content-Length 与 chunked 上传。不修改 applications 命名空间中的仓库配置。代理与隧道均为宿主机 systemd 开机服务。

### Mac Word rendering profile

Renderer releases install the private font pack pinned by
`office-fonts-manifest.json` from
`~/.local/share/cocolian-devops/fonts/office`. Font binaries are never stored in
Git. A missing file or a mismatched SHA256 stops the build before image creation.
All three renderers receive the same fonts and fontconfig profile. Candidate
pods must select native FangSong_GB2312, KaiTi_GB2312, SimSun, Microsoft YaHei,
FangSong, STZhongsong and Times New Roman before deployment.

`COCOLIAN_WORD_MAC_METRICS=1` enables the Mac Word profile. It excludes the 127%
Windows CJK enlargement when choosing grid multiples, preserves ordinary
non-grid font metrics, and uses the measured Mac Word behavior for
proportional spacing on multiple grid rows. Without this variable, the existing
Windows Word metrics remain active. The scoped `testWordBodyGridProportional`
regression exercises the Mac profile without changing the rest of the test
process. Renderer builds also retain the existing typography, table grid and
ODF compatibility tests.

`government-corpus.json` records the six government download URLs, immutable
input hashes, actual Mac Word reference page counts, and the measured 21-case
line-pitch matrix. This corpus is **not accepted** yet: equal page counts do not
prove equal pagination or visual layout. Keep this repair off main until the
remaining table tail, non-grid line-height, and line-wrapping differences pass
visual review against the saved Word PDFs.

The Mac profile supplies an eight-point Normal-template paragraph after-gap
when the DOCX omits `pPrDefault`. This was confirmed in Mac Word paragraph
settings and PDF exports of the Jining and Beijing government samples.
`testWordMacMissingParagraphDefaults` checks disabled, enabled, and explicit-zero
behavior using font-independent synthetic fixtures. Government originals are
never normalized or rewritten as part of rendering.
