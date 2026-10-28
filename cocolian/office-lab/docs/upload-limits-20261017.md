# 文件上传与 DOCX 转 PDF 上限

> 地址和目录均为通用示例，实际部署值应由自己的私有配置提供。


| 功能 | 文件上限 | 实现 |
| --- | --- | --- |
| 本地上传至知识库 | 1 GiB，即 1024 MiB | 浏览器及门户校验；每块最多 1 MiB 写入 |
| WOPI 读取及保存 | 1 GiB | 流式读取、流式临时文件写入，保存时检查文件锁并原子替换 |
| DOCX 转 PDF | DOCX 200 MiB | 流式解析单文件 multipart；表单头另预留 64 KiB |
| PDF 输出 | 200 MiB | 流式响应 |

知识库上传容量检查门限调整为 2 GiB，使用 KNOWLEDGE_MAX_BYTES 显式配置；原持久卷和已有文件保留。磁盘空间不足时拒绝上传。上传期间预留容量，临时文件不可被知识库列表选中，失败后清理。

edge-proxy 的 office 域名代理与 office-lab Ingress 请求体上限为 1g。网关普通上传上限为 1g，转换路径为 201m（为 200 MiB 文件的 multipart 表单头预留空间），转换服务对实际 DOCX 文件严格执行 200 MiB 上限。上传空闲超时 600 秒，转换引擎执行超时 300 秒，网关等待转换最多 600 秒。转换保持单任务并发。

## 验证边界

- 在隔离门户中实际传输、读取、保存整整 1 GiB 测试文件成功，超过 1 字节及空文件被拒绝，服务进程峰值 RSS 为 25584 KiB。
- 在相同资源限制的隔离转换容器中，实际提交整整 200 MiB 的有效 DOCX 成功生成 PDF；增加 1 字节被拒绝。测试 DOCX 是小文档添加未引用 ZIP 条目构成，用于验证传输与文件大小边界，不代表复杂大文档的编辑或转换性能。
- 无结尾 CRLF 的 multipart 可正常转换，非 DOCX 被拒绝。
- 普通文档上传、知识库打开、编辑保存、锁隔离、同名不覆盖和服务重启后识别已通过回归。

旧镜像摘要、Deployment、ConfigMap、Ingress 及 edge-proxy 配置已保留，可回滚；不删除 PVC。

## 线上验证及版本

通过实际域名验证：21 MiB 文件上传返回 201，21 MiB 以上的 DOCX 转换返回 200 且 PDF 有效。验证文件已清理。edge-proxy 与 Pod 内网关的 nginx -t 均通过，其他 10 个 Pod 的 UID、镜像、就绪状态与重启次数无变化。原文档卷保留。

- 门户镜像：`registry.example.invalid/cocolian/office-portal@sha256:6783d7dd24aa30a3f18fc20d9085b5aad9e71666e04c7a8d71d18fd0f9aa91c4`
- 转换镜像：`registry.example.invalid/cocolian/libreoffice-source@sha256:7d559908865f12aac5f71977c58c6c9c8a06d52a26a9a84db5c7bd05a8faf10c`
