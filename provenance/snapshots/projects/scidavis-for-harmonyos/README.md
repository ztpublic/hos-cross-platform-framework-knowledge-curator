# SciDAVis for HarmonyOS

把桌面 SciDAVis 移植到 HarmonyOS 的实验版本。

**English**: [README.en.md](./README.en.md)

## 状态

**没有精力也没有财力持续维护。**  
有能力的请直接 git 下来自己修 bug、补功能。

## 已适配（已验证可用）

- **表格 (Table)**：新建、尺寸、排序、统计、列操作、公式、跳转单元格、转矩阵
- **矩阵 (Matrix)**：新建、尺寸、转置、行列式、镜像、重算、转表格
- **二维图层 (MultiLayer / Graph)**：新建、添加曲线/函数/误差棒、格式（坐标轴/网格/刻度/标题）、绘图向导、拟合向导
- **右键菜单**：三级级联、标准文本菜单、Table/Matrix/Graph 多态（Route A 阶段 0-4 全验证）
- **文件操作**：新建/打开/保存/另存为（Qt 内嵌对话框 + 工作文件夹授权）、导入 ASCII、导出 ASCII/PDF、导出图像
- **编辑**：撤销/重做历史、粘贴、列操作
- **分析**：FFT、拟合向导、相关、卷积、行列统计
- **窗口**：重命名、级联、平铺、关闭
- **视图**：首选项、脚本语言切换
- **新建项目 / 退出**：Qt 原生未保存确认框（SciDAVis / Yes / No / Cancel），保存前自动申请文件夹授权

## 未适配 / 暂不可用

- 三维曲面 / 三维图 (3D Surface / 3D Plot)
- 打印
- 工具栏开关 / 控制台
- 帮助浏览器
- 其它大量桌面高级功能（完整数据分析套件、脚本引擎等）

## 设备与系统

- **设备类型**：`tablet`、`2in1`
- **API**：兼容 API 26、目标 API 26（仅支持 API 26 及以上系统）

## 构建

- DevEco Studio (Stage 模式)
- Qt for HarmonyOS 交叉编译产出 `libentry.so`（需自行构建，未含于仓库）

## License

本项目基于 SciDAVis 移植，继承上游协议：

> **GNU General Public License version 2 or (at your option) any later version**
>
> SPDX: `GPL-2.0-or-later`

完整许可证文本见 [LICENSE](./LICENSE)，上游版权与第三方组件声明见 [NOTICE](./NOTICE)。
