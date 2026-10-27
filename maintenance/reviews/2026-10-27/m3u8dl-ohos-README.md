# N_m3u8DL-Tauri

基于 **Rust + Tauri 2** 对 [N_m3u8DL-CLI](https://github.com/nilaoda/N_m3u8DL-CLI)（C# / .NET）的复刻实现：一个简单易用的 **m3u8 多线程下载器**，自带可视化配置界面。

> 核心下载逻辑（`src-tauri/src/core/`）与 GUI 完全解耦，可作为独立 Rust 库复用，并通过单元测试与端到端集成测试验证。

📝 **项目介绍文章（开发者视角，含踩坑记录）**：[PROJECT_INTRO.md](docs/PROJECT_INTRO.md)

---

## 特性

- 🚀 多线程并发下载（可配置 1~256 线程）
- 🔐 AES-128-CBC 自动解密（支持 base64 / data URI / 本地文件 / http(s) 密钥，默认 IV = 分片序号）
- 📦 视频片段合并：二进制拼接（ts / fMP4→mp4）或 ffmpeg 混流为 mp4（faststart 可选）
- ⏸ 断点续传：已下载分片自动跳过
- 🐢 全局限速（KB/s）
- 🎯 下载范围：按分片序号（`1-100`）或时间段（`00:00:00-00:10:00`）
- 🌐 自定义请求头、HTTP/SOCKS5 代理、请求超时、分片重试
- 🧹 完成后自动删除临时分片目录
- 🖥 可视化配置界面：参数表单（自动记忆）、进度条 / 速度 / ETA、分色日志、取消任务、打开输出文件夹
- 🧪 端到端集成测试：本地 HTTP 服务验证全链路（解析 → 下载 → 解密 → 合并）

---

## 复刻的核心功能（对照原版 N_m3u8DL-CLI）

| 原版功能（N_m3u8DL-CLI） | 复刻实现 | 对应原版文件 |
|---|---|---|
| m3u8 链接/文件解析（http/https/本地） | `core/hls.rs`：逐行状态机解析，产出分片列表 | `Parser.cs` |
| Master List 多清晰度解析 | `core/hls.rs`：`parse_master` 收集所有清晰度，自动选择最高带宽并重新解析 | `Parser.cs` `MasterListCheck` |
| AES-128-CBC 自动解密 | `core/download.rs`：密钥缓存（base64/data/file/http）、默认 IV=`0x`+序号 32 位 hex、宽容处理 fMP4 非块对齐尾部 | `Decrypter.cs` `Parser.cs` `ParseKey` |
| 多线程下载（默认 32 线程） | `core/download.rs`：tokio 信号量并发，线程数可配置 | `DownloadManager.cs` `DoDownload` |
| 分片重试（默认 15 次） | `core/download.rs`：指数退避重试；404/400 视为片段过期快速失败 | `Downloader.cs` `Global.HttpDownloadFile` |
| 断点续传 | 已存在的 `.ts` 分片自动跳过 | `Downloader.cs` |
| 下载限速（KB/s） | `core/download.rs`：全局令牌窗口限速 | `Global.cs` `MAX_SPEED` |
| 下载范围（分片/时间段） | `core/hls.rs`：`apply_range` 按序号或累计时长过滤 | `Parser.cs` `downloadRange` |
| 自定义 HTTP Headers | `key:value\|key2:value2` 格式解析为 HeaderMap | `Global.cs` |
| HTTP/SOCKS5 代理 | reqwest 代理配置 | `Global.cs` `proxyAddress` |
| 自动合并：二进制 / ffmpeg | `core/merge.rs`：二进制拼接；ffmpeg concat demuxer 混流 mp4（faststart、多 DISCONTINUITY 分组先转 mpegts 再合并）；找不到 ffmpeg 自动回退二进制 | `FFmpeg.cs` `IsComplete` |
| 完成后删除临时目录 | `delete_after_done` 选项 | `enableDelAfterDone` |
| 请求超时 / 重试次数配置 | GUI 表单可配 | `timeOut` / `retryCount` |
| SimpleG 简易 GUI 生成参数 | 原生 HTML/CSS/JS 可视化表单（localStorage 记忆） | `SimpleGUI.html` |

### 与原版的差异（未复刻）

以下原版边界功能未包含，检测到直播流（无 `#EXT-X-ENDLIST`）会明确报错而非静默失败：

- ❌ 直播流录制（`HLSLiveDownloader.cs`）
- ❌ 优酷 / 气球云 / imooc 等专有解密（`Decode*.cs`）
- ❌ 外部音轨 / 字幕轨道下载（`#EXT-X-MEDIA`）
- ❌ `m3u8dl://` URL 协议注册
- ❌ ChaCha20 加密、`muxSetJson` 混流选项
- ❌ 命令行参数（本项目为 GUI 形态；核心逻辑可作库使用）

---

## 技术栈

| 层 | 技术 |
|---|---|
| 语言 | Rust 2021（edition 2021） |
| 桌面框架 | Tauri 2（`tauri = "2"`，WebView2） |
| HTTP | reqwest 0.12（rustls-native-roots，流式下载） |
| 异步 | tokio（full） |
| 解密 | aes 0.8 + cbc 0.1（AES-128-CBC） |
| 序列化 | serde / serde_json |
| 原生对话框 | rfd |
| 前端 | 原生 HTML / CSS / JS（无构建步骤，`frontendDist` 直接加载） |

## 项目结构

```
m3u8dl-tauri/
├── frontend/                 # GUI 前端（原生三件套，无构建）
│   ├── index.html            # 页面结构
│   ├── style.css             # 深色主题样式
│   └── app.js                # 调用 Tauri 命令、轮询进度、渲染日志
├── bin/ffmpeg/ffmpeg.exe     # 项目内置 ffmpeg（自动探测，可选）
└── src-tauri/
    ├── Cargo.toml / build.rs / tauri.conf.json
    ├── capabilities/default.json
    ├── icons/                # 程序图标（脚本生成）
    ├── src/
    │   ├── main.rs           # Windows 入口
    │   ├── lib.rs            # Tauri 应用装配（命令注册）
    │   ├── commands.rs       # 命令层：start/cancel/get_task/pick_folder/detect_ffmpeg 等
    │   └── core/             # 核心逻辑（与 Tauri 解耦）
    │       ├── options.rs    # 下载配置、请求头解析、合并方式
    │       ├── hls.rs        # m3u8 解析（Master/媒体列表/KEY/MAP/BYTERANGE/范围过滤）
    │       ├── download.rs   # 多线程下载引擎（重试/解密/断点续传/限速/取消）
    │       ├── merge.rs      # 二进制合并 + ffmpeg 混流 + ffmpeg 探测
    │       ├── progress.rs   # 进度与日志结构
    │       └── state.rs      # 任务注册表（AppState）
    └── tests/
        ├── common/mod.rs           # 测试公共辅助（迷你 HTTP 服务器）
        ├── download_e2e.rs         # 加密下载 + 二进制合并端到端测试
        └── ffmpeg_merge_e2e.rs     # ffmpeg 混流端到端测试
```

---

## 快速开始

### 环境要求 / 依赖项

| 依赖 | 版本 | 说明 |
|---|---|---|
| Rust | 1.75+（stable） | 构建工具链 |
| WebView2 运行时 | 系统内置 | Windows 10/11 一般已预装 |
| ffmpeg（可选） | 6.0+ | 用于混流为 mp4；缺失时自动回退为二进制合并（ts / fMP4→mp4） |

#### ffmpeg 二进制获取方式（任选其一）

1. **官方构建（Windows）**：打开 <https://www.gyan.dev/ffmpeg/builds/>，下载 `ffmpeg-release-essentials.zip`，解压后取 `bin/ffmpeg.exe` 使用
2. **国内镜像（速度快，单文件免解压）**：
   ```bash
   mkdir -p bin/ffmpeg
   curl -L -o bin/ffmpeg/ffmpeg.exe https://registry.npmmirror.com/-/binary/ffmpeg-static/b6.0/ffmpeg-win32-x64
   ```
3. **系统安装**：将 ffmpeg 加入 PATH（如 `winget install ffmpeg` / `choco install ffmpeg`）

> 说明：`bin/ffmpeg/ffmpeg.exe` 已加入 `.gitignore`（大文件不入库，克隆后按上述方式自行放置）。应用会自动探测该路径，也可在界面「ffmpeg 路径」手动指定；验证安装：`ffmpeg -version`。

### 构建与运行

```bash
cd m3u8dl-tauri/src-tauri

cargo build        # 开发构建（产物：target/debug/m3u8dl-tauri.exe）
cargo run          # 启动 GUI
cargo run --release  # 发布构建
```

### 运行测试

```bash
cd m3u8dl-tauri/src-tauri
cargo test         # 全部测试（单元 + 集成）
```

---

## 测试（全部通过 ✅）

`cargo test` 共 **7 个测试，0 失败**。ffmpeg 混流测试使用项目内置 `bin/ffmpeg/ffmpeg.exe`（自动探测），未找到时该用例自动跳过。

### 单元测试（`core/hls.rs`，4 个）

| 用例 | 验证内容 |
|---|---|
| `test_parse_basic` | 基础媒体列表解析：分片数、`is_vod`（ENDLIST 判断）、相对路径拼接、总时长累计 |
| `test_aes_key_default_iv` | `#EXT-X-KEY` 解析：密钥 URI 相对路径解析；未指定 IV 时默认生成 `0x` + 分片序号 32 位 hex |
| `test_master` | Master List 解析：`#EXT-X-STREAM-INF` 属性（BANDWIDTH/RESOLUTION），按带宽降序、URI 解析 |
| `test_resolve_with_query` | URL 拼接：相对分片基于 base 解析、绝对地址直用 |

### 集成测试（端到端，本地 HTTP 服务驱动真实下载链路）

| 用例 | 文件 | 验证内容 |
|---|---|---|
| `test_aes_roundtrip` | `download_e2e.rs` | AES-128-CBC 加密 → 解密往返一致（含默认 IV） |
| `test_e2e_encrypted_download_and_merge` | `download_e2e.rs` | **全链路**：前 3 片 AES-128 加密 + 后 3 片明文（KEY 状态切换）→ 4 线程并发下载 → 密钥拉取与默认 IV 解密 → 二进制合并，输出与原始分片拼接**逐字节比对一致** |
| `test_e2e_ffmpeg_merge` | `ffmpeg_merge_e2e.rs` | **ffmpeg 混流**：用真实 ffmpeg 生成 3 个 MPEG-TS 分片（H.264、独立 GOP）→ 本地服务提供 m3u8 → 多线程下载 → ffmpeg concat 混流为 mp4（faststart）→ 再用 ffmpeg 探测输出文件可正常读取 |

---

## GUI 使用说明

| 配置项 | 说明 | 默认 |
|---|---|---|
| m3u8 地址 | http/https 链接或本地文件（「选择文件」按钮） | - |
| 保存目录 | 留空为程序当前目录 | - |
| 保存文件名 | 留空自动从链接生成 | - |
| 线程数 | 并发下载线程 | 16 |
| 重试次数 | 单分片失败重试上限 | 15 |
| 超时(秒) | 网络请求超时 | 10 |
| 下载范围 | `1-100`（分片序号）或 `00:00:00-00:10:00`（时间段） | - |
| 限速 KB/s | 0 = 不限速 | 0 |
| 合并方式 | ffmpeg 混流 mp4 / 二进制拼接 ts / 不合并 | ffmpeg |
| 代理地址 | `http://...` 或 `socks5://...` | - |
| 请求头 | `key:value\|key2:value2` | - |
| ffmpeg 路径 | 留空自动探测（内置 bin/ffmpeg、PATH） | - |

表单配置会自动记忆（localStorage）。下载过程中可实时查看分片进度、速度、ETA、时长与彩色日志，支持随时取消，完成后可直接在资源管理器中打开输出目录。

---

## 许可

本复刻项目为学习/演示用途，参考 MIT 协议开源的原版 [N_m3u8DL-CLI](https://github.com/nilaoda/N_m3u8DL-CLI) 实现。请仅用于合法内容下载。
