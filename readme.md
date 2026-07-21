# Palworld Resource ESP

《幻兽帕鲁》Steam PC 客户端 Mod，面向图鉴收集、帕鲁筛选和单机捕捉流程。项目始终排除真人玩家：不会扫描、筛选或绘制服务器中的真人玩家。

当前公开版本为 [`v0.1.0-beta.2`](https://github.com/iiiiiie/esp/releases/tag/v0.1.0-beta.2)，核心范围是帕鲁 ESP。宝箱、帕鲁蛋、翠叶鼠雕像、矿物等资源 ESP 尚未实现，不属于首个 Beta 的承诺范围。

> 本项目免费提供并公开源码，但采用自定义 source-available 许可证，不是 OSI 定义下的开源软件。若你付费购买了本 Mod，请立即申请退款并向销售平台投诉。

## 支持范围

- 游戏平台：Steam PC（Windows）
- Mod 框架：`UE4SS_v3.0.1-1009-gc2ac2464`
- 其他 UE4SS 版本：未测试
- Game Pass PC：未测试，欢迎社区贡献适配
- 多人服务器：社区测试状态，不保证兼容性
- 运行方式：纯客户端 UE4SS Lua + LogicMod Pak，不需要服务端组件

## 下载

普通玩家可以从以下公开渠道获取：

- [GitHub Releases](https://github.com/iiiiiie/esp/releases)：下载标有 Steam PC 的完整手动安装 ZIP
- [Nexus Mods](https://www.nexusmods.com/palworld/mods/4055)：下载同一版本的完整手动安装 ZIP
- [Steam 创意工坊](https://steamcommunity.com/sharedfiles/filedetails/?id=3768263317)：通过 Workshop Loader 自动安装

源码仓库按项目 ADR 只保存 Lua、项目自有 Blueprint 资产和构建工具，不提交 Cook 生成的 `.pak`；预编译 Pak 只通过上述公开渠道分发。

## 已实现功能

### 游戏内显示

- `Shift+Y` 打开或关闭全屏设置面板；面板打开时也可按 `Esc` 关闭
- 从屏幕顶部绘制帕鲁引导线
- 独立开关姓名、等级、距离、个体值和被动词条
- 目标距离上限 `0-330m`
- 目标显示上限 `1-100`
- 新加载帕鲁按实例路径逐只接纳，不再因一个目标变化重新扫描并解析全部已加载帕鲁
- 玩家移动时使用缓存坐标选择当前最近的 N 个目标，只原子同步最终可见集合
- 死亡、捕捉和实体卸载只更新对应候选键，下一次原子同步保持线条与元数据对齐

### 帕鲁筛选

- 等级范围与最大距离
- 性别：全部、雄性、雌性
- 闪光个体：全部、仅闪光、排除闪光
- Boss 个体：全部、仅 Boss、排除 Boss
- 九种属性多选
- 生命、攻击、防御个体值独立下限，三项必须同时满足
- 被动词条最多选择 4 个，包含条件使用 AND
- 被动词条右键排除、名称搜索、分类展开和效果悬停
- 图鉴捕捉进度：全部、未完成、已完成；捕捉数量达到 5 视为完成

### 帕鲁种类目录

- 运行时读取当前游戏数据，不提交固定版本的帕鲁数据快照
- 显示图鉴编号、名称、头像、属性和 12 项工作适应性
- 按名称或图鉴编号搜索
- 按属性和工作适应性组合筛选
- 按图鉴编号、冲刺速度或任意工作适应性排序
- 选择多个帕鲁种类后，以 OR 条件筛选世界目标
- 悬停查看基础属性、移动能力、进食量、伙伴技能与效果、击败掉落物

### 面板与配置

- 中文为默认语言，可切换英文
- 显示、常规筛选和帕鲁种类页面
- 筛选重置为默认值
- 滑块使用延迟提交，避免拖动时持续重建目标
- 显示项、筛选项、被动词条、展开状态、已选帕鲁和最后页面跨面板/存档/进程保存
- 面板打开时隔离游戏快捷键，关闭后恢复鼠标和游戏输入

## 安装

### Steam 创意工坊

1. 订阅 [`Palworld Resource ESP`](https://steamcommunity.com/sharedfiles/filedetails/?id=3768263317) 和页面列出的必需物品 [`UE4SS Experimental (Palworld)`](https://steamcommunity.com/sharedfiles/filedetails/?id=3625223587)。
2. 在 Palworld 的 `Options -> Mod Management` 中启用两者。
3. 完整重启游戏。Workshop Loader 会将订阅内容安装到：

   ```text
   Palworld\Mods\NativeMods\UE4SS\Mods\PalworldResourceESP\Scripts\main.lua
   Palworld\Pal\Content\Paks\LogicMods\PalworldResourceESP.pak
   Palworld\Mods\ManagedMods\PalworldResourceESP\InstallManifest.json
   ```

4. 进入存档后，附近帕鲁会自动出现引导线；按 `Shift+Y` 打开设置面板，打开后也可按 `Esc` 关闭。

创意工坊条目已声明 UE4SS 必需依赖。若 Steam 没有自动下载或启用该依赖，请先单独订阅并启用 UE4SS。

### Release ZIP / 手动安装

1. 安装并配置 `UE4SS_v3.0.1-1009-gc2ac2464`。
2. 关闭游戏，将 Release ZIP 解压到 `Palworld` 游戏根目录；压缩包内的 `Pal` 目录应与游戏现有的 `Pal` 目录合并。
3. 确认以下文件存在：

   ```text
   Palworld\Pal\Binaries\Win64\ue4ss\Mods\PalworldResourceESP\enabled.txt
   Palworld\Pal\Binaries\Win64\ue4ss\Mods\PalworldResourceESP\Scripts\main.lua
   Palworld\Pal\Content\Paks\LogicMods\PalworldResourceESP.pak
   ```

4. 完整重启游戏。进入存档后附近帕鲁会自动出现引导线，无需手动开关 Mod。

手动安装或从源码构建时，分别将 `PalworldResourceESP` Lua Mod 文件夹和 Cook 后的 Pak 放到：

```text
Palworld\Pal\Binaries\Win64\ue4ss\Mods\PalworldResourceESP
Palworld\Pal\Content\Paks\LogicMods\PalworldResourceESP.pak
```

## 卸载

创意工坊安装应先在 `Options -> Mod Management` 中禁用本 Mod，再取消订阅。只有没有其他 Mod 依赖 UE4SS 时才禁用 UE4SS。

手动安装应在关闭游戏后移走或禁用以下内容：

```text
Pal\Binaries\Win64\ue4ss\Mods\PalworldResourceESP
Pal\Content\Paks\LogicMods\PalworldResourceESP.pak
```

用户设置保存在 Mod 的 `Scripts\user-settings.log`。删除 Mod 前可自行备份该文件。

## 已知限制

- 只显示游戏已经加载并同步到本地客户端的实体，不会强制加载远处帕鲁。
- 目标上限表示最近一次原子重建时，玩家位置下最近的 N 个已加载、已初始化候选。生命周期事件默认等待 500 毫秒静默窗口，连续信号最长合并 1500 毫秒，完整重建之间至少间隔 2500 毫秒。
- 进入存档、切换运行模式、修改需要 Lua 字段的筛选，或累计移动 120 米后，会执行一次显式完整快照；稳定静止且没有生命周期变化时不做周期性全量扫描。
- 闪光、固定 Boss、多人服务器和 Game Pass 场景尚未形成完整覆盖测试。
- 游戏或 UE4SS 更新可能改变 SDK/API，需要社区反馈后适配。

## 开发与测试

安装 Node.js 后运行：

```powershell
npm install
npm test
```

自动化测试覆盖玩家硬排除、筛选组合、配置迁移、生命周期事件合并、死亡后的原子幸存者重建、最近 N 个目标、移动发现和性能标记解析。LogicMod 资产生成与 Cook 需要项目文档中记录的 Unreal Engine 5.1 与 Palworld Modding Kit 环境。

代码地图见 [`docs/CODEMAPS`](docs/CODEMAPS)，架构决策见 [`docs/adr`](docs/adr)，运行验证矩阵见 [`docs/testing`](docs/testing)。开发与发布流程见 [`docs/CONTRIBUTING.md`](docs/CONTRIBUTING.md) 和 [`docs/RUNBOOK.md`](docs/RUNBOOK.md)。

## 反馈与贡献

- GitHub Issues 和 Pull Requests 均可提交
- QQ 群：`1073495830`
- 贡献代码视为同意按本项目当前许可证发布，具体见 [`LICENSE`](LICENSE)

## 许可证与分发

本项目采用 **Palworld Resource ESP 源码公开许可证 1.0**：

- 允许未被排除的个人和组织免费使用、研究、修改和再分发
- 再分发必须免费、保留署名与许可证，并公开对应修改源码
- 禁止出售、付费下载、付费捆绑或将本项目作为付费服务的必要组成部分
- 不向 `wanjiadongli.com` 的运营/控制主体、B 站 UID `1317077696` 的运营/控制者及其规避代理授予许可
- GitHub、Nexus Mods、Valve/Steam Workshop 获得完成正常托管与分发所需的平台许可

这是一份 source-available 自定义许可证，不应宣传为 MIT、GPL 或 OSI 开源。各分发平台的服务条款可能要求上传者授予平台或订阅者额外权利；在 Nexus Mods 或 Steam 创意工坊发布前，应再次核对当时生效的平台条款。自定义限制能否在特定司法辖区执行不作保证。

本项目与 Pocketpair 无隶属或授权关系。《幻兽帕鲁》及相关名称、商标和游戏素材归其权利人所有。

## English Summary

Palworld Resource ESP is a Steam PC client Mod for loaded wild Pals. It never scans or renders real human players. The tested dependency is `UE4SS_v3.0.1-1009-gc2ac2464`; other UE4SS versions, Game Pass PC, and multiplayer servers are not guaranteed.

End users should download the complete Steam PC package from [GitHub Releases](https://github.com/iiiiiie/esp/releases). Cooked `.pak` files are distributed as release assets and are intentionally not committed to the source repository.

This repository is source-available, not OSI open source. Free use, modification, and redistribution are available only under the custom [`LICENSE`](LICENSE), which also defines excluded parties and platform-hosting permissions. The Mod must not be sold or placed behind paid access.
