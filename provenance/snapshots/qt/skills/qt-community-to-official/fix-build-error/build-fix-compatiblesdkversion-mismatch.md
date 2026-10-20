# compatibleSdkVersion 版本号与本机 SDK 不符

> 编号：build-fix-08
> 日期：2026-10-10（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建更新）（基于实测构建）
> 涉及文件：`build-profile.json5`
> 工程根：`QtBugDemo-official/`
> 当前状态：已修复（构建通过）

## 1. 报错

当 `.hvigor/cache` 包含旧版本缓存时，hvigor 配置阶段报出：

```
00303168 Configuration Error: SDK component missing.
   at HmosSdkLoader.getHmosSdkComponents (.../hmos-sdk-loader.js)
```

> 本次构建（清理缓存后）未触发此错误，但版本不一致仍有风险：
> hvigor 可能在增量构建时命中旧缓存导致 `SDK component missing`。

## 2. 根因

`build-profile.json5`：

```json5
"compatibleSdkVersion": "6.0.0(20)",
```

本机 DevEco SDK（`sdk-pkg.json` 实测）：

```
"platformVersion": "6.1.1",
"apiVersion": "24",
```

hvigor 解析 `6.0.0(20)` → 本地无 `6.0.0` 组件（只有 `6.1.1`）→
缓存命中时报 `SDK component missing`。

## 3. 修复

```json5
"compatibleSdkVersion": "6.1.1(24)",
```

> HarmonyOS 模式强制 `X.Y.Z(API)` 字符串格式。

## 4. 排查

遇到 `SDK component missing` (00303168) 时同时检查：

1. **版本号**（本文档）：`compatibleSdkVersion` 是否与本机 SDK 一致
2. **目录层级**（build-fix-10 关联）：`DEVECO_SDK_HOME` 是否指向
   含 `default/` 子目录的**父目录**
3. **缓存**：清理 `.hvigor/cache` 后重试（`rm -rf .hvigor/cache`）

## Auto-fix

```yaml
match: "SDK component missing|00303168"
files:
  - build-profile.json5
actions:
  - type: replace
    from: '"compatibleSdkVersion": "6.0.0(20)"'
    to: '"compatibleSdkVersion": "6.1.1(24)"'
```
