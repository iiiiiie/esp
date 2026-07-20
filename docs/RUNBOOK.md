# 运行与发布手册

适用范围：Steam PC、`UE4SS_v3.0.1-1009-gc2ac2464`、纯客户端 Lua + LogicMod Pak。

<!-- AUTO-GENERATED:START sources=readme.md,package.json,LogicMod/README.md,tools/performance,docs/testing -->
## 部署

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
4. 若公开 Release 有问题，将其重新设为 Draft；保留标签与资产供审计，再发布修正版。
5. 回滚后重跑启动、进入存档、面板、死亡/捕捉、返回标题和正常退出检查。

升级或回滚均不得覆盖唯一副本；所有外部 PMK 同步和游戏部署必须先建立可恢复备份。
