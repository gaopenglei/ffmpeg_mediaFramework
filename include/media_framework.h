/**
 * @file media_framework.h
 * @brief FFmpeg媒体框架主头文件
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 这是媒体框架的主头文件，包含了所有模块的接口定义。
 * 用户只需包含此文件即可使用框架的所有功能。
 * 
 * @note 本框架严格使用FFmpeg LGPL组件，不包含任何GPL许可的代码。
 * 
 * @example 基本使用示例
 * @code
 * #include <media_framework.h>
 * 
 * int main(void) {
 *     // 初始化框架
 *     media_init();
 *     
 *     // 创建处理器
 *     MediaProcessor *proc = media_processor_create();
 *     
 *     // 执行转码
 *     media_processor_transcode(proc, "input.mp4", "output.mkv",
 *                               MEDIA_CODEC_ID_H264, MEDIA_CODEC_ID_AAC);
 *     
 *     // 清理
 *     media_processor_destroy(&proc);
 *     media_cleanup();
 *     
 *     return 0;
 * }
 * @endcode
 */

#ifndef MEDIA_FRAMEWORK_H
#define MEDIA_FRAMEWORK_H

/* 版本信息 */
#define MEDIA_FRAMEWORK_VERSION_MAJOR 1
#define MEDIA_FRAMEWORK_VERSION_MINOR 0
#define MEDIA_FRAMEWORK_VERSION_PATCH 0

/* 核心模块 */
#include "core/media_types.h"
#include "core/media_context.h"
#include "core/media_frame.h"
#include "core/media_packet.h"

/* 输入模块 */
#include "input/media_reader.h"
#include "input/media_capture.h"

/* 处理模块 */
#include "processing/media_processor.h"
#include "processing/media_filter.h"
#include "processing/media_advanced.h"

/* 输出模块 */
#include "output/media_writer.h"
#include "output/media_streaming.h"

/* 工具模块 */
#include "utils/media_utils.h"

/**
 * @defgroup MediaFramework FFmpeg媒体框架
 * @{
 * 
 * @brief 一个功能全面、灵活且高效的媒体处理框架
 * 
 * 本框架基于FFmpeg LGPL库构建，提供以下功能：
 * - 基础音视频处理（格式转换、剪辑、拼接、裁剪、缩放）
 * - 滤镜与特效（色彩调整、几何变换、模糊锐化、音频处理）
 * - 实时处理与流媒体（设备捕获、推拉流）
 * - 高级功能（多声道处理、360°视频）
 * 
 * @section quick_start 快速开始
 * 
 * @subsection installation 安装
 * 
 * @code{.sh}
 * # Linux
 * mkdir build && cd build
 * cmake ..
 * make -j$(nproc)
 * sudo make install
 * @endcode
 * 
 * @subsection basic_usage 基本使用
 * 
 * @code{.c}
 * #include <media_framework.h>
 * 
 * int main(void) {
 *     media_init();
 *     
 *     // 格式转换
 *     MediaProcessor *proc = media_processor_create();
 *     media_processor_convert(proc, "input.avi", "output.mp4");
 *     media_processor_destroy(&proc);
 *     
 *     media_cleanup();
 *     return 0;
 * }
 * @endcode
 * 
 * @section architecture 架构设计
 * 
 * 框架采用模块化设计，分为以下模块：
 * 
 * - @ref CoreModule 核心模块：基础类型定义、帧/包管理、全局上下文
 * - @ref InputModule 输入模块：文件读取、设备捕获
 * - @ref ProcessingModule 处理模块：转码、滤镜、高级处理
 * - @ref OutputModule 输出模块：文件写入、流媒体
 * - @ref UtilsModule 工具模块：日志、内存、字符串处理
 * 
 * @section lgpl_compliance LGPL合规性
 * 
 * 本框架严格遵循LGPL v2.1许可证：
 * - 仅使用FFmpeg LGPL组件
 * - 不包含任何GPL许可的代码
 * - FFmpeg必须以默认配置编译（不添加--enable-gpl）
 * 
 * @}
 */

/**
 * @defgroup CoreModule 核心模块
 * @ingroup MediaFramework
 * @{
 * 
 * @brief 核心数据类型和管理接口
 * 
 * 核心模块提供框架的基础设施：
 * - 基础类型定义（枚举、结构体）
 * - 媒体帧管理
 * - 媒体包管理
 * - 全局上下文
 * 
 * @}
 */

/**
 * @defgroup InputModule 输入模块
 * @ingroup MediaFramework
 * @{
 * 
 * @brief 输入源管理接口
 * 
 * 输入模块负责从各种来源获取音视频数据：
 * - 文件读取（支持多种格式）
 * - 设备捕获（摄像头、麦克风）
 * - 网络流接收
 * 
 * @}
 */

/**
 * @defgroup ProcessingModule 处理模块
 * @ingroup MediaFramework
 * @{
 * 
 * @brief 音视频处理接口
 * 
 * 处理模块实现各种音视频处理功能：
 * - 格式转换
 * - 剪辑与拼接
 * - 裁剪与缩放
 * - 滤镜与特效
 * - 高级处理
 * 
 * @}
 */

/**
 * @defgroup OutputModule 输出模块
 * @ingroup MediaFramework
 * @{
 * 
 * @brief 输出目标管理接口
 * 
 * 输出模块负责将处理后的数据输出到各种目标：
 * - 文件写入（支持多种格式）
 * - 流媒体推送（RTMP、RTSP等）
 * 
 * @}
 */

/**
 * @defgroup UtilsModule 工具模块
 * @ingroup MediaFramework
 * @{
 * 
 * @brief 工具函数接口
 * 
 * 工具模块提供各种辅助功能：
 * - 日志系统
 * - 内存管理
 * - 字符串处理
 * - 平台检测
 * 
 * @}
 */

#endif /* MEDIA_FRAMEWORK_H */
