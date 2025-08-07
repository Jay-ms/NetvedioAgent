# NetvedioAgent

<div align="center">
  <img src="https://img.shields.io/badge/Platform-Luckfox%20PICO%20RV1106-blue" alt="Platform">
  <img src="https://img.shields.io/badge/AI-RetinaFace%20+%20FaceNet-green" alt="AI">
  <img src="https://img.shields.io/badge/Protocol-RTSP-orange" alt="Protocol">
  <img src="https://img.shields.io/badge/License-CC%20BY--NC%204.0-yellow" alt="License">
</div>

## 硬件平台 :wrench:

| 组件 | 型号 | 描述 |
|------|------|------|
| **主控板** :computer: | Luckfox-PICO RV1106 | 高性能嵌入式AI开发板 |
| **摄像头** :camera: | SC3366 | 高清图像传感器 |
| **云台** :gear: | SG90舵机云台 | 二轴转动云台系统 |

## 项目概述 :book:

**NetvedioAgent** 是一个基于 Luckfox-PICO RV1106 平台的智能视频跟踪服务端系统。本项目集成了先进的人脸识别技术、高效的视频编码和网络传输能力，为用户提供完整的智能监控解决方案。

### 核心特性 :sparkles:

- **双模型协作**: RetinaFace 人脸检测 + FaceNet 特征提取 
- **实时视频流**: RKMPI 硬件编码 + RTSP 网络推流 
- **智能跟踪**: PID 控制算法实现精准云台跟随 
- **双工作模式**: 自动跟踪 + 手动控制无缝切换 
- **网络通信**: TCP 协议实现客户端与服务端实时交互

## 系统工作原理 :arrows_counterclockwise:

### 初始化阶段 :rocket:
加载 RetinaFace 和 FaceNet 模型，配置 RKMPI、舵机驱动、TCP 服务，初始化 PID 控制器，使用 FaceNet 提取目标人脸特征向量。

### 多线程架构 :thread:
- **图像捕获线程** : 通过 RK_MPI_VI 获取摄像头帧，存入 `frame_queue` 队列，队列满时自动清理最早帧
- **图像处理线程** : 从队列获取最新帧，进行 RetinaFace 人脸检测和 FaceNet 特征匹配，PID 控制云台跟随目标，RKMPI 编码并 RTSP 推流
- **TCP 通信线程** : 监听客户端控制指令，处理模式切换（自动/手动）和云台控制命令

### 线程安全 :lock:
`frame_queue` 队列为共享资源，使用互斥锁和条件变量保证线程安全，防止数据竞争。系统结束时等待所有线程正常结束，释放 RKMPI 资源和模型上下文。

## AI 模型工作流程 :robot:

### 模型初始化 :package:
```cpp
init_retinaface_facenet_model(model_path, model_path2, &app_retinaface_ctx, &app_facenet_ctx);
```

**主要步骤:**
1. 加载 RetinaFace 和 FaceNet 模型文件
2. 获取模型输入输出张量属性
3. 分配模型推理内存空间
4. 配置上下文结构体参数

### 目标特征提取 :dart:
1. **图像预处理**: Letterbox 缩放，保持宽高比
2. **FaceNet 推理**: 提取目标人脸特征向量
3. **特征归一化**: 生成 `reference_out_fp32` 参考特征

### 实时推理流程 :arrows_counterclockwise:
1. **人脸检测**: RetinaFace 检测图像中所有人脸
2. **特征提取**: 对每个检测到的人脸提取特征向量
3. **相似度计算**: 计算与目标特征的欧氏距离
4. **目标选择**: 选择距离最小的人脸作为跟踪目标

### 常见问题 :warning:
- **问题**: 误识别其他人脸
- **解决方案**: 
  - 调整欧氏距离阈值 `min_norm`
  - 使用高质量的目标人脸照片

## 快速部署 :rocket:

### 环境准备 :clipboard:
- Ubuntu 18.04/20.04 LTS
- Luckfox-Pico SDK 环境
- 交叉编译工具链

### SDK 环境搭建 :books:
参考官方文档: [Luckfox-Pico-SDK](https://wiki.luckfox.com/zh/Luckfox-Pico/Luckfox-Pico-SDK)

### 获取源码 :inbox_tray:
```bash
git clone git@github.com:Jay-ms/NetvedioAgent.git
cd NetvedioAgent
```

### 配置编译 :gear:
修改 `CMakeLists.txt`:
```cmake
# 设置项目路径
set(PROJECT_DIR "/your/project/path/NetvedioAgent")

# 设置交叉编译器
set(CMAKE_C_COMPILER "/your/toolchain/path/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "/your/toolchain/path/arm-rockchip830-linux-uclibcgnueabihf-g++")
```

### 编译项目 :building_construction:
```bash
chmod +x make.sh
./make.sh
```

### 部署到设备 :outbox_tray:
```bash
# 传输可执行文件
scp NetvedioAgent root@[设备IP]:/

# 传输模型文件
scp -r model root@[设备IP]:/

# 替换目标人脸图片
scp your_target_face.jpg root@[设备IP]:/model/test.jpg
```

### 运行系统 :arrow_forward:
```bash
ssh root@[设备IP]
./NetvedioAgent ./model/RetinaFace.rknn ./model/mobilefacenet.rknn ./model/test.jpg
```

### 验证部署 :mag:
- **RTSP 视频流**: `rtsp://[设备IP]:8554/live`
- **TCP 控制端口**: `[设备IP]:8080`

---

## 开源协议 :page_facing_up:

本项目遵循 CC BY-NC 4.0 开源协议，详细内容请见 [LICENSE](LICENSE)。

---

<div align="center">
  
### 如果这个项目对你有帮助，请给个 Star 支持一下！ :star:

</div>