# FalconM 云台方向控制与角度反馈

本文记录 FalconM 云台控制功能在 OBS 中的实现方式、协议映射和主要代码入口。

## 功能概览

当前支持：

- 上、下、左、右、回中五个方向。
- 短按、长按、释放三种操作。
- 主动查询当前水平和垂直角度。
- 开启或关闭设备周期性角度上报。
- 显示连接状态、角度和水平/垂直限位状态。
- 每个 FalconM source 对应一个独立的非模态控制窗口。

控制链路如下：

```text
Qt 控制窗口
  -> OBS source proc_handler
  -> FalconMStream
  -> MQTT 信令
  -> FalconM 设备
```

## MQTT 协议映射

### 方向控制：AYR

发送两个字节：

```text
[direction, operation]
```

方向枚举定义在 `plugins/xbotogo-falconM/falconm-stream.hpp`：

```text
0 up
1 down
2 left
3 right
4 center
```

操作枚举为：

```text
0 short press
1 long press
2 release
```

控制窗口只使用长按控制模式：按钮按下时立即发送 long press，按钮释放时发送 release。协议仍保留 short press 枚举，供其他调用方或后续功能使用，但当前方向按钮不会发送 short press。

### 角度查询：BXR

发送一个值为 `0` 的字节，请求设备返回当前电机角度。

### 周期上报：DGR

发送一个字节控制周期性上报：

```text
0 disable
1 enable
```

设备连接成功后自动开启上报并立即查询一次角度；source 停止或断开前关闭上报。

### 角度数据：BXA / DFA

`BXA` 和 `DFA` payload 使用大端序解析。

`BXA` 至少包含：

```text
result      uint16
horizontal  int32
vertical    int32
```

`DFA` 另外包含限位字节：

```text
result             uint16
horizontal         int32
horizontal_limit   uint8
vertical           int32
vertical_limit     uint8
```

角度在协议层以百分之一度的整数保存，前端显示时除以 `100.0`。

## Stream 层实现

`FalconMStream` 增加了以下接口：

```cpp
sendDirection(direction, operation)
queryMotorAngle()
setMotorAngleReportEnabled(enabled)
motorAngle()
```

MQTT 回调在 `FalconMStreamSdk::onPeerMessage()` 中识别 `BXA` 和 `DFA`，完成长度检查、大小端转换和有符号角度解析，然后写入线程安全的角度缓存。缓存由 `std::mutex` 保护，供 UI 线程读取。

原有的通用 signaling callback 仍然保留，因此新增协议解析不会影响其他信令处理。

## OBS source 接口

FalconM source 在创建时注册以下 proc_handler 方法：

```text
send_direction(direction, operation, out success)
query_motor_angle(out success)
set_motor_angle_reporting(enabled, out success)
get_motor_angle(out result, out horizontal, out vertical,
                out horizontal_limit, out vertical_limit)
```

前端只依赖 OBS 的 `proc_handler` 和 `calldata` API，不直接依赖 MQTT SDK。这样每个 source 都可以独立绑定自己的设备连接和角度状态。

### `obs_source_get_proc_handler` 用法

`obs_source_get_proc_handler()` 返回由 libobs 管理的 handler，调用方不能手动释放。source 创建时取得 handler 并注册过程：

```cpp
proc_handler_t *ph = obs_source_get_proc_handler(source);
proc_handler_add(ph,
                 "void send_direction(int direction, int operation, out bool success)",
                 falconm_send_direction,
                 source_data);
```

调用方使用 `calldata_t` 传入 `in` 参数，并从同一个对象读取 `out` 参数：

```cpp
calldata_t cd;
calldata_init(&cd);
calldata_set_int(&cd, "direction", 0);
calldata_set_int(&cd, "operation", 0);

bool called = proc_handler_call(obs_source_get_proc_handler(source),
                                "send_direction", &cd);

bool success = false;
calldata_get_bool(&cd, "success", &success);
calldata_free(&cd);
```

声明字符串决定参数名、类型和 `in`/`out` 方向；`proc_handler_call()` 的返回值表示是否找到并调用了该过程，过程自身的执行结果应通过 `out` 参数返回。

OBS 通用 API 说明见：

- `docs/sphinx/reference-libobs-callback.rst` 的 Procedure handlers 小节。
- `docs/sphinx/reference-sources.rst` 的 `obs_source_get_proc_handler` 小节。

## Qt 控制窗口

控制窗口文件：

- `frontend/dialogs/OBSBasicFalconMControl.hpp`
- `frontend/dialogs/OBSBasicFalconMControl.cpp`

窗口每 500ms 调用 `get_motor_angle` 刷新：

- Active/Inactive 连接状态。
- 水平角度和垂直角度。
- 水平限位和垂直限位。

设备管理窗口通过设备列表双击打开控制窗口。同一个 source 只允许一个控制窗口；如果窗口已经存在，则将其置前并激活。

设备列表和控制窗口都使用公开的 source 引用 API：

```cpp
obs_source_get_ref(source)
obs_source_release(source)
```

不能在前端使用只对 libobs 内部可见的 `obs_source_addref`。

## 生命周期

```text
source create
  -> 注册 proc_handler
  -> source activate
  -> FalconMStream::connect
  -> 开启 DGR + 查询 BXR
  -> 接收 BXA/DFA 并更新角度缓存
  -> UI 定时读取缓存
  -> source deactivate
  -> 关闭 DGR
  -> stopStreaming / disconnect
```

连接失败时 `falconm_activate()` 会立即返回，不再继续执行后续启动流程。

## 工程与本地化

新增控制窗口已加入 `frontend/cmake/ui-dialogs.cmake`，并增加了英文和中文方向按钮文本：

- `frontend/data/locale/en-US.ini`
- `frontend/data/locale/zh-CN.ini`

## 验证状态

已完成：

- `clang-format --dry-run --Werror`。
- `git diff --check`。
- FalconM 插件源码编译阶段验证。

完整 macOS Xcode 构建曾进入新增 FalconM 文件的编译阶段；后续重试受到当前环境无法写入用户级 Xcode `DerivedData/PIFCache` 的权限错误阻断。该错误发生在 Xcode 依赖图生成阶段，不是源码诊断错误。
