# Palworld Resource & Pal ESP

## Document Status

- **Status**: DRAFT - needs validation
- **Date**: 2026-07-14
- **Working title**: Palworld Resource & Pal ESP
- **Primary platform**: Steam PC
- **Distribution target**: Steam Workshop and source releases

## Problem Statement

《幻兽帕鲁》的收集型玩家经常需要依靠运气、重复跑图、逐个查看实体以及在游戏与攻略/交互地图之间切换，才能找到满足种类、属性、词条、个体值或收集进度要求的帕鲁和资源。这些操作消耗大量时间，但玩家又可能不希望通过修改存档、生成物品或改变概率来跳过游戏过程。

本项目面向这类玩家提供纯客户端、只读的实时查找与筛选能力。玩家仍需亲自移动、战斗和捕捉，Mod 只减少无效搜索和信息获取成本。

## Evidence

- 用户本人存在明确需求，并愿意长期使用和维护该 Mod。
- [Pal Analyzer](https://www.nexusmods.com/palworld/mods/336) 已有约 56.5 万下载，证明野生帕鲁信息查看有强需求。
- [Pal Info](https://www.nexusmods.com/palworld/mods/178) 和 [Pal IVs](https://www.nexusmods.com/palworld/mods/437) 分别覆盖捕捉数量、词条、属性和 IV，均有较高下载量。
- [Map Collectables Helper](https://www.nexusmods.com/palworld/mods/466) 已覆盖宝箱、蛋、收藏品和矿物节点，证明客户端资源标记可行。
- [YetAnotherMinimap](https://www.nexusmods.com/palworld/mods/2973) 已证明纯客户端 Mod 可以在单机和多人游戏中显示帕鲁、宝箱和蛋等对象。
- 现有 Mod 多数只解决单个信息或地图展示问题。当前未发现同时提供复杂帕鲁筛选、世界空间指引、多类资源适配并永久排除真人玩家的完整实现。

## Proposed Solution

构建一个 Steam PC 纯客户端 Mod，从客户端当前已加载或已复制的游戏对象中识别野生帕鲁与受支持资源，读取可用属性，应用组合筛选，并通过顶部引导线、屏幕外方向标记、名字、等级和距离等方式指向目标。

Mod 默认显示中文界面，并提供名为 `Language` 的设置项切换中文和英文。所有真人玩家对象必须在实体采集阶段被永久排除，不能通过配置重新启用。

## Key Hypothesis

我们相信，组合筛选客户端已加载的野生帕鲁并提供实时三维方向指引，可以为不愿修改游戏数据的收集型玩家显著减少无效跑图、查攻略和逐个检查的时间。

当玩家能更快找到满足条件的目标，同时真人玩家误标记为零、游戏稳定性和性能下降处于可接受范围时，说明产品方向成立。

## What We're NOT Building

- 真人玩家、队友、公会成员或其他服务器用户定位。
- 自瞄、自动攻击、自动投掷、自动捕捉或战斗决策。
- 瞬移到目标或自动寻路到目标。
- 修改刷新率、捕捉率、掉落率、词条、个体值或其他游戏数据。
- 生成帕鲁、物品或资源。
- 存档编辑。
- 反作弊、服务器规则或服务器 Mod 禁用机制的绕过。
- v1 不实现地图图标或地图适配。
- v1 不实现普通树木和普通实例化植被扫描。
- v1 不实现骨骼显示。
- v1 不承诺读取服务器未复制给客户端的动态实体和属性。

## Success Metrics

| Metric | Target | How Measured |
|--------|--------|--------------|
| 真人玩家误标记 | 0 | 单机类型边界测试；多人环境等待社区测试与反馈 |
| 已加载匹配目标发现延迟 | TBD - Phase 1 建立基线后确定 | 生成/进入加载范围到首次显示的时间 |
| 帧时间开销 | TBD - Phase 1 在最低测试配置上建立预算 | 开关 Mod 前后相同场景的帧时间对比 |
| 稳定性 | TBD - 定义连续运行测试时长 | 维护者完成单机长时间运行；多人兼容性不作保证，等待社区验证 |
| 筛选正确率 | 100% 覆盖已支持且数据完整的测试实体 | 使用已知帕鲁和属性样本验证 |
| 本地化完整性 | 已发布语言无缺失键 | 自动检查语言键并进行人工 UI 检查 |

## Open Questions

- [ ] 项目最终公开名称是什么，是否继续使用 `ESP` 作为主名称？
- [ ] MVP 默认扫描距离、刷新周期和最大显示目标数量是多少？
- [ ] 性能预算和最低支持硬件如何定义？
- [ ] 图鉴完成条件应读取游戏数据，还是在游戏规则不变时使用捕捉数量 5？
- [ ] 弱点在游戏数据中的准确语义是什么，是否存在稳定的 Socket、骨骼或组件标识？
- [ ] Alpha、Boss、Lucky/闪光在当前版本中的对象分类和字段如何统一？
- [ ] 首批资源适配器的确切顺序是什么？
- [ ] 开源许可证使用 MIT、GPL 或其他许可证？
- [ ] 英文翻译由谁审核，社区修订流程如何管理？

---

## Users & Context

**Primary User**

- **Who**: 时间有限、重视收集效率，但不愿直接修改数据的帕鲁收集者、图鉴玩家、高个体/词条猎人和资源采集者。
- **Current behavior**: 使用攻略、交互地图、视频、手机副屏或逐个查看帕鲁，反复跑图和刷新。
- **Trigger**: 需要寻找特定种类、属性、词条、个体值、收集进度或特殊资源。
- **Success state**: 在当前客户端可见数据范围内快速定位匹配目标，然后亲自前往、战斗和捕捉。

**Job to Be Done**

当我需要寻找特定帕鲁、词条、个体值或世界资源时，我希望直接筛选当前客户端已加载的目标并获得清晰的三维方向指引，从而减少无意义的跑图、查攻略和逐个检查，同时保留亲自探索和捕捉的过程。

**Non-Users**

- 需要真人玩家追踪、自瞄或战斗自动化的用户。
- 需要直接修改存档、生成内容或改变概率的用户。
- 需要服务端管理、服务器强制配置或服务器 Mod 禁用功能的服主。
- 需要隐藏运行、规避检测或绕过服务器限制的用户。

---

## Product Policies

- Mod 尝试在所有 Steam PC 单机和服务器环境中运行，不显示联机警告或确认弹窗。
- 维护者首发只保证经过验证的单机行为；服务器兼容性属于尽力支持，不作可用性保证，等待社区 Issues/PR 和测试反馈。
- 服务器是否允许该 Mod 由服务器自行决定，本项目不实现客户端限制。
- v1 必须纯客户端安装，不要求服务器安装组件。
- Game Pass PC 不属于 v1 支持范围，可由具备测试环境的社区贡献者后续适配。
- 中文是默认和源语言，英文是首发可选语言。
- 社区可以提交 Issues 和 Pull Requests，是否合并由维护者根据质量决定。

---

## Solution Detail

### Core Capabilities (MoSCoW)

| Priority | Capability | Rationale |
|----------|------------|-----------|
| Must | Steam PC 纯客户端运行 | 当前唯一可测试和正式支持的平台 |
| Must | UE4SS 运行时依赖 | 提供对象发现、Hook、Lua/C++ 扩展和 Workshop 依赖管理 |
| Must | 真人玩家采集层硬排除 | 核心公平性与产品边界，不能只是渲染层开关 |
| Must | `Shift+Y` 打开自有面板 | 提供统一、可扩展的复杂筛选入口 |
| Must | 中文默认、`Language` 切换英文 | 符合维护者语言能力并保留国际用户入口 |
| Must | 配置持久化 | 保存语言、筛选、样式、距离和性能选项 |
| Must | 扫描距离 10-50000m | 保留用户设定范围，但只作用于客户端已加载对象 |
| Must | 最大显示数量和刷新周期 | 防止画面拥挤与扫描失控 |
| Must | 野生帕鲁种类/名称筛选 | 核心查找能力 |
| Must | 属性多选 | 正确处理多属性帕鲁 |
| Must | 等级范围和性别筛选 | 常用收集条件 |
| Must | Lucky/闪光和 Alpha/Boss 筛选 | 高价值目标查找 |
| Must | 被动词条任一/全部/排除规则 | 支持实际词条狩猎工作流 |
| Must | HP/攻击/防御个体值阈值 | 支持高个体目标筛选 |
| Must | 图鉴捕捉数量与完成状态 | 避免重复收集已完成种类 |
| Must | 顶部锚点引导线 | 避免占用敏感的画面中心区域 |
| Must | 名字、等级、距离 | 最小可用目标信息 |
| Must | 屏幕外方向指示 | 目标不在视野内时仍可导航 |
| Must | 简单标记或方框 | 提供目标位置轮廓，默认样式需克制 |
| Should | 颜色规则和筛选预设 | 快速区分目标并复用常见搜索条件 |
| Should | 宝箱世界 ESP | 第一批地图资源类型之一，不依赖地图 UI |
| Should | 帕鲁蛋世界 ESP | 支持稀有度、属性和客户端可知信息 |
| Should | 翠叶鼠雕像/特殊雕像世界 ESP | 支持收藏状态筛选 |
| Should | 矿物节点世界 ESP | 高价值、稳定且比普通植被更有意义 |
| Should | 特殊树木资源适配 | 仅适配远古树木等有独立资源意义的对象 |
| Should | 弱点标记 | 替代骨骼显示，帮助手动投掷和攻击 |
| Should | 最近目标排序和一键聚焦 | 降低多个匹配目标造成的信息负担 |
| Could | 边缘轮廓、完整 3D 方框 | 视觉增强，需先评估性能和遮挡 |
| Could | 血量显示 | 游戏已有血条，因此优先级低 |
| Could | 更多掉落物、地下城和特殊资源 | 通过资源适配器逐步扩展 |
| Could | Game Pass PC 社区适配 | 必须由具备环境的贡献者独立测试 |
| Could | C++ 热路径优化 | Lua/Blueprint 性能不足时采用 |
| Could | 地图图标与地图筛选 | 与现有 Mod 重合，只有出现明确价值时再做 |
| Could | 静态世界分区或坐标数据研究 | 可能帮助显示未实例化但位置固定的资源 |
| Won't | 真人玩家显示 | 永久禁止 |
| Won't | 普通树木全量扫描 | 价值低、对象数量大、实现和性能风险高 |
| Won't | 骨骼显示 | 不符合本项目核心用途，弱点标记更有价值 |
| Won't | 强制获取服务器未同步的动态实体 | 纯客户端无法可靠实现，不采用网络绕过 |

### MVP Scope

MVP 用于验证“复杂帕鲁筛选 + 顶部方向指引”是否真正减少玩家搜索时间。MVP 不要求一次覆盖所有资源，也不要求地图、骨骼、血量和高级视觉效果。

MVP 完成条件：

1. 在 Steam PC 单机环境中完成维护者验证；服务器运行保留为未经保证的兼容目标。
2. 真人玩家对象无法进入候选实体注册表。
3. 能正确扫描客户端已加载的野生帕鲁并读取一组核心属性。
4. 能组合筛选并只显示匹配目标。
5. 能通过顶部引导线、屏幕外指示、名字、等级和距离导航。
6. 面板中文默认并可切换英文。
7. 性能、对象失效和场景切换行为经过基线测试。

### User Flow

1. 玩家从 Steam Workshop 安装 Mod 及 UE4SS 依赖。
2. 在游戏标题页启用 Mod 并进入单机或服务器。
3. 按 `Shift+Y` 打开中文筛选面板。
4. 选择帕鲁种类、属性、词条、个体值等条件。
5. 调整距离、最大显示数量和显示样式。
6. 关闭面板，查看从屏幕顶部指向目标的引导线和目标标签。
7. 屏幕外目标通过边缘方向指示显示。
8. 玩家亲自移动、战斗并捕捉目标。

---

## Technical Approach

**Feasibility**: MVP HIGH; full vision MEDIUM

**Architecture Notes**

- 遵循 [ADR-0001](../../../docs/adr/0001-use-ue4ss-hybrid-client-mod-architecture.md)。
- UE4SS Lua 负责对象发现、Hook、事件订阅、缓存和数据标准化。
- LogicMod/Blueprint Widget 负责筛选面板、顶部引导线、屏幕外方向和标签绘制。
- 优先使用事件和 `NotifyOnNewObject` 增量维护对象，避免每帧执行全量 `FindAllOf`。
- 使用不同刷新周期处理对象发现、位置更新、属性更新和低频收集状态。
- 所有候选对象先经过真人玩家排除，再进入适配、筛选和渲染管线。
- PalSchema 和 Mod Config Menu 不作为强制依赖；复杂筛选由自有面板承担。
- Steam Workshop 包含 Lua + LogicMods 类型，并声明 UE4SS 依赖。
- 如果性能分析证明 Lua 热路径不满足预算，再把扫描、投影或渲染迁移到 UE4SS C++。

**Entity Pipeline**

```text
UE4SS object events / periodic discovery
  -> object validity check
  -> hard reject APalPlayerCharacter and equivalent player-owned representations
  -> category adapter (Pal, chest, egg, statue, ore, special resource)
  -> normalized entity snapshot
  -> filter engine
  -> visibility budget / ordering
  -> Widget renderer
```

**Future Technical Escalation**

1. Lua + Blueprint/Widget profiling and optimization.
2. UE4SS C++ for high-frequency scanning or rendering.
3. Updated SDK dump and additional engine hooks.
4. Static world data or world-partition research for fixed resources.
5. Direct offsets, signatures or external memory reading require a separate ADR and are not part of the current commitment.

Direct memory reading cannot recover dynamic data the server never sent to the client.

### Technical Risks

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| 真人玩家被错误分类 | Medium | 采集层硬排除、独立测试矩阵、未知角色默认不显示 |
| 多人服务器只复制附近实体 | High | 明确“已加载实体”语义，对未知/缺失数据不猜测 |
| 扫描距离与实际加载范围不一致 | High | UI 解释为筛选半径，不触发强制加载 |
| Lua 全量扫描导致卡顿 | High | 事件驱动缓存、分批扫描、多刷新周期、最大显示预算 |
| UObject 失效或场景切换崩溃 | High | 每次访问前验证、弱引用/重新发现、清理生命周期 |
| 游戏更新改变类、字段或 Widget | High | 版本兼容矩阵、适配器隔离、启动自检、快速禁用损坏类别 |
| 英文翻译不自然 | Medium | 中文作为源语言，允许社区修订并检查缺失键 |
| 弱点数据不稳定或定义不一致 | Medium | 独立技术 Spike，不将其作为 MVP 发布阻塞项 |
| 普通植被对象数量过大 | High | 不做普通树木，仅适配少量特殊资源对象 |
| 高级视觉遮挡正常游戏 | Medium | 顶部引导线为默认、所有视觉元素可单独关闭、限制显示数量 |

---

## Implementation Phases

| # | Phase | Description | Status | Parallel | Depends | PRP Plan |
|---|-------|-------------|--------|----------|---------|----------|
| 1 | Runtime Spike | 验证 UE4SS、对象分类、核心字段、单机生命周期和玩家硬排除 | complete | - | - | [.claude/PRPs/plans/runtime-spike.plan.md](../plans/runtime-spike.plan.md) |
| 2 | Entity Core | 建立实体注册表、适配器、缓存、失效清理和筛选模型 | in progress | - | 1 | [.claude/PRPs/plans/entity-core.plan.md](../plans/entity-core.plan.md) |
| 3 | Pal Filter MVP | 实现帕鲁种类、属性、等级、性别、稀有、词条、IV、图鉴筛选 | pending | with 4 | 2 | - |
| 4 | UI & I18n | 实现 Shift+Y 面板、中文默认、Language 切换和配置持久化 | pending | with 3 | 2 | - |
| 5 | ESP Renderer | 实现顶部引导线、屏幕外指示、标签、方框和显示预算 | pending | - | 3, 4 | - |
| 6 | Reliability Gate | 单机/联机、性能、长时间运行、场景切换和玩家排除验证 | pending | - | 5 | - |
| 7 | Resource Adapters | 逐步增加宝箱、蛋、雕像、矿物和特殊树木 | pending | - | 6 | - |
| 8 | Advanced Research | 弱点、高级视觉、C++ 优化、Game Pass、静态世界数据 | pending | - | 7 | - |

### Phase Details

**Phase 1: Runtime Spike**

- **Goal**: 在正式编码前证明关键数据和渲染路径可行。
- **Scope**: 发现野生帕鲁、读取核心属性、区分玩家/怪物、创建最小顶部指示，并为未来社区多人测试保留诊断能力。
- **Success signal**: 单机测试中可稳定显示一个匹配帕鲁，真人玩家类型在采集层硬排除；服务器可用性不作为本阶段完成条件。
- **Outcome**: 2026-07-16 完成。Steam 单机已验证多目标顶部引导线、`candidate_player_count=0`、性别 Blueprint 适配、捕捉/死亡消线、返回标题和正常退出；多人仍等待社区证据。

**Phase 2: Entity Core**

- **Goal**: 建立可扩展而不过度抽象的实体处理主干。
- **Scope**: 生命周期、缓存、规范化快照、类别适配器、筛选接口和显示预算。
- **Success signal**: 新增一种实体类别无需修改玩家排除和渲染核心。

**Phase 3: Pal Filter MVP**

- **Goal**: 覆盖核心捉宠工作流。
- **Scope**: 所有 Must 级帕鲁筛选条件和组合语义。
- **Success signal**: 已知测试样本能被准确包含或排除。

**Phase 4: UI & I18n**

- **Goal**: 提供可操作、可保存的筛选体验。
- **Scope**: 中文面板、Language 选项、英文资源、配置保存和输入控制。
- **Success signal**: 不重启游戏即可修改筛选，重启后设置保留。

**Phase 5: ESP Renderer**

- **Goal**: 清晰导航而不干扰游戏中心视野。
- **Scope**: 顶部锚点引导线、屏幕外指示、名字、等级、距离、简单标记、颜色和数量限制。
- **Success signal**: 多目标场景仍可读，不遮挡准星和核心战斗 UI。

**Phase 6: Reliability Gate**

- **Goal**: 确保 MVP 可以公开发布。
- **Scope**: 性能预算、失效对象、场景切换、死亡/重生、坐骑和单机验证；主机联机与专用服务器客户端由社区条件具备时补充测试。
- **Success signal**: 发布门槛全部通过，已知限制被记录。

**Phase 7: Resource Adapters**

- **Goal**: 从帕鲁查找扩展到高价值世界资源。
- **Scope**: 宝箱、蛋、雕像、矿物和特殊树木，按可行性逐项发布。
- **Success signal**: 每类资源都有独立测试和性能预算，不依赖地图 UI。

**Phase 8: Advanced Research**

- **Goal**: 评估非核心增强能力。
- **Scope**: 弱点、C++ 热路径、高级视觉、Game Pass 和静态世界信息。
- **Success signal**: 每项研究形成独立结论、ADR 或明确拒绝理由。

### Parallelism Notes

- Phase 3 和 Phase 4 可在实体快照与筛选接口固定后并行。
- 渲染依赖筛选结果和 UI 配置契约，因此 Phase 5 在两者完成后进行。
- 资源适配必须在 MVP 可靠性门槛后开始，避免扩大未稳定的扫描范围。

---

## Decisions Log

| Decision | Choice | Alternatives | Rationale |
|----------|--------|--------------|-----------|
| 首发平台 | Steam PC | 同时支持 Game Pass | 维护者只有 Steam 测试环境，Workshop 也仅在 Steam 可用 |
| 安装模式 | 纯客户端 | 要求服务器组件 | 需要进入任意服务器时无需服主配合 |
| 服务器政策 | 不增加警告或客户端限制 | 私服限定、联机弹窗 | 是否允许 Mod 由服务器自行管理 |
| 服务器测试承诺 | 尽力兼容但不保证可用 | 首发强制完成多人矩阵 | 维护者不进行多人游玩，等待社区提供可复现反馈 |
| 玩家显示 | 采集层永久排除 | 默认关闭、渲染层隐藏 | 防止配置或 UI 缺陷造成真人玩家显示 |
| v1 技术栈 | UE4SS Lua + LogicMod Widget | 纯 Lua、纯 Blueprint、C++ 首发 | 平衡运行时访问、原生 UI、开发速度和可迁移性 |
| 配置 UI | 自有 Shift+Y 面板 | 强制依赖 Mod Config Menu | 复杂筛选需要专用界面，减少强制依赖 |
| 默认语言 | 中文 | 英文、自动系统语言 | 维护者以中文开发和审核，Language 可切换英文 |
| 引导线锚点 | 屏幕顶部 | 画面中心 | 避免干扰准星和正常操作 |
| 骨骼 | 不做 | 完整骨骼 ESP | 非 FPS 核心需求，价值低于弱点标记 |
| 血量 | 低优先级 | MVP 必须 | 游戏已有血条，重复价值低 |
| 地图 | 延后 | MVP 地图集成 | 与现有 Mod 重合，先聚焦世界空间筛选 |
| 树木 | 仅特殊树木 | 全部植被 | 普通树木数量大、价值低、实现风险高 |
| 未同步实体 | v1 不承诺强制加载 | 网络/内存绕过 | 纯客户端无法读取服务器未发送的动态数据 |

---

## Research Summary

**Market Context**

- 帕鲁信息、IV、收集状态和地图资源辅助均有成熟用户需求。
- 现有产品覆盖大量局部能力，因此本项目必须以组合筛选、世界空间导航、资源适配器和永久排除真人玩家形成差异化。
- `Map Collectables Helper` 已覆盖许多资源类型，但其重点是地图，且部分矿物使用预置坐标；本项目优先读取实际客户端对象并暂不做地图。

**Technical Context**

- [UE4SS Function Overview](https://github.com/PalworldModding/Docs/blob/master/docs/developers/ue4ss-modding/lua-mods/ue4ss-functions.mdx) 提供 Hook、对象创建监听、对象查找和定时任务。
- [Using Blueprints with Lua](https://github.com/PalworldModding/Docs/blob/master/docs/developers/ue4ss-modding/lua-mods/blueprints-with-lua.mdx) 证明 Lua 与 LogicMod/Blueprint 可以通过自定义事件协作。
- [LogicMod Introduction](https://github.com/PalworldModding/Docs/blob/master/docs/developers/ue4ss-modding/logic-mods/introduction.md) 建议最终用户界面使用 Widget，而不是 UE4SS 调试 GUI。
- [Steam Workshop Packaging](https://github.com/PalworldModding/Docs/blob/master/docs/developers/mod-publishing/workshop/packaging.md) 支持 Lua、C++、LogicMod、Pak、PalSchema 和依赖声明。
- 生成的 Palworld 类型信息中，玩家和怪物存在独立类型，帕鲁个体参数提供等级、性别、词条、稀有状态和多种属性接口。

---

*Generated: 2026-07-14*
*Status: DRAFT - needs validation*
