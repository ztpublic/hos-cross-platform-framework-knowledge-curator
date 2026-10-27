# LiteIDE -OH

LiteIDE 是由 visualfc 开发的一款专为 Go 语言设计的轻量级 IDE，是 Go 语言生态里历史最悠久、最纯粹的 Qt5 IDE，在 GitHub 有 7.5K stars。

把 LiteIDE 搬到鸿蒙 PC 上的意义：

填补鸿蒙 PC 的"Go 语言开发工具链" —— 鸿蒙 PC 目前没有原生的 Go IDE，LiteIDE 是这个赛道最成熟的开源方案；
验证"qmake 项目 + ohos-clang mkspec"的移植模式 ——LiteIDE 是第一个 qmake 项目，需要解决 host qmake 5.15 vs target Qt-OHOS 5.12.12 的版本错配问题；
验证"插件体系完整移植" —— LiteIDE 有 30+ 个插件，每个插件都是独立的 .so，这是本系列迄今为止最复杂的产物体系。
