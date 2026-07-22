# DefectInspect

基于 Qt5 与 OpenCV 的 PCB 缺陷检测软件。

系统支持检测图片、模板图和图片文件夹导入，可在界面中完成检测参数配置、原图与结果图对比、缺陷信息展示、运行日志查看及检测记录保存。项目使用 OpenCV 实现模板差分、灰度化、滤波降噪、阈值分割、形态学处理和轮廓提取，并根据轮廓面积、外接矩形尺寸、长宽比和填充率等特征判断异常区域。

图像检测任务通过 QThread 和 Worker-Object 模式在独立线程中执行，避免大尺寸图像处理阻塞 Qt 主界面。项目以 Linux 为主要运行环境，支持 CMake 和 qmake 两种构建方式。

## 功能列表

- 导入单张待检测图片并自动执行检测。
- 导入无缺陷模板图，通过模板差分增强缺陷区域。
- 递归导入图片文件夹，支持上一张、下一张切换。
- 显示原始图片、缺陷标注结果、检测结论和处理耗时。
- 根据面积、尺寸、长宽比和填充率筛选并分类异常轮廓。
- 动态调整阈值、滤波核、形态学核和最小缺陷面积等参数。
- 使用 QThread 在工作线程中执行检测算法。
- 在界面中展示缺陷位置、面积、长宽比及缺陷类型。
- 自动将检测信息追加写入 CSV 记录文件。
- 支持保存带缺陷框和类别标签的结果图片。

## 技术栈

- C++11
- Qt5 Widgets
- OpenCV
- QThread、信号槽与 Qt 元对象系统
- CMake / qmake
- GCC / Linux
- DeepPCB 数据集
- CSV 检测记录

## 系统架构

```text
用户操作
   |
   v
MainWindow
界面显示、参数读取、任务提交、结果保存
   |
   | QueuedConnection
   v
DetectionWorker  <---- QThread 工作线程
检测任务入口、结果信号发送
   |
   v
DefectDetector
OpenCV 图像处理、轮廓提取、缺陷判定
   |
   v
DetectionResult
原图、结果图、缺陷列表、结论、耗时
   |
   v
MainWindow
更新界面、写入 CSV、保存结果图
```

主要模块说明：

| 模块 | 作用 |
| --- | --- |
| `MainWindow` | 构建界面，处理用户操作，读取参数并提交检测任务 |
| `DetectionWorker` | 工作线程中的任务入口，调用检测算法并发送完成信号 |
| `DefectDetector` | 封装 OpenCV 检测流程和缺陷判定逻辑 |
| `DetectionTypes` | 定义检测参数、缺陷信息和检测结果等数据结构 |
| `ImageUtils` | 完成 `cv::Mat` 与 `QImage` 之间的转换 |
| `OpenCvCompat` | 兼容不同 OpenCV 版本中的常量和接口名称 |

## 检测流程

导入模板图后，系统执行以下处理流程：

```text
模板图与待检测图
        |
        v
灰度化与高斯滤波
        |
        v
absdiff 绝对差分
        |
        v
固定阈值或 Otsu 阈值分割
        |
        v
形态学开运算与闭运算
        |
        v
外部轮廓提取
        |
        v
面积、尺寸、长宽比过滤
        |
        v
缺陷分类与结果框绘制
```

模板图是无缺陷的标准参考图。正常区域在两张图片中较为相似，差分后像素值接近零；存在断路、缺口或其他异常的位置会在差分图中形成明显区域，便于后续分割和轮廓提取。

未导入模板图时，系统直接对待检测图片进行灰度阈值分割。这种模式可用于背景较简单、目标与背景灰度差异明显的图像。

## 项目目录

```text
DefectInspect/
  CMakeLists.txt
  CMakeSettings.json
  DefectInspect.pro
  README.md
  include/
    DetectionTypes.h
    DetectionWorker.h
    DefectDetector.h
    ImageUtils.h
    MainWindow.h
    OpenCvCompat.h
  src/
    main.cpp
    DetectionWorker.cpp
    DefectDetector.cpp
    ImageUtils.cpp
    MainWindow.cpp
  data/
    samples/
  logs/
  results/
```

## 环境依赖

编译前需要准备以下组件：

- 支持 C++11 的 GCC/G++
- Qt5 Core、Gui、Widgets 开发库
- OpenCV 开发库
- CMake 或 qmake
- make
- pkg-config

可以使用以下命令检查关键依赖：

```bash
g++ --version
qmake-qt5 -v
cmake3 --version
pkg-config --modversion opencv
pkg-config --cflags --libs opencv
```

不同 Linux 发行版中的命令名称可能有所不同，例如 Qt5 的 qmake 也可能直接命名为 `qmake`，CMake 也可能命名为 `cmake`。

## 编译与运行

### 使用 CMake

```bash
cd DefectInspect
mkdir -p build
cd build
cmake3 ..
make -j$(nproc)
./DefectInspect
```

如果系统使用 `cmake` 命令：

```bash
cmake ..
make -j$(nproc)
./DefectInspect
```

CMake 会优先查找 `OpenCVConfig.cmake`。如果系统中的 OpenCV 未提供该文件，项目会自动使用 `pkg-config opencv` 获取头文件路径和链接库。

### 使用 qmake

```bash
cd DefectInspect
qmake-qt5 DefectInspect.pro
make -j$(nproc)
./DefectInspect
```

如果 Qt5 的工具命令为 `qmake`：

```bash
qmake DefectInspect.pro
make -j$(nproc)
./DefectInspect
```

该程序是桌面 GUI 应用，需要在具有图形桌面环境的 Linux 会话中运行。仅通过普通 SSH 终端启动时，如果没有配置图形显示环境，可能出现无法连接显示服务器的错误。

## DeepPCB 数据集使用方法

DeepPCB 中每组数据通常包含模板图、待检测图和标注文件：

```text
00041000_temp.jpg    无缺陷模板图
00041000_test.jpg    待检测图片
00041000.txt         真实缺陷标注
```

标注文件中每行数据的格式为：

```text
x1 y1 x2 y2 type
```

其中 `type` 对应 DeepPCB 的六种原始缺陷类型：

| 编号 | 类型 |
| --- | --- |
| 1 | Open |
| 2 | Short |
| 3 | Mousebite |
| 4 | Spur |
| 5 | Copper |
| 6 | Pin-hole |

当前版本的推荐操作步骤：

1. 启动 `DefectInspect`。
2. 点击“导入模板图”，选择某个编号的 `_temp.jpg`。
3. 点击“导入图片”，选择相同编号的 `_test.jpg`。
4. 图片导入后系统自动提交检测任务。
5. 根据结果调整阈值、滤波核和最小缺陷面积等参数。
6. 点击“执行检测”重新检测当前图片。
7. 点击“保存结果图”保存带标注框的检测结果。

例如，下面两个文件应配对使用：

```text
00041000_temp.jpg
00041000_test.jpg
```

不要使用一个模板图检测其他编号或其他组的待检测图片。模板不匹配、图像位置偏移或拍摄条件变化会产生大量差分区域，从而增加误检。

当前“导入文件夹”功能会递归读取目录中的所有图片，并且整个程序只保存一个模板路径。因此直接选择 DeepPCB 的总目录时，可能同时读取 `_temp.jpg` 和 `_test.jpg`，不适合作为自动配对评估。第一版建议手动选择同编号的模板图和待检测图。

## 参数说明

| 参数 | 默认值 | 作用 |
| --- | ---: | --- |
| 分割阈值 | 40 | 将差分图或灰度图转换为二值图 |
| 滤波核大小 | 3 | 高斯滤波核尺寸，用于减少图像噪声 |
| 形态学核大小 | 3 | 控制开闭运算的处理范围 |
| 最小缺陷面积 | 30 | 过滤面积过小的噪声轮廓 |
| 最小宽度 | 3 | 过滤宽度过小的轮廓 |
| 最小高度 | 3 | 过滤高度过小的轮廓 |
| 划痕长宽比 | 4.0 | 长宽比达到该值时优先判断为划痕 |
| 最大长宽比 | 25.0 | 过滤形状过于狭长的异常轮廓 |
| 反向阈值 | 关闭 | 使用反向二值化处理亮暗关系相反的图像 |
| Otsu 自动阈值 | 关闭 | 自动计算分割阈值，启用后忽略手动阈值 |

滤波核和形态学核在检测前会被转换为有效的正奇数，避免 OpenCV 因核尺寸无效而报错。

## 检测结果

检测完成后，界面会显示：

- 检测结论：存在有效缺陷时为 `NG`，否则为 `OK`。
- 缺陷数量：通过参数过滤后的轮廓数量。
- 处理耗时：检测算法执行时间，单位为毫秒。
- 缺陷列表：缺陷类型、面积、长宽比、位置和尺寸。
- 结果图片：在原图上绘制缺陷外接矩形及英文类别标签。

当前启发式分类结果包括：

- 划痕
- 缺口
- 污点
- 异常

这些类别由几何特征规则产生，与 DeepPCB 的六种原始标签并非一一对应。

## CSV 记录

每次成功检测后，程序会自动追加记录到可执行文件所在目录下的：

```text
logs/detection_records.csv
```

CSV 字段如下：

```text
time,image_path,template_path,defect_count,conclusion,elapsed_ms
```

| 字段 | 含义 |
| --- | --- |
| `time` | 检测完成时间 |
| `image_path` | 待检测图片路径 |
| `template_path` | 当前模板图片路径 |
| `defect_count` | 检出的缺陷数量 |
| `conclusion` | 检测结论 `OK` 或 `NG` |
| `elapsed_ms` | 检测处理耗时 |

“保存结果图”会将当前标注结果保存为 PNG 或 JPEG 文件，默认保存目录为可执行文件所在目录下的 `results/`。

## 当前版本限制

- 模板图与待检测图需要具有较好的位置和尺寸对应关系，目前未实现特征点配准和透视校正。
- 文件夹导入尚未根据 `_temp.jpg`、`_test.jpg` 和 `.txt` 自动完成 DeepPCB 数据配对。
- 当前缺陷类别由轮廓几何特征规则得到，不等同于 DeepPCB 的六种标准类别。
- 当前程序尚未读取 `.txt` 标注文件，因此不会直接计算 Precision、Recall、F1、IoU 或 mAP。
- 阈值和特征规则需要根据图像尺寸、光照及缺陷形态进行调整。

后续版本可以增加 DeepPCB 自动配对、标注解析、IoU 匹配、Precision/Recall/F1 统计、图像配准及批量评估功能。

## 常见问题

### CMake 找不到 OpenCVConfig.cmake

先确认 pkg-config 能找到 OpenCV：

```bash
pkg-config --modversion opencv
pkg-config --cflags --libs opencv
```

项目的 CMake 配置会在找不到 `OpenCVConfig.cmake` 时自动回退到 `pkg-config opencv`。如果上述命令也失败，需要安装 OpenCV 开发包或配置 `PKG_CONFIG_PATH`。

### qmake 命令不存在

检查 Qt5 的 qmake 名称和路径：

```bash
which qmake
which qmake-qt5
qmake-qt5 -v
```

### 为什么必须使用 ./DefectInspect

Linux 默认不会在当前目录搜索可执行文件。`./DefectInspect` 中的 `./` 明确表示运行当前目录下的程序。

### 导入模板后出现大量缺陷框

检查模板图与待检测图是否为相同编号，确认两张图片的尺寸、方向和位置基本一致，然后适当提高分割阈值、最小缺陷面积或形态学核大小。

## License

本项目用于学习、课程实践和个人项目展示。DeepPCB 数据集的使用请遵循其原始项目的许可要求。
