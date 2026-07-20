# 运行与发布手册

适用范围：Steam PC、`UE4SS_v3.0.1-1009-gc2ac2464`、纯客户端 Lua + LogicMod Pak。

<!-- AUTO-GENERATED:START sources=readme.md,package.json,LogicMod/README.md,tools/performance,docs/testing -->
## 部署

### Steam Workshop Loader

1. 订阅条目及其必需物品 `UE4SS Experimental (Palworld)`。
2. 在 `Options -> Mod Management` 中启用 `UE4SSExperimentalPW` 和 `PalworldResourceESP`。
3. 完整重启游戏，确认 Loader 创建：

   ```text
   Mods/NativeMods/UE4SS/Mods/PalworldResourceESP/Scripts/main.lua
   Pal/Content/Paks/LogicMods/PalworldResourceESP.pak
   Mods/ManagedMods/PalworldResourceESP/InstallManifest.json
   ```

4. 检查 `InstallManifest.json` 的 `WorkshopId` 与订阅条目一致，且 `LastInstallTimeUtc` 和 `LastWorkshopUpdateTimeUtc` 都不是空时间。
5. 检查 `Mods/PalModSettings.ini` 同时列出 `UE4SSExperimentalPW` 和 `PalworldResourceESP`。

### Release ZIP / 手动部署

1. 关闭 Palworld。
2. 安装并配置已测试 UE4SS 版本。
3. 将完整 Release ZIP 解压到 `Palworld` 根目录并合并 `Pal` 目录。
4. 确认：

   ```text
   Pal/Binaries/Win64/ue4ss/Mods/PalworldResourceESP/enabled.txt
   Pal/Binaries/Win64/ue4ss/Mods/PalworldResourceESP/Scripts/main.lua
   Pal/Content/Paks/LogicMods/PalworldResourceESP.pak
   ```

5. 完整重启游戏，进入存档后按 `Shift+Y` 检查面板。

## 健康检查

项目没有 HTTP 健康端点。以 `UE4SS.log` 和游戏行为为准：

| 信号 | 期望 |
| --- | --- |
| `BOOT_OK` | 每个有效世界代次完成一次启动 |
| `PANEL_KEYBIND_READY key=Shift+Y` | 面板热键注册成功 |
| `BRIDGE_READY` / `BRIDGE_TARGETS_SYNCED` | LogicMod Bridge 可用 |
| `PLAYER_AUDIT ... candidate_player_count=0` | 真人玩家边界成立 |
| `EVENT_TARGET_ADDED/REMOVED` | 新帕鲁与死亡/捕捉/卸载增量更新 |
| 稳定场景 | 不应每 5 秒出现周期性 `SCAN_DONE` |

性能诊断使用 `tools/performance/Watch-PalworldPanelCapture.ps1` 和外部 PresentMon；输出保存在仓库外。

## 发布流程

1. 运行 `npm test` 与 `git diff --check`。
2. 通过 `Sync-LogicModTooling.ps1` 将固定源码同步到外部 PMK；保留脚本生成的时间戳备份。
3. 在 UE 5.1.1/PMK 中生成、编译、Cook 非零 chunk。
4. 将 chunk 命名为 `PalworldResourceESP.pak`，不得使用 `_P` 后缀。
5. 用 UnrealPak `-List` 验证 9 个 `.uasset`、9 个 `.uexp`、0 DLL。
6. 从 Git 跟踪的 `PalworldResourceESP` 文件组装 Lua 目录；不得复制 `user-settings.log`。
7. 组装 `Pal/...` 完整安装 ZIP，加入 README、LICENSE 和 SHA-256 文件。
8. 先创建 Draft/Prerelease，上传并比较 GitHub 资产 digest，再公开。

当前公开检查点：`v0.1.0-beta.1`，ZIP SHA-256 `CB1FABA1BEEFF9EE59198B701F41FCC88313EDF141AC8AAC167CDFD7F3AA7CCF`，内部 Pak SHA-256 `192084C3A7E54BDDEB66A8AFFB3FC19ED001FB6274C5D637920E2233EB5115E8`。

### Steam Workshop 证据

- 公开条目：[`3768263317`](https://steamcommunity.com/sharedfiles/filedetails/?id=3768263317)
- 包版本：`0.1.0-beta.1-workshop.1`
- 2026-07-20 真实订阅下载：Steam 清单记录安装和订阅详情；13 个发布载荷文件与上传暂存包逐文件一致。Steam 额外分发的 `.workshop.json` 只含条目 ID 和空发布字段，不含敏感信息，也不参与安装规则。
- 2026-07-20 空安装启动：Workshop Loader 重新安装 8 个 Lua 文件、Pak 和 `Info.json`；`WorkshopId=3768263317`，安装与 Workshop 更新时间均有效，Lua/Pak/Info 哈希与下载内容一致。
- 运行冒烟：绿色引导线出现；一次热键注册；两次 `Shift+Y` 请求、派发和完成全部一一对应，完成状态均为 `ok`；无面板或启动失败；正常退出。
- 未覆盖：击杀/捕捉清理、远距离卸载、新帕鲁流式刷新、完整面板控件和筛选回归；公开后的剩余兼容性问题依赖社区反馈补充。

### Nexus Mods 证据

- 公开条目：[`4055`](https://www.nexusmods.com/palworld/mods/4055)
- 主文件：`Palworld Resource ESP v0.1.0-beta.1 (Steam PC)`，版本 `0.1.0-beta.1`，页面大小 `163KB`；来源 ZIP SHA-256 为 `CB1FABA1BEEFF9EE59198B701F41FCC88313EDF141AC8AAC167CDFD7F3AA7CCF`。
- 页面元数据：分类 `User Interface`；标签 `Gameplay`、`Quality of Life`、`AI-Generated Content`；头图 `1300x372`，图库图源 `1920x800`。
- 权限：自定义 source-available 许可证与 Credits 已保存；允许 Nexus 平台奖励，禁止付费访问或销售，完整 `LICENSE` 文本优先。
- 2026-07-20 新上传器的 `www.nexusmods.com -> next.nexusmods.com` 权限保存和发布 POST 被 Cloudflare 跨域预检拦截；在已登录 Chrome 的 `next.nexusmods.com` 同源页面提交 Nexus 原始请求后均返回 `success=true`。
- 发布后独立状态读取为 `modStatus=1`；无 Cookie 浏览器上下文访问 Description 与 Files 均返回 200，主文件和 `Manual download` 可见。
<!-- AUTO-GENERATED:END -->

## 常见问题

| 现象 | 检查 |
| --- | --- |
| 启动即崩溃 | 确认 Pak 来自同一发布包；检查 `PostBeginPlay` 是否保持被动；查看最新 UE4SS/崩溃日志 |
| 无引导线 | 确认 Lua 与 Pak 同时安装；检查 `BOOT_OK`、Bridge 和过滤条件；切换 Mod 开关触发显式修复快照 |
| 新帕鲁不出现 | 检查 BeginPlay readiness retry 和对象路径解析日志；不应通过恢复 5 秒全量扫描解决 |
| 面板无法输入/鼠标丢失 | 检查 `UIOnlyEx`、Mod-owned input-disable flag 和关闭时恢复路径 |
| 设置不保存 | 检查 Mod 目录可写、`USER_SETTINGS_PATH` 和 `USER_SETTINGS_SAVED version=v12` |
| 固定约 5 秒卡顿 | 检查当前 profile 的 `reconcile_interval_ms=0`，确认没有旧 Lua 或旧 Pak 混装 |

## 回滚

1. 关闭游戏并备份 `Scripts/user-settings.log`。
2. 不删除当前文件：将问题 Pak 和 Lua Mod 目录分别改名为 `__DEPRECATED_YYYYMMDD_<原名>`。
3. 从已验证的上一发布包恢复完整 Lua + Pak 组合，不混用版本。
4. 若公开 GitHub Release 有问题，将其重新设为 Draft；保留标签与资产供审计，再发布修正版。
5. 若 Steam Workshop 或 Nexus Mods 条目有问题，先将对应条目改为 Hidden/Unpublished，再发布修正版。
6. 回滚后重跑启动、进入存档、面板、死亡/捕捉、返回标题和正常退出检查。

升级或回滚均不得覆盖唯一副本；所有外部 PMK 同步和游戏部署必须先建立可恢复备份。
