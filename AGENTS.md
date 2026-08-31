# AGENTS.md

本文件适用于仓库根目录及所有子目录。若子目录存在更具体的 `AGENTS.md`，则以更深层文件为准。

## 项目定位

这是基于 OBS Studio 的 XBotGo 定制工程。除上游 OBS 的采集、合成、编码、录制和推流能力外，当前业务重点包括：

- `plugins/xbotogo-falconM/`：FalconM Media SDK 音视频源插件及其 SSDP/UDP 设备发现实现，仅面向 macOS arm64。
- `frontend/xbotgo/`：XBotGo 直播配置、直播任务请求、心跳和停止逻辑。
- `frontend/widgets/OBSBasic*`、`frontend/forms/OBSBasic.ui`：XBotGo 菜单及与 OBS 主流程的集成。

开始修改前先阅读根目录 `README.md`。涉及通用 OBS 行为时，同时参考 `README.rst`、`CONTRIBUTING.md` 和 `CODESTYLE.md`。

## 工作原则

1. 先运行 `git status --short`，识别并保留用户已有修改。不要回滚、覆盖或格式化与当前任务无关的文件。
2. 使用 `rg`/`rg --files` 定位代码。先理解现有调用链和生命周期，再做局部修改。
3. XBotGo 业务改动应优先限制在上述业务目录；只有确需接入 OBS 生命周期或 UI 时才修改上游目录，避免大范围重构上游代码。
4. 不编辑 `build_*`、CMake 生成文件、打包产物或 IDE 用户配置。不要手工修改 `plugins/xbotogo-falconM/vendor/media-sdk/` 下的头文件和动态库。
5. 未经明确要求，不修改第三方依赖、子模块版本、服务端地址、协议字段或默认业务参数。
6. 不提交访问令牌、账号、推流密钥、签名材料或设备隐私数据；日志中也不得输出这些信息。

## 架构与实现约束

### OBS/libobs

- 尊重 OBS 对象的引用计数和所有权模型，优先使用工程已有的 RAII 包装类型。
- Source 的 create/update/activate/deactivate/destroy 必须保持幂等和线程安全；销毁后不得再进入 SDK 回调。
- 不在 OBS 音视频回调、渲染线程或 Qt UI 线程执行阻塞式网络、设备连接或长时间等待。
- 修改场景、Source 或推流生命周期时，检查正常停止、失败回滚、重复启动和应用退出四条路径。

### FalconM 插件

- 保留现有目标名和目录拼写 `xbotogo-falconM`，不要擅自改名。
- Media SDK 当前只有 `arm64` 产物。不要宣称或配置 x86_64/Universal 支持，除非相应 SDK 已提供。
- SDK 连接和断开应在控制线程完成；音视频数据交给 OBS 前必须确认 Source 仍有效。
- 修改 `falconm.hpp`、`falconm-source.cpp` 或 `falconm-stream.cpp` 时应把三者视为同一生命周期单元审查。
- 保持插件包内 `Frameworks/libmedia_sdk.1.0.0.dylib` 的复制逻辑和 `@loader_path` RPATH 可用。

### 设备发现

- SSDP 使用 IPv4 组播 `239.255.255.250:1900`。不要放宽来源地址校验或端口范围校验而不说明安全影响。
- 设备响应至少需要 `X-Device-ID`、`X-Device-IP`、`X-MQTT-Port` 和 `X-Protocol-Version`。
- 设备搜索必须容忍重复响应、无效数据包、多网卡、接口消失和对话框关闭。
- `Device` 结构、SSDP 解析和搜索对话框是 FalconM 插件私有实现；修改后验证插件属性页的搜索和设备选择流程。

### Qt 前端与直播任务

- 网络请求必须异步执行，并绑定有效的 Qt `context`，避免回调访问已销毁窗口。
- 直播开始流程必须覆盖：地址获取失败、用户取消、服务创建失败、推流准备失败和正常停止。
- 心跳定时器不得重复创建；同一时刻最多允许一个心跳请求在途；停止任务后清空活动任务状态。
- 推流和拉流 URL 仅接受有效的 `rtmp`/`rtmps` 地址。
- 新增或修改用户可见文案时，同步维护 `frontend/data/locale/en-US.ini` 和 `frontend/data/locale/zh-CN.ini`，代码中使用 `QTStr`/OBS 本地化机制，不硬编码界面文字。

## 代码风格

- C、C++、Objective-C 和 Objective-C++ 以根目录 `.clang-format` 为准；不要顺手格式化未修改代码。
- CMake 以 `.gersemirc` 为准，使用 2 空格缩进。
- 其他文件遵循 `.editorconfig`。所有文本使用 UTF-8、LF、文件末尾换行并清除行尾空格。
- 保持 include 顺序，不手工排序 `.clang-format` 明确要求保留的 include。
- 注释应解释约束和原因，不复述代码；新增公共接口时说明所有权、线程要求和失败语义。

针对本次修改文件检查格式：

```bash
./build-aux/run-clang-format --check --fail-error <changed-source-files>
./build-aux/run-gersemi --check --fail-error <changed-cmake-files>
```

只有在确认差异范围后才运行自动写入式格式化命令。

## 构建

当前 XBotGo 开发环境以 macOS 13+、Apple Silicon、Xcode、Qt 6 和 CMake 3.28+ 为基准。

标准配置与构建：

```bash
git submodule update --init --recursive
cmake --preset macos
cmake --build build_macos --config Debug --parallel 8
```

若仓库已有可用的 `build_macos_xcode`，可做增量验证；不要仅为普通源码改动重新生成它：

```bash
cmake --build build_macos_xcode --config Debug --target xbotogo-falconM --parallel 8
cmake --build build_macos_xcode --config Debug --target obs-studio --parallel 8
```

根据改动范围选择最小充分目标：

| 改动范围 | 至少验证 |
| --- | --- |
| `plugins/xbotogo-falconM/` | `xbotogo-falconM`；涉及前端集成时再构建 `obs-studio` |
| `frontend/xbotgo/`、主窗口或 `.ui` | `obs-studio` |
| `libobs/` 或公共 Source/Scene API | `obs-studio`、受影响插件及相关测试 |
| CMake 或依赖关系 | 重新配置后构建所有受影响目标 |
| 仅 Markdown | `git diff --check` 并检查本地链接 |

## 测试与验证

- 构建成功只是最低要求。对网络、线程或生命周期修改，应说明并人工覆盖相关成功和失败路径。
- 若构建配置启用了测试，使用 `ctest --test-dir <build-dir> -C Debug --output-on-failure` 运行相关测试。
- 修改 SSDP 解析时，应覆盖有效响应、缺字段、非法 IPv4、来源地址不一致、端口越界和非法协议版本。
- 修改直播 JSON 解析时，应覆盖网络错误、非法 JSON、非 200 业务码、缺失字段及非法 URL。
- 修改音视频路径时，至少检查反复激活/停用、删除 Source、设备断开、停止推流和退出应用。
- 无法运行某项验证时，在交付说明中明确写出未运行项目和原因，不把推测描述成验证结果。

## 完成标准

- 实现范围与用户请求一致，没有夹带无关重构。
- `git diff --check` 通过，相关格式检查和最小充分构建/测试已执行。
- 未触碰生成文件、第三方 SDK 或用户已有改动。
- 新行为、限制或构建方式变化已同步更新 `README.md` 或相邻文档。
- 最终交付说明列出修改文件、行为变化、验证结果和仍存在的限制。
