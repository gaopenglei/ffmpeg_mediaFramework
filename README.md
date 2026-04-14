# FFmpeg Media Framework

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)](https://github.com/example/media-framework)

一个基于FFmpeg LGPL库构建的功能全面、灵活且高效的媒体处理框架。

## 功能特性

### 基础音视频处理
- **格式转换**: 支持MP4、AVI、MKV、MP3、AAC、WAV等常见格式
- **剪辑与拼接**: 精确时间戳/帧数剪辑，多片段无缝拼接
- **裁剪与缩放**: 视频画面裁剪，多种缩放算法支持

### 滤镜与特效
- **图像滤镜**: 色彩调整、几何变换、模糊与锐化
- **音频滤镜**: 均衡器、混响与回声、降噪

### 实时处理与流媒体
- **实时捕获**: 摄像头、麦克风实时捕获与编码
- **流媒体**: RTMP推流、RTSP拉流、HLS支持

### 高级功能
- **多声道处理**: 5.1/7.1声道支持，音频混合
- **360°视频**: 投影转换，球面视频特效

### 跨平台支持
- Windows (MSVC, MinGW)
- Linux (GCC, Clang)

## 项目要求

⚠️ **重要**: 本项目严格使用FFmpeg LGPL组件，不包含任何GPL许可的代码。

FFmpeg必须以默认配置编译（不添加`--enable-gpl`选项）。

## 依赖项

### 必需依赖
- FFmpeg (LGPL配置):
  - libavformat
  - libavcodec
  - libavutil
  - libswscale
  - libswresample
  - libavfilter

### 构建工具
- CMake >= 3.16
- C编译器 (支持C11)
- C++编译器 (支持C++17)

## 构建说明

### Linux

```bash
# 安装依赖 (Ubuntu/Debian)
sudo apt-get install build-essential cmake pkg-config
sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev \
                     libswscale-dev libswresample-dev libavfilter-dev

# 构建
mkdir build && cd build
cmake ..
make -j$(nproc)

# 安装
sudo make install
```

### Windows

```powershell
# 使用vcpkg安装FFmpeg
vcpkg install ffmpeg:x64-windows

# 配置CMake
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg路径]/scripts/buildsystems/vcpkg.cmake

# 构建
cmake --build . --config Release
```

## 使用示例

### 命令行工具

```bash
# 格式转换
media-tool convert -i input.avi -o output.mp4

# 视频剪辑
media-tool clip -i input.mp4 -o output.mp4 -s 00:01:00 -e 00:02:00

# 视频拼接
media-tool concat -i input1.mp4 -i input2.mp4 -o output.mp4

# 视频缩放
media-tool scale -i input.mp4 -o output.mp4 -w 1280 -h 720

# 添加滤镜
media-tool filter -i input.mp4 -o output.mp4 --brightness 0.2 --contrast 1.1

# 推流
media-tool push -i input.mp4 -u rtmp://server/live/stream

# 查看媒体信息
media-tool info -i input.mp4
```

### API使用

```c
#include <media_framework/media_context.h>
#include <media_framework/processing/media_processor.h>

int main(void)
{
    /* 初始化框架 */
    media_init();
    
    /* 创建处理器 */
    MediaProcessor *processor = media_processor_create();
    
    /* 执行转码 */
    MediaErrorCode ret = media_processor_transcode(
        processor,
        "input.mp4",
        "output.mkv",
        MEDIA_CODEC_ID_H264,
        MEDIA_CODEC_ID_AAC
    );
    
    /* 清理 */
    media_processor_destroy(&processor);
    media_cleanup();
    
    return ret == MEDIA_SUCCESS ? 0 : 1;
}
```

## 项目结构

```
ffmpeg_media_framework/
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 项目说明
├── LICENSE                 # 许可证
├── include/                # 头文件
│   ├── core/               # 核心模块头文件
│   ├── input/              # 输入模块头文件
│   ├── processing/         # 处理模块头文件
│   ├── output/             # 输出模块头文件
│   └── utils/              # 工具模块头文件
├── src/                    # 源代码
│   ├── core/               # 核心模块实现
│   ├── input/              # 输入模块实现
│   ├── processing/         # 处理模块实现
│   ├── output/             # 输出模块实现
│   └── utils/              # 工具模块实现
├── tools/                  # 命令行工具
├── tests/                  # 测试代码
├── docs/                   # 文档
└── cmake/                  # CMake模块
```

## 模块说明

### 核心模块 (core)
- `media_types`: 基础类型定义
- `media_frame`: 媒体帧管理
- `media_packet`: 媒体包管理
- `media_context`: 全局上下文

### 输入模块 (input)
- `media_reader`: 文件读取
- `media_capture`: 设备捕获

### 处理模块 (processing)
- `media_processor`: 核心处理器
- `media_filter`: 滤镜处理
- `media_advanced`: 高级功能

### 输出模块 (output)
- `media_writer`: 文件写入
- `media_streaming`: 流媒体处理

## 测试

```bash
# 运行所有测试
cd build
ctest --output-on-failure

# 运行特定测试
./test_types
./test_reader test_video.mp4
./test_processor input.mp4 output.mp4
```

## 许可证

本项目采用LGPL v2.1许可证。详见[LICENSE](LICENSE)文件。

## 贡献指南

1. Fork本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建Pull Request

## 联系方式
   xxx
