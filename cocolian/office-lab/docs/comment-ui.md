# Writer 批注界面

普通 Writer 批注采用简洁的页边样式：作者、正文、左侧强调线，移除卡片阴影和头像，保留折叠讨论的计数。时间、解决对勾和“⋮”只在悬浮或键盘聚焦时显示。它们保留布局空间，因此显示时不会移动后续批注。触屏没有悬浮，操作保持可见；菜单展开后也持续显示当前批注的操作。

对勾复用原生解决/重新打开批注命令；修改、回复、删除、线程操作及导航仍在原生菜单中。按钮遵循当前编辑权限、作者权限和宿主的 Hide_Command 设置，解决状态变化后同步更新无障碍名称与按下状态。菜单支持 Enter/Space，键盘跳转到批注时聚焦菜单。AI 审阅状态仍由原生命令和门户审阅关联处理。修订卡片、表格及演示文稿的原有控件不受普通 Writer 批注样式影响。

## 验证

在环境中安全提供 `LAB_KEY`，并安装 Playwright 与 Chrome 后运行：

```sh
python cocolian/office-lab/scripts/verify-comment-style.py --base https://office.cocolian.cn --output comment-style-validation
```

脚本只新建合成测试文档，验证默认/悬浮显示、卡片高度不变、菜单展开状态、修改与回复、键盘解决和重新打开、宿主命令隐藏、只读切换、删除回复、WOPI 保存和刷新。输出截图与结果，不打印登录密钥。候选代码可用 `--candidate` 指向包含 `candidate-methods.json` 和 `online/browser/css/cool.css` 的临时构建目录；正式验收不替换浏览器资源。
