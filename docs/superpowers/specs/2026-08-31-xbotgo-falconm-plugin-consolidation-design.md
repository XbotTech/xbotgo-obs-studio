# XBotGo/FalconM 单插件收敛设计

## 状态

- 日期：2026-08-31
- 状态：已确认
- 适用平台：macOS 13+、Apple Silicon arm64、Qt 6

## 目标

将 XBotGo 菜单、直播任务、FalconM 设备管理、设备控制、角色场景和自动导播全部迁入
`plugins/xbotgo/`，并继续只生成 `xbotogo-falconM` 一个插件动态模块。

迁移完成后，删除或禁用该插件应同时移除全部 XBotGo 功能；OBS frontend 不再包含 XBotGo/FalconM
实现、状态、菜单、Dock 或本地化文案。用户看到的顶层 **XBotGo** 菜单、设备管理 Dock 和业务行为保持不变。

## 非目标

- 不改变 Media SDK、Source ID、设备协议、服务端地址、直播请求字段或默认业务参数。
- 不增加 x86_64、Universal 或非 macOS 平台支持。
- 不重构 FalconM 音视频生命周期或改变 Source 激活、停用、连接和销毁语义。
- 不保留未被调用的 `XBotGoSliderControlDemoDialog`。

## 架构

`xbotogo-falconM` 是一个深模块。它的外部 interface 只有 libobs Source 注册、libobs signal/proc 机制和
公开的 `obs-frontend-api`；Qt 只用于插件自己的界面和异步网络实现。插件实现不得包含 `OBSBasic.hpp`、
`OBSApp.hpp`、`qt-wrappers.hpp`、`RemoteTextThread.hpp` 或其他 frontend 私有头文件。

插件内部按职责组织：

```text
plugins/xbotgo/
├── runtime/        插件 UI 生命周期、frontend event 分发、顶层菜单
├── ui/             设备管理 Dock、设备卡片、控制窗口和通用控件
├── live/           直播配置模型、请求、对话框、心跳和状态机
├── director/       自动导播及纯策略
├── scenes/         Camera Role 场景和 SceneItem 迁移
├── device-search/  SSDP 设备发现
├── protocol/       FalconM 协议
└── falconm-*       Source 和 Media SDK 生命周期
```

`obs_module_load()` 立即注册 FalconM Source，并创建唯一的 `XBotGoPluginRuntime`。Runtime 注册 frontend
callback 后，通过主窗口 queued invocation 请求一次初始化，同时将 `OBS_FRONTEND_EVENT_FINISHED_LOADING` 作为
兜底触发；两条路径调用同一个幂等初始化方法。`obs_module_unload()` 对称移除 frontend callback、菜单和 Dock，
停止网络及自动导播，断开所有 signal，再销毁 Qt 对象。

## 内部模块和 interface

### XBotGoPluginRuntime

Runtime 是唯一装配入口，拥有菜单、Dock、自动导播和直播 Runtime。它通过
`obs_frontend_add_event_callback()` 接收 frontend 生命周期事件，并将事件转交给内部模块。

初始化发生在 Qt UI 线程。顶层菜单通过 `obs_frontend_get_main_window()` 获得 `QMainWindow`，插入 Help
菜单之前；无法定位 Help 时追加到菜单栏。菜单包含当前的“设备管理”和“开始直播”两个 Action。Dock 由插件创建
`QDockWidget`，通过 `obs_frontend_add_custom_qdock()` 注册，默认位于右侧，可关闭且不可浮动或移动。

### FalconMSourceBridge

Bridge 是插件 UI、自动导播和 FalconM Source 之间的内部 seam。它集中维护 Source ID、signal 名称和 proc
handler 名称，提供 Source 类型识别、连接状态、控制命令和角度订阅能力。调用者不再硬编码
`xbotogo_falconm`、`motor_angle_report` 或 proc 名称。

Bridge 内部沿用现有 OBS proc/signal 机制，以保持 Source 所有权、线程和销毁语义。`falconm-protocol.hpp`
成为插件内部协议类型，不再形成 frontend 到插件实现的跨目录包含。

### CameraRoleDirector

Camera Role 场景名称、UUID 私有标记以及 SceneItem transform、crop、可见性、锁定、混合和转场状态保持不变。
原有 `OBSBasic` 调用替换为公开 interface：

| 当前依赖 | 迁移后 |
| --- | --- |
| `OBSBasic::GetCanvases()` | `obs_enum_canvases()` |
| 当前编辑场景 | `obs_frontend_get_current_preview_scene()`；非 Studio Mode 使用当前场景 |
| 当前 Program 场景 | `obs_frontend_get_current_scene()` |
| `SetCurrentScene()` / `TransitionToScene()` | `obs_frontend_set_current_scene()` |
| `SaveProject()` | `obs_frontend_save()` |

普通场景使用 `obs_enum_scenes()`，非 EPHEMERAL Canvas 使用 `obs_canvas_enum_scenes()`。角色切换仍先确保目标
SceneItem 创建成功，再删除其他场景中的旧 SceneItem；失败时保留原布局。

自动导播继续要求唯一 Center Source，沿用 ±30° 阈值、1–10 秒冷却范围、默认开启和 3 秒冷却。成功发起
Program 切换后才更新冷却时间。

### XBotGoLiveRuntime

直播 Runtime 使用以下状态机阻止重复启动和竞态：

```text
Idle -> Fetching -> Confirming -> Starting -> Streaming -> Stopping -> Idle
```

- `Idle` 才接受“开始直播”。
- `Fetching` 使用异步 `QNetworkAccessManager` 获取任务；失败返回 `Idle` 并显示错误。
- `Confirming` 显示配置窗口；取消时停止已创建的服务端任务，再返回 `Idle`。
- 确认后创建 `rtmp_custom` Service，通过 `obs_frontend_set_streaming_service()` 设置，并调用
  `obs_frontend_save_streaming_service()`，随后进入 `Starting` 并调用 `obs_frontend_streaming_start()`。
- `OBS_FRONTEND_EVENT_STREAMING_STARTING` 启动心跳；`STREAMING_STARTED` 进入 `Streaming`。
- `STREAMING_STOPPING` 进入 `Stopping` 并停止心跳；`STREAMING_STOPPED` 发送停止任务请求、清空任务状态并回到
  `Idle`。
- 调用 `obs_frontend_streaming_start()` 前清除本轮 starting-event 标记；调用返回后仍未同步收到
  `STREAMING_STARTING` 即判定本地启动失败，停止服务端任务并回到 `Idle`，避免菜单永久禁用。

服务端返回 task ID 后，用户取消、Service 创建失败和本地推流启动失败都必须发送一次停止任务请求。任务 ID
由 Runtime 单点持有并以 `std::exchange` 取出，保证正常停止、错误回滚和重复 frontend event 不会重复停止同一任务。

拉流 URL 继续只在确认窗口展示。当前无消费者的 `OBSBasic::xbotgoPullUrl` 不迁移。

## 网络、线程和失败处理

直播开始、心跳和停止请求由 Runtime 所有的 `QNetworkAccessManager` 异步执行，每个请求使用 10 秒超时。
同一时刻最多一个心跳请求在途。回调以有效 QObject 为 context；Runtime 关闭时停止 timer、abort 所有未完成
reply，并使后续回调失效。

日志记录请求种类、任务阶段和错误，不记录推流密钥、完整 URL、认证信息、完整请求体或设备隐私数据。网络错误、
非法 JSON、非 200 业务码、缺字段和非法 RTMP/RTMPS URL 均保持可诊断的失败结果。

Source signal 和 Media SDK 回调可以来自非 UI 线程。所有 Qt Widget、菜单、Dock、场景切换和直播状态 UI 更新都
通过 queued invocation 回到 Qt UI 线程。Source 引用在排队期间使用 OBS 强/弱引用包装，销毁后不再调用插件实现。

## UI 和本地化

迁移后的类型使用插件职责命名：

- `OBSBasicFalconMDevices` -> `FalconMDevicesWidget`
- `OBSBasicFalconMControl` -> `FalconMControlWidget`
- `OBSBasicAutoDirectorControl` -> `AutoDirectorControlWidget`
- `XBotGoLiveStreamConfigDialog` -> `LiveStreamConfigDialog`

插件 target 使用 AUTOMOC，不再手工包含 `moc_*.cpp`。`QTStr` 替换为插件内部 `Tr(key)`，底层使用
`obs_module_text()`。全部 `Basic.MainMenu.XBotGo.*` 文案迁入插件的 `data/locale/en-US.ini` 和
`zh-CN.ini`，改为插件私有稳定 key。frontend locale 删除对应条目。

## Frontend 清理

迁移完成且插件行为验证通过后执行清理：

- 删除 `frontend/xbotgo/`。
- 删除 `frontend/dialogs/` 下六个 FalconM/AutoDirector 文件。
- 从 `OBSBasic.cpp`、`OBSBasic.hpp`、`OBSBasic_MainControls.cpp` 移除 XBotGo 成员、初始化、slots、直播状态和退出逻辑。
- 从 `OBSBasic.ui` 移除 XBotGo 菜单和 Action。
- 从 frontend CMake 和 locale 移除 XBotGo 文件及文案。
- 保留 `plugins/CMakeLists.txt` 对 `xbotogo-falconM` 的平台限定注册。

清理采用最后一步，确保迁移期间随时可对比旧行为。最终全仓除插件目录、插件注册和说明文档外，不再包含
XBotGo/FalconM 实现或用户文案。

## 测试和验收

自动验证：

- 将 `XBotGoAutoDirectorPolicyTest` 迁入插件测试目标，保持角度边界、冷却和 Program 切换策略覆盖。
- 为直播响应解析和状态机覆盖网络错误、非法 JSON、业务失败、缺字段、非法 URL、取消、启动失败、正常停止和重复事件。
- 保持 SSDP 有效响应、缺字段、非法 IPv4、来源地址不一致、端口越界和协议版本测试要求。
- 构建 `xbotogo-falconM` 和 `obs-studio`，运行相关 CTest、clang-format、gersemi 和 `git diff --check`。
- 配置层验证插件禁用时 `obs-studio` 仍可构建，且 frontend 不引用任何插件内部头文件或符号。

人工验收：

- 顶层 XBotGo 菜单位置、两个 Action、默认右侧 Dock 和中英文文案与迁移前一致。
- 反复创建、删除、激活和停用 FalconM Source；设备连接、断开和重连无悬挂回调。
- 角色分配保留 SceneItem 布局；普通模式、Studio Mode、多 Canvas 和场景集合切换正确。
- 自动导播开关、冷却和 Center/Left/Right Program 切换正确。
- 直播获取失败、用户取消、Service 创建失败、启动失败、正常推流、停止和应用退出均清理任务状态。
- 删除或禁用插件后，OBS 前端没有 XBotGo 菜单、Dock、业务状态或本地化残留。

## 完成条件

迁移只有在上述自动验证和人工验收均有明确结果、用户已有修改保持不变、SDK/RPATH/Frameworks 打包逻辑未受
影响，并且 frontend 恢复为无 XBotGo 定制的状态后才算完成。无法执行的真实设备或服务端验证必须在交付中列明，
不得以构建成功代替行为验证。
