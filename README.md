# XBotGo OBS Studio 工程说明

本工程基于 [OBS Studio](https://obsproject.com/) 开发，在保留 OBS 采集、合成、编码、录制和推流能力的基础上，增加了 XBotGo 设备发现、FalconM 音视频源和 XBotGo 直播任务管理能力。

> 上游 OBS 的项目介绍见 [README.rst](README.rst)，开发规范见 [CONTRIBUTING.md](CONTRIBUTING.md) 和 [CODESTYLE.md](CODESTYLE.md)。

## 当前定制能力

- **局域网设备发现**：通过 SSDP 在 `239.255.255.250:1900` 搜索 XBotGo 设备，解析设备 ID、IPv4 地址、MQTT 端口和协议版本。
- **FalconM 音视频源**：以 OBS Source 插件形式接入 FalconM Media SDK，向 OBS 输出设备音视频数据。
- **设备快速配置**：在添加 FalconM 源时搜索并选择设备，自动填写设备 ID、Broker 地址及端口。
- **XBotGo 直播任务**：从 XBotGo 服务获取 RTMP/RTMPS 推流与拉流地址，创建推流配置，并管理直播心跳和停止请求。
- **中英文界面**：XBotGo 菜单、设备搜索和直播配置已接入 OBS 本地化资源。

## 工程结构

| 目录 | 作用 |
| --- | --- |
| `libobs/` | OBS 核心库：场景、Source、Output、Encoder、Service 等基础抽象 |
| `frontend/` | Qt 桌面端；XBotGo 界面与直播逻辑位于 `frontend/xbotgo/` |
| `plugins/` | OBS 官方插件及业务插件；FalconM 插件位于 `plugins/xbotogo-falconM/` |
| `shared/xbotgo-device-discovery/` | 可被前端和插件复用的 SSDP 设备发现组件 |
| `libobs-opengl/`、`libobs-metal/` | OpenGL 和 Metal 图形后端 |
| `deps/`、`shared/` | 第三方组件及 OBS 公共组件 |
| `cmake/`、`build-aux/` | CMake 模块、依赖准备、构建和打包辅助脚本 |
| `docs/sphinx/` | libobs 开发者文档源文件 |
| `test/` | OBS 测试工程 |

XBotGo 相关模块的主要调用关系如下：

```text
OBS Qt 前端
├── XBotGo 菜单
│   ├── 设备搜索 ───────┐
│   └── 开始直播        │
├── 直播任务服务        │
│   ├── 获取推/拉流地址 │
│   ├── 定时心跳        │
│   └── 停止直播任务    │
└── FalconM Source 插件 │
    └── 设备选择 ───────┤
                        ▼
             xbotgo-device-discovery
                        │ SSDP/UDP
                        ▼
                   XBotGo 设备
```

## XBotGo 关键代码入口

| 功能 | 入口文件 |
| --- | --- |
| XBotGo 菜单与操作 | `frontend/widgets/OBSBasic_MainControls.cpp` |
| 菜单 UI 定义 | `frontend/forms/OBSBasic.ui` |
| 直播配置对话框 | `frontend/xbotgo/dialogs/XBotGoLiveStreamConfigDialog.cpp` |
| 直播任务请求、心跳与停止 | `frontend/xbotgo/services/XBotGoLiveStreamProvider.cpp` |
| 设备搜索对话框 | `shared/xbotgo-device-discovery/XBotGoDeviceSearchDialog.cpp` |
| SSDP 响应解析 | `shared/xbotgo-device-discovery/XBotGoSsdpParser.cpp` |
| FalconM Source 注册 | `plugins/xbotogo-falconM/xbotogo-falconM.cpp` |
| FalconM Source 生命周期 | `plugins/xbotogo-falconM/falconm-source.cpp` |
| Media SDK 数据接入 | `plugins/xbotogo-falconM/falconm-stream.cpp` |
| 中英文文案 | `frontend/data/locale/en-US.ini`、`frontend/data/locale/zh-CN.ini` |

## 构建环境

当前 XBotGo/FalconM 实现面向 **macOS Apple Silicon（arm64）**。仓库内的 FalconM Media SDK 仅包含 arm64 头文件和动态库，因此不能直接生成可运行的 Intel 或 Universal 版本。

建议环境：

- macOS 13 或更高版本
- Apple Silicon Mac
- Xcode 及 Xcode Command Line Tools
- CMake 3.28 或更高版本（工程当前采用最高 3.30 的 CMake Policy）
- OBS 所需的预编译依赖和 Qt 6
- Git（包含子模块）

首次拉取后初始化子模块：

```bash
git submodule update --init --recursive
```

OBS 的依赖准备方式会随上游版本变化，统一以 [官方 macOS 构建说明](https://github.com/obsproject/obs-studio/wiki/Building-OBS-Studio) 和本仓库 `CMakePresets.json` 中的依赖版本为准。

## 配置与编译

使用仓库的 macOS CMake Preset：

```bash
cmake --preset macos
cmake --build build_macos --config Debug --parallel 8
```

也可以生成独立的 Xcode 构建目录：

```bash
cmake -S . -B build_macos_xcode -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
cmake --build build_macos_xcode --config Debug --parallel 8
```

只验证 XBotGo 相关目标时，可以分别构建：

```bash
cmake --build build_macos_xcode --config Debug --target xbotgo-device-discovery --parallel 8
cmake --build build_macos_xcode --config Debug --target xbotogo-falconM --parallel 8
cmake --build build_macos_xcode --config Debug --target obs-studio --parallel 8
```

构建过程中，`plugins/xbotogo-falconM/CMakeLists.txt` 会把 `libmedia_sdk.1.0.0.dylib` 复制到插件旁的 `Frameworks` 目录，并设置运行时搜索路径。

## 使用流程

### 搜索设备并添加 FalconM 源

1. 确保 Mac 与 XBotGo 设备位于同一局域网，且网络允许 UDP 组播。
2. 在 OBS 主菜单选择 **XBotGo → 搜索设备**，确认设备能够被发现。
3. 在来源面板新增 FalconM/XBotGo 来源。
4. 在来源属性中选择搜索到的设备，确认设备 ID、IP/Broker 地址和 MQTT 端口。
5. 添加来源后检查预览画面及音频电平；隐藏来源不会断开设备连接。

设备 SSDP 响应必须包含以下字段，且 `X-Device-IP` 必须与数据包来源 IPv4 地址一致：

```text
X-Device-ID: <device-id>
X-Device-IP: <ipv4-address>
X-MQTT-Port: <1-65535>
X-Protocol-Version: <0-65535>
```

### 发起 XBotGo 直播

1. 在 OBS 主菜单选择 **XBotGo → 开始直播**。
2. 等待服务端返回推流地址、拉流地址和任务 ID。
3. 在弹出的配置窗口确认 RTMP/RTMPS 服务器及推流密钥。
4. 确认后由 OBS 创建自定义推流服务并开始推流。
5. 推流期间客户端每 10 秒发送一次任务心跳，停止推流时通知服务端结束任务。

直播服务地址及请求字段目前直接定义在 `XBotGoLiveStreamProvider.cpp` 中。切换测试/生产环境或接入认证信息时，应优先将这些配置外置，避免把密钥或令牌提交到仓库。

## 调试与排查

- **搜索不到设备**：确认双方在同一网段；检查防火墙及路由器是否允许 UDP 1900 和组播；确认设备返回了完整的 `X-*` 字段。
- **插件未加载**：检查 OBS 日志中的模块加载错误，并确认插件包内 `Frameworks/libmedia_sdk.1.0.0.dylib` 存在且为 arm64。
- **有设备但无画面**：核对设备 ID、Broker IP、MQTT 端口，以及 Media SDK 的连接和回调日志。
- **直播地址获取失败**：检查网络请求错误、服务端返回码和 JSON 字段；推流、拉流 URL 必须使用 `rtmp` 或 `rtmps`。
- **修改后未生效**：重新构建 `xbotogo-falconM` 与 `obs-studio`，确认运行的是当前构建目录产出的 App。

OBS 日志可从菜单 **帮助 → 日志文件 → 查看当前日志** 打开。XBotGo 直播请求、心跳、停止请求及部分设备发现过程会写入该日志。
FalconM 的 info 日志由编译宏 `XBOTGO_FALCONM_INFO_LOG_ENABLED` 控制，默认值为 `1`；设为 `0` 可关闭 info
日志，不影响 warning 和 error 日志。info 日志消息包含精确到毫秒的本地时间。

## 开发约定

- 避免直接修改与需求无关的上游 OBS 代码，业务代码优先放入 `frontend/xbotgo/`、`shared/xbotgo-device-discovery/` 或独立插件目录。
- 新增界面文案时同时维护 `en-US.ini` 和 `zh-CN.ini`。
- 修改共享接口后，同时构建前端与 FalconM 插件，避免两侧接口不一致。
- 不要提交构建目录、用户配置、日志、签名文件、访问令牌或其他敏感信息。
- C/C++、CMake 和提交信息遵循本仓库的 [代码风格](CODESTYLE.md) 与 [贡献指南](CONTRIBUTING.md)。

## 上游资料

- [OBS Studio 官网](https://obsproject.com/)
- [OBS 构建说明](https://github.com/obsproject/obs-studio/wiki/Building-OBS-Studio)
- [OBS 插件开发文档](https://obsproject.com/docs/plugins.html)
- [libobs API 文档](https://obsproject.com/docs/)
- [OBS Studio 源码仓库](https://github.com/obsproject/obs-studio)

## 许可证

本工程继承 OBS Studio 的 GNU General Public License v2 或更高版本授权，详情见 [COPYING](COPYING)。第三方组件及 FalconM Media SDK 仍分别受其自身许可证约束。
