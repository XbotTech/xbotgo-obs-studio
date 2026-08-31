# FalconM Source 重启恢复流程

FalconM Source、设备发现、控制 UI 和前端运行时均由 `plugins/xbotogo-falconM/` 单独拥有；上游 OBS 前端不包含 XBotGo 业务实现。

本文记录 OBS 项目重启后，`falconm_source` 如何被重新创建，以及 FalconM 连接参数从哪里恢复。

## 结论

`falconm_source` 不会跨进程保存在内存中。每次重启时，OBS 会：

1. 加载 `xbotogo-falconM` 插件并注册 source 类型 `xbotogo_falconm`。
2. 读取当前场景集合的 JSON 文件。
3. 从 JSON 的 `sources` 数组找到 `id` 为 `xbotogo_falconm` 的 source。
4. 根据该 ID 调用插件的 `.create` 回调，执行 `new falconm_source`。
5. 从 source 的 `settings` 恢复 `broker_address`、`device_id` 和 `broker_port`。
6. `.create` 完成后向内部控制线程提交请求，调用 `FalconMStream::connect()` 连接设备。

## 启动与注册

OBS 启动时，`OBSApp::loadAppModules()` 调用 `obs_load_all_modules2()`，加载插件模块：

- `frontend/OBSApp.cpp:2040`

插件入口位于 `plugins/xbotogo-falconM/xbotogo-falconM.cpp`：

```cpp
bool obs_module_load(void)
{
	obs_register_source(&xbotgo::falconm_source_info);
	return true;
}
```

source 信息定义在 `plugins/xbotogo-falconM/falconm-source.cpp`，其中 ID 为：

```cpp
obs_source_info falconm_source_info = {
	.id = "xbotogo_falconm",
	.create = falconm_create,
	...
};
```

## 场景集合文件

当前场景集合名称保存在用户配置的：

```text
Basic / SceneCollection
```

OBS 根据该名称定位场景集合文件，通常为：

```text
obs-studio/basic/scenes/<场景集合名>.json
```

`OBSBasic::Load()` 读取该 JSON 文件：

- `frontend/widgets/OBSBasic_SceneCollections.cpp:1132`

如果文件不存在，OBS 会创建默认场景并重新保存项目。

## Source 保存格式

保存场景集合时，OBS 调用 `obs_save_source()` 保存所有非场景 source：

- `frontend/widgets/OBSBasic_SceneCollections.cpp:845`
- `libobs/obs.c:2437`

FalconM source 在 JSON 中的关键结构类似：

```json
{
  "id": "xbotogo_falconm",
  "name": "FalconM",
  "uuid": "...",
  "settings": {
    "broker_address": "169.254.184.18",
    "device_id": "Xbt-F-6c092e",
    "broker_port": 1883
  }
}
```

实际值以当前场景集合 JSON 为准。

## Source 加载与实例创建

加载场景集合时，`OBSBasic::LoadData()` 取出 `sources` 和 `groups` 数组，然后调用：

```cpp
obs_load_sources(sources, addMissingFiles, files);
```

- `frontend/widgets/OBSBasic_SceneCollections.cpp:1225`
- `frontend/widgets/OBSBasic_SceneCollections.cpp:1351`

`obs_load_sources()` 对每一条 source 数据调用 `obs_load_source()`。libobs 从数据中读取 `id`、名称、UUID 和 `settings`，再调用 `obs_source_create_set_last_ver()`：

- `libobs/obs.c:2256`
- `libobs/obs.c:2292`

因为插件已经注册了 `xbotogo_falconm`，libobs 可以找到对应的 `falconm_source_info`，最终执行插件的创建回调：

```cpp
static void *falconm_create(obs_data_t *s, obs_source_t *source)
{
	auto *d = new falconm_source;
	d->source = source;
	d->stream = falconm_stream_create();
	d->broker_address = obs_data_get_string(s, "broker_address");
	d->device_id = obs_data_get_string(s, "device_id");
	d->broker_port = get_broker_port(s);
	return d;
}
```

- `plugins/xbotogo-falconM/falconm-source.cpp:57`

因此，`falconm_source` 是每次启动时新分配的对象，连接参数来自 JSON 的 `settings`，而不是来自上一次运行时的 C++ 对象。

## 创建与连接

source 创建完成后会立即向内部控制线程提交连接请求：

```cpp
falconm_request_reconnect(d);
```

连接、拉流与解码不再受 OBS source 的 showing/active 状态影响。隐藏 source 或切换到其他场景时，OBS 不再合成显示该 source，但 FalconM 连接和预览流仍保持开启。只有删除 source 触发 `falconm_destroy()` 时才停流并断开连接。修改设备连接参数时会在控制线程中断开旧连接并建立新连接。

## 默认值注意事项

插件的 `falconm_defaults()` 设置默认值：

```cpp
broker_address = ""
device_id      = ""
broker_port    = 1883
```

这些默认值用于创建 source 时 settings 中没有对应字段的情况。`falconm_source` 结构体中声明的成员初始值会在 `falconm_create()` 中被 settings 读取结果覆盖，正常重启时应以场景集合 JSON 中保存的值为准。

## 完整时序

```text
OBS 启动
  -> 加载 xbotogo-falconM 动态模块
  -> obs_register_source(falconm_source_info)
  -> 读取 Basic/SceneCollection
  -> 读取 basic/scenes/<name>.json
  -> 遍历 sources 数组
  -> 根据 id=xbotogo_falconm 创建 source
  -> new falconm_source
  -> 从 settings 恢复 broker_address/device_id/broker_port
  -> 提交异步连接请求
  -> FalconMStream::connect()
  -> source 隐藏/显示不改变连接
  -> source 删除时 FalconMStream::disconnect()
```
