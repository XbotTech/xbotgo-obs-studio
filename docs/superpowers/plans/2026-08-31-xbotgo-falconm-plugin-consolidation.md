# XBotGo/FalconM 插件收敛实施计划

> **面向执行代理：** 必须使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 子技能，逐项实施本计划。使用复选框（`- [ ]`）跟踪步骤进度。

**目标：** 将所有 XBotGo/FalconM 功能迁入唯一的 `xbotogo-falconM` 插件，同时保留当前顶层菜单和既有行为，并从 OBS 前端移除所有 XBotGo 定制代码。

**架构：** 将 `xbotogo-falconM` 建设为深模块，对外仅依赖 libobs、`obs-frontend-api` 和 Qt。插件自有的 Runtime 管理菜单、Dock、前端事件、直播任务状态和关闭流程；内部模块分别负责 FalconM 控制、角色场景、自动导播、设备搜索和直播网络请求。

**技术栈：** C++17、Qt 6 Core/Network/Widgets、libobs、obs-frontend-api、CMake 3.28+、AppleClang/Xcode、macOS arm64。

**设计规格：** `docs/superpowers/specs/2026-08-31-xbotgo-falconm-plugin-consolidation-design.md`

## 全局约束

- 保留目标名和目录拼写 `xbotogo-falconM`。
- 仅支持 Apple Silicon arm64 上的 macOS 13+；不得增加 x86_64 或 Universal 支持。
- 保持 Media SDK 头文件/库、Frameworks 复制逻辑和 `@loader_path` RPATH 不变。
- 保持 Source ID、SSDP 协议、Media SDK 行为、服务地址、请求字段和默认参数不变。
- 插件实现可以包含 libobs、`obs-frontend-api` 和 Qt，但不得包含 `OBSBasic.hpp`、`OBSApp.hpp`、`qt-wrappers.hpp`、`RemoteTextThread.hpp` 等前端私有头文件。
- 保留用户未提交的 `plugins/xbotogo-falconM/falconm-stream.cpp` 修改。实施期间不执行 `git add` 或 `git commit`；只有用户明确要求时才暂存或提交。
- 网络工作保持异步，Widget 工作在 UI 线程执行，排队访问 Source 时使用 OBS 引用保护生命周期。

## 锁定的文件结构

```text
plugins/xbotogo-falconM/
├── runtime/xbotgo-plugin-runtime.{cpp,hpp}
├── runtime/xbotgo-translation.hpp
├── ui/auto-director-control-widget.{cpp,hpp}
├── ui/falconm-control-widget.{cpp,hpp}
├── ui/falconm-devices-widget.{cpp,hpp}
├── ui/xbotgo-combo-box-control.{cpp,hpp}
├── ui/xbotgo-slider-control.{cpp,hpp}
├── live/live-stream-config.hpp
├── live/live-stream-config-dialog.{cpp,hpp}
├── live/live-stream-parser.{cpp,hpp}
├── live/live-stream-session.{cpp,hpp}
├── live/live-task-client.{cpp,hpp}
├── live/live-stream-runtime.{cpp,hpp}
├── director/auto-director.{cpp,hpp}
├── director/auto-director-policy.hpp
├── director/auto-director-policy-test.cpp
├── scenes/camera-role-scenes.{cpp,hpp}
└── falconm-source-bridge.{cpp,hpp}
```

保留现有 `device-search/`、`protocol/`、插件根目录的 FalconM 生命周期文件、vendor SDK 和插件本地化目录。只有插件侧替代实现构建成功后，才能删除前端对应代码。

---

### 任务 1：提取并测试直播领域逻辑

**文件：**
- 新建：`plugins/xbotogo-falconM/live/live-stream-config.hpp`
- 新建：`plugins/xbotogo-falconM/live/live-stream-parser.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/live/live-stream-session.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/live/live-stream-test.cpp`
- 修改：`plugins/xbotogo-falconM/CMakeLists.txt`
- 参考：`frontend/xbotgo/models/XBotGoLiveStreamConfig.hpp`
- 参考：`frontend/xbotgo/services/XBotGoLiveStreamProvider.cpp`

**接口：**
- 产出：`LiveStreamConfig`、`ParseLiveStreamUrls`、`ParseLiveStreamResponse`、`LiveStreamPhase` 和 `LiveStreamSession`。

- [x] **步骤 1：编写失败的解析器和状态测试**

覆盖有效响应、无效 JSON、非 200 业务码、缺少任务 ID、无效 RTMP URL、合法状态转换、拒绝重复启动以及任务只释放一次。

```cpp
QString error;
const auto config = xbotgo::ParseLiveStreamResponse(validResponse, error);
assert(config && config->taskId == QStringLiteral("task-1"));

xbotgo::LiveStreamSession session;
assert(session.beginFetch());
assert(!session.beginFetch());
assert(session.beginConfirming(QStringLiteral("task-1")));
assert(session.beginStarting());
assert(session.observeStreamingStarting());
assert(session.observeStreamingStarted());
assert(session.observeStreamingStopping());
assert(session.finish() == QStringLiteral("task-1"));
assert(!session.finish());
```

- [x] **步骤 2：注册并运行红灯测试**

在 `ENABLE_UNIT_TESTS` 条件下添加 `xbotgo-live-stream-test`，并链接 Qt Core 和 Network。

```bash
cmake --build build_macos --config Debug --target xbotgo-live-stream-test --parallel 8
```

预期：由于新接口尚不存在，编译失败。

- [x] **步骤 3：实现最小接口**

```cpp
struct LiveStreamConfig {
	QString pushServer, pushStreamKey, pullServer, pullStreamKey, taskId;
	QString pullUrl() const;
};

std::optional<LiveStreamConfig> ParseLiveStreamUrls(const QString &, const QString &, QString &);
std::optional<LiveStreamConfig> ParseLiveStreamResponse(const QByteArray &, QString &);

enum class LiveStreamPhase { Idle, Fetching, Confirming, Starting, Streaming, Stopping };

class LiveStreamSession {
public:
	LiveStreamPhase phase() const noexcept;
	bool beginFetch() noexcept;
	bool beginConfirming(QString taskId);
	bool beginStarting() noexcept;
	bool observeStreamingStarting() noexcept;
	bool observeStreamingStarted() noexcept;
	bool observeStreamingStopping() noexcept;
	bool startingEventObserved() const noexcept;
	std::optional<QString> finish();
};
```

原样迁移现有解析语义。`finish()` 只返回一次任务 ID，并将状态重置为 Idle。

- [x] **步骤 4：运行绿灯测试**

```bash
cmake --build build_macos --config Debug --target xbotgo-live-stream-test --parallel 8
ctest --test-dir build_macos -C Debug -R '^xbotgo-live-stream-test$' --output-on-failure
```

预期：该测试通过。

---

### 任务 2：替换前端网络和翻译依赖

**文件：**
- 新建：`plugins/xbotogo-falconM/runtime/xbotgo-translation.hpp`
- 新建：`plugins/xbotogo-falconM/live/live-task-client.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/live/live-stream-config-dialog.{cpp,hpp}`
- 修改：`plugins/xbotogo-falconM/CMakeLists.txt`
- 参考：`frontend/xbotgo/services/XBotGoLiveStreamProvider.*`
- 参考：`frontend/xbotgo/dialogs/XBotGoLiveStreamConfigDialog.*`

**接口：**
- 使用：任务 1 的直播类型。
- 产出：`Tr`、`LiveTaskClient` 和 `LiveStreamConfigDialog`。

- [x] **步骤 1：添加插件翻译并迁移对话框**

```cpp
inline QString Tr(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}
```

重命名对话框类型，将 `QTStr` 替换为 `Tr`，并保留只读拉流 URL 和 RTMP/RTMPS 校验。

- [x] **步骤 2：实现异步客户端**

```cpp
class LiveTaskClient final : public QObject {
	Q_OBJECT
public:
	using StartCallback = std::function<void(std::optional<LiveStreamConfig>, QString)>;
	using Completion = std::function<void(QString)>;
	explicit LiveTaskClient(QObject *parent = nullptr);
	void requestStart(QObject *context, StartCallback callback);
	void requestHeartbeat(QObject *context, const QString &taskId, Completion callback);
	void requestStop(QObject *context, QString taskId, Completion callback = {});
	void abortAll();
private:
	QNetworkAccessManager manager_;
	QSet<QNetworkReply *> replies_;
};
```

沿用现有 URL、请求头、JSON 字段和 10,000 ms 传输超时。使用传入的 context 连接回调。跟踪并删除每个 reply。日志只记录类别和错误，不得记录载荷、URL、密钥或任务 ID。

- [x] **步骤 3：构建并扫描**

```bash
cmake --build build_macos --config Debug --target xbotogo-falconM --parallel 8
rg -n 'OBSBasic\.hpp|OBSApp\.hpp|qt-wrappers\.hpp|RemoteTextThread' \
  plugins/xbotogo-falconM/runtime plugins/xbotogo-falconM/live
```

预期：构建成功，扫描无匹配项。

---

### 任务 3：增加 FalconM Source 边界并迁移控制界面

**文件：**
- 新建：`plugins/xbotogo-falconM/falconm-source-bridge.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/ui/xbotgo-combo-box-control.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/ui/xbotgo-slider-control.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/ui/falconm-control-widget.{cpp,hpp}`
- 修改：`plugins/xbotogo-falconM/CMakeLists.txt`
- 参考：`frontend/xbotgo/components/*`
- 参考：`frontend/xbotgo/sources/*`
- 参考：`frontend/dialogs/OBSBasicFalconMControl.*`

**接口：**
- 产出：`FalconMSourceBridge`、`FalconMControlWidget`、`ComboBoxControl` 和 `SliderControl`。

- [x] **步骤 1：实现桥接层**

```cpp
class FalconMSourceBridge final {
public:
	explicit FalconMSourceBridge(obs_source_t *source);
	OBSSource lock() const;
	std::string uuid() const;
	bool valid() const;
	bool connected() const;
	bool send(const FalconRequest &request) const;
	falconm_device_state state() const;
	bool connectMotorAngleReport(signal_callback_t, void *) const;
	void disconnectMotorAngleReport(signal_callback_t, void *) const;
	static bool IsFalconM(obs_source_t *source);
private:
	OBSWeakSource source_;
	std::string uuid_;
};
```

将 `IsFalconM` 作为唯一的 Source ID 比较入口。每个方法都锁定弱引用、拒绝已删除的 Source、通过 `obs_obj_get_data` 获取 `falconm_source`，并验证 `stream`。把现有 `XBotGoFalconMSource` 和 `XBotGoSourceObserver` 的职责收敛到该桥接层及插件自有的信号订阅中。保留现有 proc 注册以兼容外部调用；插件 UI 统一使用桥接层。

- [x] **步骤 2：迁移控件并重命名控制 Widget**

原样迁移 ComboBox/Slider 行为。将 `OBSBasicFalconMControl` 迁移为 `FalconMControlWidget`，持有一个桥接对象，并将原始 proc 调用替换为类型化请求发送和 `falconm_device_state` 读取。保留轮询、序列号检查、变焦防抖、校准、模式、拍摄参数、蜂鸣器和方向控制。

```cpp
void FalconMControlWidget::RefreshState()
{
	const falconm_device_state state = source_.state();
	// 仅在现有序列计数器递增时更新字段。
}
```

- [x] **步骤 3：构建并扫描**

```bash
cmake --build build_macos --config Debug --target xbotogo-falconM --parallel 8
rg -n 'proc_handler_call|\.\./\.\./plugins/' \
  plugins/xbotogo-falconM/ui plugins/xbotogo-falconM/falconm-source-bridge.*
```

预期：构建成功，扫描无匹配项。

---

### 任务 4：迁移摄像机角色场景和自动导播

**文件：**
- 新建：`plugins/xbotogo-falconM/scenes/camera-role-scenes.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/director/auto-director.{cpp,hpp}`
- 移动：`frontend/xbotgo/director/XBotGoAutoDirectorPolicy.hpp` 至 `plugins/xbotogo-falconM/director/auto-director-policy.hpp`
- 移动：`frontend/xbotgo/director/XBotGoAutoDirectorPolicyTest.cpp` 至 `plugins/xbotogo-falconM/director/auto-director-policy-test.cpp`
- 新建：`plugins/xbotogo-falconM/ui/auto-director-control-widget.{cpp,hpp}`
- 修改：`plugins/xbotogo-falconM/CMakeLists.txt`

**接口：**
- 使用：`FalconMSourceBridge`。
- 产出：`CameraRole`、角色场景函数、`AutoDirector` 及其控制 Widget。

- [x] **步骤 1：移动并运行未修改的策略测试**

```bash
cmake --build build_macos --config Debug --target xbotgo-auto-director-policy-test --parallel 8
ctest --test-dir build_macos -C Debug -R '^xbotgo-auto-director-policy-test$' --output-on-failure
```

预期：插件路径下的该测试通过。

- [x] **步骤 2：迁移角色场景函数**

```cpp
enum class CameraRole { Center, Left, Right };
const char *CameraRoleSceneName(CameraRole);
OBSSource GetCameraRoleScene(CameraRole);
std::optional<CameraRole> GetSourceCameraRole(obs_source_t *);
bool AssignSourceToCameraRoleScene(obs_source_t *, CameraRole);
```

使用 `obs_enum_scenes()` 枚举普通场景，使用 `obs_enum_canvases()` 和 `obs_canvas_enum_scenes()` 枚举持久画布。通过公共接口获取预览/当前场景，保留 SceneItem 状态，并使用 `obs_frontend_save()` 保存。

- [x] **步骤 3：迁移 AutoDirector 及其控件**

构造 `AutoDirector(QObject *parent = nullptr)`。通过桥接层访问 Source/角度，使用 `obs_frontend_get_current_scene()` 获取 Program，使用 `obs_frontend_set_current_scene()` 执行切换。开始冷却前重新读取 Program。将 UI 类型重命名为 `AutoDirectorControlWidget`，并保留默认启用和 3 秒冷却行为。

- [x] **步骤 4：测试、构建并扫描**

```bash
cmake --build build_macos --config Debug \
  --target xbotgo-auto-director-policy-test xbotogo-falconM --parallel 8
ctest --test-dir build_macos -C Debug -R '^xbotgo-auto-director-policy-test$' --output-on-failure
rg -n 'OBSBasic|OBSApp|qt-wrappers' \
  plugins/xbotogo-falconM/director plugins/xbotogo-falconM/scenes
```

预期：测试和构建通过，扫描无匹配项。

---

### 任务 5：组装插件自有的菜单、Dock 和直播 Runtime

**文件：**
- 新建：`plugins/xbotogo-falconM/ui/falconm-devices-widget.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/live/live-stream-runtime.{cpp,hpp}`
- 新建：`plugins/xbotogo-falconM/runtime/xbotgo-plugin-runtime.{cpp,hpp}`
- 修改：`plugins/xbotogo-falconM/xbotogo-falconM.cpp`
- 修改：`plugins/xbotogo-falconM/CMakeLists.txt`
- 修改：`plugins/xbotogo-falconM/data/locale/en-US.ini`
- 修改：`plugins/xbotogo-falconM/data/locale/zh-CN.ini`

**接口：**
- 使用：设备发现、`FalconMSourceBridge`、控制 Widget、`AutoDirector`、`LiveTaskClient`、`LiveStreamSession` 和 `obs_frontend_*`。
- 产出：一个插件自有的 `XBotGoPluginRuntime`、保持不变的顶层 **XBotGo** 菜单，以及保持不变的设备管理 Dock。

- [x] **步骤 1：迁移设备管理容器 Widget**

将 `OBSBasicFalconMDevices` 的内容迁移到 `FalconMDevicesWidget`。保留当前 Source 列表、搜索/添加流程、选中 Source 的控制项、自动导播控制项、布局和可见文案。其构造函数接收普通的 `QWidget *parent`，不得接收或转换为 `OBSBasic *`。

- [x] **步骤 2：实现直播编排状态机**

```cpp
class LiveStreamRuntime final : public QObject {
	Q_OBJECT
public:
	LiveStreamRuntime(QAction &startAction, QWidget &dialogParent, QObject *parent = nullptr);
	void start();
	void handleFrontendEvent(obs_frontend_event event);
	void shutdown();

private:
	void setActionForPhase();
	void stopTaskAndReset();
	LiveTaskClient client_;
	LiveStreamSession session_;
	QTimer heartbeatTimer_;
	bool heartbeatInFlight_ = false;
};
```

使用 libobs 创建 `rtmp_custom` 服务，通过 `obs_frontend_set_streaming_service()` 和 `obs_frontend_save_streaming_service()` 安装并持久化；之后仅通过 `obs_frontend_streaming_start()`、`obs_frontend_streaming_stop()` 和前端事件驱动推流。地址获取失败、对话框取消、服务创建失败、推流启动失败、正常停止或关闭时，使用 `std::exchange` 消费任务 ID，并且最多发送一次停止请求。保持 10 秒请求超时、单个心跳定时器，同时最多只有一个心跳请求在途。日志不得记录推流 URL、任务 ID、签名或设备标识符。

- [x] **步骤 3：实现幂等的插件 Runtime 所有权**

```cpp
class XBotGoPluginRuntime final : public QObject {
	Q_OBJECT
public:
	explicit XBotGoPluginRuntime(QObject *parent = nullptr);
	~XBotGoPluginRuntime() override;
	void requestInitialize();
	void shutdown();

private:
	static void FrontendEvent(obs_frontend_event event, void *data);
	void initializeFrontend();
	void handleFrontendEvent(obs_frontend_event event);
	bool initialized_ = false;
	QPointer<QMenu> menu_;
	QPointer<QDockWidget> dock_;
	std::unique_ptr<AutoDirector> director_;
	std::unique_ptr<LiveStreamRuntime> live_;
};
```

`requestInitialize()` 将 `initializeFrontend()` 排队到 Qt 主线程执行，同时注册 `OBS_FRONTEND_EVENT_FINISHED_LOADING` 作为兜底。`initializeFrontend()` 必须幂等：存在 **Help** 菜单时，在它前面创建顶层 **XBotGo** 菜单，否则追加到末尾；同时创建右侧 Dock 并连接现有操作。Dock 配置为可关闭、不可浮动、不可移动。`shutdown()` 注销前端回调、中止网络请求、停止定时器、断开 Source 回调并移除 Dock/菜单，且可安全重复调用。

- [x] **步骤 4：接入模块加载/卸载和本地化**

在 `obs_module_load()` 中保留 Source 注册和场景适配设置。在 `XBOTGO_FRONTEND_API` 条件下创建一个 `std::unique_ptr<XBotGoPluginRuntime>` 并请求初始化。在 `obs_module_unload()` 中，先调用 `shutdown()` 并重置该对象，再拆除 SDK。

将 `frontend/data/locale/en-US.ini` 和 `frontend/data/locale/zh-CN.ini` 中所有 `Basic.MainMenu.XBotGo.*` 键复制到插件本地化文件；只有新类确有需要时才能重命名键。插件的中英文键集合必须完全一致。所有插件用户可见文案均通过 `Tr()` 辅助函数调用 `obs_module_text()`。

- [x] **步骤 5：在旧前端集成仍存在时验证插件**

```bash
cmake --preset macos
cmake --build build_macos --config Debug --target xbotogo-falconM obs-studio --parallel 8
python3 - <<'PY'
from pathlib import Path

def keys(path):
    return {
        line.split('=', 1)[0].strip()
        for line in Path(path).read_text().splitlines()
        if line.strip() and not line.lstrip().startswith(('#', ';')) and '=' in line
    }

en = keys('plugins/xbotogo-falconM/data/locale/en-US.ini')
zh = keys('plugins/xbotogo-falconM/data/locale/zh-CN.ini')
assert en == zh, sorted(en ^ zh)
PY
```

预期：配置和两个目标均成功；本地化键比较以零状态退出。由于新旧前端实现仍会同时编译，不得启动该中间版本。

---

### 任务 6：移除前端实现并使插件可选

**文件：**
- 删除：`frontend/xbotgo/`
- 删除：`frontend/dialogs/OBSBasicFalconMControl.{cpp,hpp}`
- 删除：`frontend/dialogs/OBSBasicFalconMDevices.{cpp,hpp}`
- 删除：`frontend/dialogs/OBSBasicAutoDirectorControl.{cpp,hpp}`
- 删除：`frontend/cmake/ui-xbotgo.cmake`
- 修改：`frontend/CMakeLists.txt`
- 修改：`frontend/cmake/ui-dialogs.cmake`
- 修改：`frontend/widgets/OBSBasic.hpp`
- 修改：`frontend/widgets/OBSBasic.cpp`
- 修改：`frontend/widgets/OBSBasic_MainControls.cpp`
- 修改：`frontend/forms/OBSBasic.ui`
- 修改：`frontend/data/locale/en-US.ini`
- 修改：`frontend/data/locale/zh-CN.ini`
- 修改：`plugins/CMakeLists.txt`
- 修改：`README.md`
- 修改：`AGENTS.md`
- 修改：`docs/falconm-direction-control.md`
- 修改：`docs/falconm-source-restart.md`
- 修改：`docs/macos-packaging.md`

- [x] **步骤 1：移除前端所有权和生成式 UI 条目**

从 `OBSBasic.cpp`、`OBSBasic.hpp` 和 `OBSBasic_MainControls.cpp` 删除 XBotGo 成员、槽函数、include、初始化、拆除、操作处理器和直播状态。从 `frontend/forms/OBSBasic.ui` 删除 XBotGo 菜单/操作；从 `frontend/CMakeLists.txt` 删除 `ui-xbotgo.cmake`，并从 `frontend/cmake/ui-dialogs.cmake` 删除六个废弃对话框条目；然后删除废弃的前端类和未使用的 Slider 演示。保持无关 OBS 菜单顺序和行为不变。

- [x] **步骤 2：移除前端本地化并强制插件架构/可选性**

从两个前端本地化文件删除所有 `Basic.MainMenu.XBotGo.*` 条目。在 `plugins/CMakeLists.txt` 中，将无条件注册替换为：

```cmake
option(ENABLE_XBOTOGO_FALCONM "Enable the XBotGo FalconM plugin (macOS arm64 only)" ON)
if(ENABLE_XBOTOGO_FALCONM)
  add_obs_plugin(xbotogo-falconM PLATFORMS MACOS ARCHITECTURES arm64)
endif()
```

这是受支持的禁用边界。保持现有目标名和目录拼写不变。

- [x] **步骤 3：更新仓库指引**

编辑 `AGENTS.md` 前调用 `writing-for-agents` 技能。更新 `README.md`、`AGENTS.md` 和三份 FalconM 文档，将 `plugins/xbotogo-falconM/` 描述为 XBotGo 设备发现、前端 UI、直播任务、角色场景和自动导播的唯一所有者。删除对 `shared/xbotgo-device-discovery/` 和 `frontend/xbotgo/` 的过时引用；记录 macOS arm64 限制和 `ENABLE_XBOTOGO_FALCONM=OFF` 用法。

- [x] **步骤 4：证明插件之外不再存在 XBotGo/FalconM 实现**

```bash
rg -n -i 'xbotgo|falconm|falcon-m' \
  --glob '!plugins/xbotogo-falconM/**' \
  --glob '!docs/superpowers/**' \
  --glob '!docs/*.md' \
  --glob '!README.md' --glob '!AGENTS.md' \
  --glob '!plugins/CMakeLists.txt' .
```

预期：插件之外没有 Source、UI、本地化或构建集成匹配项。明确排除的根级注册和文档是仅有的预期引用；分别检查三份已编辑的 FalconM 文档，确保所有代码路径都指向插件内部。

- [x] **步骤 5：构建正常配置和禁用插件配置**

```bash
cmake --preset macos
cmake --build build_macos --config Debug --target xbotogo-falconM obs-studio --parallel 8
cmake --preset macos -B build_macos_no_xbotgo \
  -DENABLE_XBOTOGO_FALCONM=OFF -DENABLE_UNIT_TESTS=ON
cmake --build build_macos_no_xbotgo --config Debug --target obs-studio --parallel 8
```

预期：正常配置下插件和前端构建成功；禁用配置可以构建 `obs-studio`，且不会创建 `xbotogo-falconM` 目标，前端也不包含任何 XBotGo 符号或本地化键。

---

### 任务 7：最终验证和人工验收

**文件：**
- 仅验证；任何失败都应返回其所属任务修复，然后重新执行对应验证步骤。

- [x] **步骤 1：运行单元测试和面向集成的测试**

```bash
cmake --build build_macos --config Debug \
  --target falconm-protocol-test xbotgo-live-stream-test \
  xbotgo-auto-director-policy-test xbotogo-falconM obs-studio --parallel 8
ctest --test-dir build_macos -C Debug \
  -R '^(falconm-protocol-test|xbotgo-live-stream-test|xbotgo-auto-director-policy-test)$' \
  --output-on-failure
```

预期：三个测试和两个生产目标全部通过。

- [x] **步骤 2：仅对迁移修改运行格式和空白检查**

```bash
git diff --name-only --diff-filter=ACMR 8ef776a17 -- '*.cpp' '*.hpp' '*.h' '*.m' '*.mm' | \
  rg -v '^plugins/xbotogo-falconM/falconm-stream\.cpp$' | \
  xargs ./build-aux/run-clang-format --check --fail-error
git diff --name-only --diff-filter=ACMR 8ef776a17 -- 'CMakeLists.txt' '*.cmake' | \
  xargs ./build-aux/run-gersemi --check --fail-error
git diff --check 8ef776a17 -- . \
  ':(exclude)plugins/xbotogo-falconM/falconm-stream.cpp'
```

预期：所有命令均以零状态退出。运行自动写入式格式化前，先检查每份文件列表；不得格式化无关的用户修改。

- [ ] **步骤 3：在 macOS arm64 上执行 UI 和生命周期人工验收**

启用插件并启动 Debug 应用，验证：

1. 顶层 **XBotGo** 菜单紧邻 **Help** 之前显示，所有可见标签和布局与迁移前 UI 一致。
2. 设备管理打开右侧可关闭、不可浮动、不可移动的 Dock；重复打开/关闭不会产生重复菜单、Dock、定时器、回调或对话框。
3. 设备发现能够容忍重复响应、无效响应和对话框关闭；添加设备会创建 FalconM Source。
4. 反复激活/停用 Source、删除 Source、设备断开和应用关闭时控件均正常工作；销毁后不再触发回调。
5. 摄像机角色分配和自动导播切换正常；冷却逻辑使用当前 Program 场景。
6. 直播启动覆盖成功、地址获取失败、对话框取消、服务失败、推流启动失败、正常停止和应用退出。每个已创建任务最多收到一次停止请求；心跳请求绝不重叠。
7. 日志不包含推流 URL、任务 ID、签名、令牌或设备标识符。

- [x] **步骤 4：确认范围并保留用户已有修改**

```bash
git status --short
git diff --stat 8ef776a17
git diff -- plugins/xbotogo-falconM/falconm-stream.cpp
```

预期：迁移改动全部可追溯，生成文件/vendor 文件未被修改，用户已有的 `falconm-stream.cpp` 修改仍然存在；迁移过程没有自动暂存或提交任何文件。
