# 贡献指南

本项目接受 Issue 和 Pull Request。提交内容必须保持真人玩家硬排除、纯客户端运行和源码公开许可证边界。

<!-- AUTO-GENERATED:START sources=package.json,package-lock.json,tools/logicmod/Sync-LogicModTooling.ps1 -->
## 开发环境

基础 Lua/文档开发需要：

- Node.js 与 npm
- Windows PowerShell（性能脚本需要）

LogicMod 资产开发额外需要：

- Unreal Engine 5.1.1
- 外部 Palworld Modding Kit no-Wwise 构建环境
- Visual Studio 2022 与 MSVC 14.38
- .NET 6

运行时验证使用 Steam PC 和 `UE4SS_v3.0.1-1009-gc2ac2464`。其他平台/版本的结果必须明确标注为社区验证。

## 安装依赖

```powershell
npm install
```

仓库没有 `.env` 文件或必需环境变量。完整 PMK、Unreal Engine、Cook 缓存和第三方游戏资产必须保留在仓库外。

## 命令

| 命令 | 说明 |
| --- | --- |
| `npm test` | 运行 36 个 Lua 测试、Lua 5.3 解析、运行时 Stub、Blueprint 源码契约和性能标记测试 |
| `./tools/logicmod/Sync-LogicModTooling.ps1 -PmkRoot <path>` | 备份外部 PMK 对应文件后，同步仓库中的编辑器插件与自动化脚本 |

## 测试位置

- `tests/lua/`: 纯数据模块与配置迁移测试。
- `tests/run-lua-tests.mjs`: Lua 解析、UE4SS Stub、面板/生命周期和 Blueprint 源码契约。
- `tests/performance/`: PresentMon 标记与分段解析。
- `docs/testing/`: Steam 手工验证矩阵。

没有配置独立 linter、formatter 或 pre-commit hook。修改时保持现有 Lua 5.3、C++/UE 和 Markdown 风格，并以 `npm test` 与 `git diff --check` 为最低门禁。
<!-- AUTO-GENERATED:END -->

## 代码边界

- 玩家分类必须在 adapter、筛选、预算和渲染之前完成，任何 UI 配置都不能绕过。
- Blueprint 不得主动调用 Lua 发现事件，也不得自行扫描 Actor。
- 延迟任务不得保留 UE4SS UObject wrapper；仅允许保存可重新解析的对象路径和代次。
- 游戏数据优先从当前运行版本读取，不提交帕鲁/词条静态快照作为最终事实。
- 不提交 Pak、运行时 DLL、`user-settings.log`、日志、存档、凭据或 PMK/引擎产物。

## Pull Request 清单

- [ ] 改动范围与 Issue/需求一致，没有无关重构。
- [ ] 新行为有自动测试或对应 `docs/testing` 手工验证项。
- [ ] `npm test` 和 `git diff --check` 通过。
- [ ] 真人玩家硬排除仍在所有 adapter/渲染路径之前。
- [ ] 新设置有版本迁移、边界限制和失败回退。
- [ ] 架构或长期边界变化已新增/更新 ADR。
- [ ] 没有提交 Cook/PMK/日志/用户设置/密钥。
- [ ] 用户可见文字默认中文，英文只在语言切换后显示。
