# macOS 打包与发布

本文记录如何为 XBotGo OBS Studio 生成可安装的 macOS 磁盘映像（DMG），以及内部测试包和正式发布包的签名、公证与验证流程。

## 支持范围

- 当前仅支持 Apple Silicon（arm64）。FalconM Media SDK 没有提供 x86_64 产物，因此不能生成可运行的 Intel 或 Universal 安装包。
- 仓库打包脚本生成包含 `OBS.app` 和“应用程序”目录快捷方式的 DMG，不生成 PKG。
- 当前应用仍沿用上游 OBS 的应用名称、Bundle ID、图标和 Sparkle 更新地址。作为 XBotGo 产品正式发布前，应先完成相应品牌配置。

## 环境准备

打包必须在 Apple Silicon Mac 上进行，并准备以下环境：

- Xcode 和 Xcode Command Line Tools；
- CMake、Git、Zsh 5.9 或更高版本；
- Homebrew；打包辅助脚本会通过 `.github/scripts/.Brewfile` 检查所需工具；
- OBS 预编译依赖、Qt 6 和 CEF，默认位于仓库的 `.deps/`；
- 已初始化的 Git 子模块。

当前 `.github/scripts/utils.zsh/check_macos` 会检查 macOS 26.0 或更高版本。该限制可能随上游 OBS 构建脚本变化，应以当前脚本为准。

首次准备仓库时执行：

```bash
git submodule update --init --recursive
```

以下命令均在仓库根目录执行。

## 生成内部测试 DMG

内部测试包使用 ad-hoc 签名，不需要 Apple Developer 证书。

先准备仓库内的 Xcode 编译缓存目录：

```bash
mkdir -p build_macos/CompilationCache.noindex
```

先构建 arm64 Release 应用：

```bash
CI=1 \
GITHUB_EVENT_NAME=workflow_dispatch \
GITHUB_REF_NAME=local \
XCODE_CAS_PATH="$PWD/build_macos/CompilationCache.noindex" \
.github/scripts/build-macos \
  --target macos-arm64 \
  --config Release
```

构建成功后，待打包应用位于：

```text
build_macos/OBS.app
```

生成 DMG：

```bash
CI=1 \
.github/scripts/package-macos \
  --target macos-arm64 \
  --config Release \
  --package
```

最终产物位于 `build_macos/`，文件名类似：

```text
obs-studio-<version>-<commit>-macos-apple.dmg
```

双击 DMG 后，将 `OBS.app` 拖入“应用程序”目录即可安装。ad-hoc 签名包适合开发和内部测试；复制到其他 Mac 后可能被 Gatekeeper 拦截，需要由测试人员在“系统设置 → 隐私与安全性”中确认允许打开。

## 生成正式发布 DMG

面向普通用户分发时，应使用 Apple Developer ID 对应用和 DMG 签名，并提交 Apple Notarization。开始前需要：

- 已安装到登录钥匙串的 `Developer ID Application` 证书及私钥；
- Apple Developer Team ID；
- 构建配置所需的 Provisioning Profile UUID；
- 用于公证的 Apple ID 和 Apple 专用密码。

通过当前 Shell 或 CI Secret 提供签名信息：

```bash
export CODESIGN_IDENT='Developer ID Application: Company Name (TEAM_ID)'
export CODESIGN_TEAM='TEAM_ID'
export PROVISIONING_PROFILE='PROVISIONING_PROFILE_UUID'
export CODESIGN_IDENT_USER='developer@example.com'
export CODESIGN_IDENT_PASS='APPLE_APP_SPECIFIC_PASSWORD'
```

这些值属于敏感信息，不得写入源码、CMake 文件、文档实例、日志或提交记录。团队发布应优先使用 CI Secret。

执行签名构建：

```bash
CI=1 \
GITHUB_EVENT_NAME=workflow_dispatch \
GITHUB_REF_NAME=local \
XCODE_CAS_PATH="$PWD/build_macos/CompilationCache.noindex" \
.github/scripts/build-macos \
  --target macos-arm64 \
  --config Release \
  --codesign
```

生成、签名并公证 DMG：

```bash
CI=1 \
.github/scripts/package-macos \
  --target macos-arm64 \
  --config Release \
  --package \
  --codesign \
  --notarize
```

`package-macos` 会使用 `notarytool` 等待公证完成，并用 `stapler` 将公证票据附加到 DMG。任何签名或公证步骤失败都不应发布该产物。

## 验证产物

先将实际生成的 DMG 路径保存到变量：

```bash
xbotgo_dmg_path='build_macos/obs-studio-<version>-<commit>-macos-apple.dmg'
```

验证 DMG 文件结构：

```bash
hdiutil verify "$xbotgo_dmg_path"
```

验证 App 签名：

```bash
codesign --verify --deep --strict --verbose=2 build_macos/OBS.app
codesign -dv --verbose=2 build_macos/OBS.app
```

确认主程序是 arm64：

```bash
file build_macos/OBS.app/Contents/MacOS/OBS
```

确认 FalconM 插件及 Media SDK 已进入 App Bundle：

```bash
find build_macos/OBS.app/Contents/PlugIns/xbotogo-falconM.plugin \
  -name 'libmedia_sdk*.dylib' \
  -print
```

预期能够找到：

```text
build_macos/OBS.app/Contents/PlugIns/xbotogo-falconM.plugin/Contents/Frameworks/libmedia_sdk.1.0.0.dylib
```

正式发布包还应验证公证票据和 Gatekeeper 接受状态：

```bash
xcrun stapler validate "$xbotgo_dmg_path"
spctl -a -vv -t open --context context:primary-signature "$xbotgo_dmg_path"
```

最后应在一台没有开发环境和签名证书的 Apple Silicon Mac 上完成安装、首次启动、FalconM 插件加载、设备连接、预览、录制和直播验证。

## 常见问题

### 打包脚本提示 `requires CI environment`

仓库的 `build-macos` 和 `package-macos` 是面向 CI 设计的脚本，本地调用时需要设置 `CI=1`。

### Xcode 提示 `CAS cannot be initialized` 或 `builtin/lock: Operation not permitted`

`macos-ci` 预设启用了 Xcode Compilation Cache。本地构建必须先创建可写缓存目录，并通过 `XCODE_CAS_PATH` 传给 `build-macos`；不要省略文档构建命令中的该变量。

### 提示找不到 `build_macos/OBS.app`

必须先成功执行 `build-macos`。普通的 `cmake --build` 产物通常位于 `build_macos/frontend/<config>/OBS.app`，而 `package-macos` 明确读取 `build_macos/OBS.app`。

### FalconM 插件无法加载

确认 App Bundle 中同时存在 `xbotogo-falconM.plugin` 和 `Frameworks/libmedia_sdk.1.0.0.dylib`，并检查二者均为 arm64。然后查看 OBS 日志中的模块加载、动态库搜索路径和签名错误。

### 其他 Mac 无法直接打开内部测试包

ad-hoc 签名不能替代 Developer ID 签名和 Apple 公证。需要无人工确认地分发给普通用户时，应生成正式发布包。
