# build-host 自动构建与部署

监听 GitHub 的 `cocolian/cocolian`、`cocolian/cocolian-office`、`cocolian/online.mirror` 三个库的 `main`。服务器主动每分钟拉取远程提交号，稳定 45 秒后触发；无需公网 Webhook、GitHub Actions Runner 或 GitHub 写入 Token。首次启用会构建当前版本；之后只有新提交或手动强制运行才构建。

源码归属：门户使用 cocolian/main；渲染引擎、coolwsd 和 browser 使用 online.mirror/main；字体配置、编辑辅助标记和转换适配器使用 cocolian-office/main。这里明确采用引擎 main，不使用外层旧的 submodule 提交号；每次运行的 commits.json 锁定三个实际提交。其他 SDK/office-core 子模块不参与现有服务构建。

流程：独立自动化工作区拉取固定提交 → 按源文件变更复用 Debian 13 构建缓存增量编译（3 CPU / 12 GiB，执行四个布局回归测试）→ 构建四个运行时镜像 → 经加密 SSH 隧道推送 registry.example.invalid → 在现有 k3s 节点串行启动无 Service 标签的候选 Pod 并转换合成 DOCX → 按不可变镜像摘要串行更新 office-lab → 检查登录、浏览器入口、普通/WordMac 编辑器及转换器导出、WOPI 锁定保存读取解锁 → 比较其他命名空间 Pod → 记录成功。未变化的引擎或浏览器复用已验证的编译结果，仍重新打包和发布镜像。发布不向 GitHub 回写，避免循环触发。

只修改 office-lab 的 collabora、collabora-wordmac、libreoffice、wopi 镜像，保留 gateway 配置、Secret、文档 PVC、字体及既有运行参数。不会自动应用仓库中的任意 Kubernetes YAML 或修改命名空间/入口配置。新建另一套服务需要单独维护部署模板。失败不更新 last_success；部署后失败自动恢复本次已修改的 Deployment 模板；存在并发修改时停止覆盖并记录待人工处理原因。候选 Pod 为本流程临时资源，验收后删除；仅删除本次创建的 UUID 命名测试文件，不修改业务文档。不会删除历史镜像/PVC，导出中间 tar 在推送后删除。

服务器安装位置：

- systemd: `/etc/systemd/system/cocolian-devops.service`、`.timer`，以 构建用户执行，开机自动启动。
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

安装（仅 build-host）：将 watch.py 复制到上述用户目录，将 `cocolian-devops-system.service` 安装为系统 `cocolian-devops.service`、`cocolian-devops-system.timer` 安装为 `.timer`，准备私有配置后执行 `sudo systemctl daemon-reload && sudo systemctl enable --now cocolian-devops.timer`。不要重复安装草稿用户级服务。

镜像传输：公网反向代理下载大层速度较慢，宿主机安装 `cocolian-registry-tunnel.service`，使用现有宿主机 SSH 身份连接 VM，将 VM 的 `127.0.0.1:15000` 反向转发到现有 registry Service。仅本机回环监听，公网不开放端口，传输受 SSH 加密；该回环端点使用 HTTP，公共 registry.example.invalid 的 TLS 校验保持启用。两端重启后 systemd 自动重连。私有 config.json 设置 registry_endpoint=127.0.0.1:15000；业务镜像始终记录公共域名与不可变摘要。
