/**
 * @file media_frame.h
 * @brief 媒体帧管理接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了媒体帧的创建、销毁和操作接口。
 * MediaFrame是处理模块中数据流转的核心数据结构。
 */

#ifndef MEDIA_FRAME_H
#define MEDIA_FRAME_H

#include "media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 帧创建与销毁
 * ============================================================================ */

/**
 * @brief 创建视频帧
 * @param width 视频宽度
 * @param height 视频高度
 * @param format 像素格式
 * @return 帧指针，失败返回NULL
 */
MediaFrame* media_frame_create_video(int32_t width, int32_t height, 
                                      MediaPixelFormat format);

/**
 * @brief 创建音频帧
 * @param nb_samples 采样数
 * @param format 采样格式
 * @param channels 声道数
 * @param sample_rate 采样率
 * @return 帧指针，失败返回NULL
 */
MediaFrame* media_frame_create_audio(int32_t nb_samples, 
                                      MediaSampleFormat format,
                                      int32_t channels, 
                                      int32_t sample_rate);

/**
 * @brief 创建空帧
 * @return 帧指针，失败返回NULL
 */
MediaFrame* media_frame_create(void);

/**
 * @brief 引用帧（增加引用计数）
 * @param frame 帧指针
 * @return 新的帧引用，失败返回NULL
 */
MediaFrame* media_frame_ref(MediaFrame *frame);

/**
 * @brief 释放帧引用（减少引用计数）
 * @param frame 帧指针
 */
void media_frame_unref(MediaFrame *frame);

/**
 * @brief 释放帧（释放所有引用）
 * @param frame 帧指针
 */
void media_frame_free(MediaFrame **frame);

/**
 * @brief 复制帧属性（不复制数据）
 * @param dst 目标帧
 * @param src 源帧
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_copy_props(MediaFrame *dst, const MediaFrame *src);

/**
 * @brief 复制帧数据
 * @param dst 目标帧
 * @param src 源帧
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_copy(MediaFrame *dst, const MediaFrame *src);

/**
 * @brief 克隆帧（完整复制）
 * @param frame 源帧
 * @return 新帧指针，失败返回NULL
 */
MediaFrame* media_frame_clone(const MediaFrame *frame);

/* ============================================================================
 * 帧数据访问
 * ============================================================================ */

/**
 * @brief 获取帧数据指针数组
 * @param frame 帧指针
 * @return 数据指针数组
 */
uint8_t** media_frame_get_data(MediaFrame *frame);

/**
 * @brief 获取帧数据行大小数组
 * @param frame 帧指针
 * @return 行大小数组
 */
int* media_frame_get_linesize(MediaFrame *frame);

/**
 * @brief 获取帧数据总大小
 * @param frame 帧指针
 * @return 总大小（字节）
 */
int32_t media_frame_get_data_size(const MediaFrame *frame);

/**
 * @brief 使帧数据可写
 * @param frame 帧指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_make_writable(MediaFrame *frame);

/* ============================================================================
 * 帧属性设置
 * ============================================================================ */

/**
 * @brief 设置帧PTS
 * @param frame 帧指针
 * @param pts 显示时间戳
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_set_pts(MediaFrame *frame, int64_t pts);

/**
 * @brief 设置帧DTS
 * @param frame 帧指针
 * @param dts 解码时间戳
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_set_dts(MediaFrame *frame, int64_t dts);

/**
 * @brief 设置帧持续时间
 * @param frame 帧指针
 * @param duration 持续时间
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_set_duration(MediaFrame *frame, int64_t duration);

/**
 * @brief 设置帧为关键帧
 * @param frame 帧指针
 * @param is_key 是否为关键帧
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_set_key_frame(MediaFrame *frame, bool is_key);

/**
 * @brief 设置帧为隔行扫描
 * @param frame 帧指针
 * @param interlaced 是否为隔行扫描
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_set_interlaced(MediaFrame *frame, bool interlaced);

/* ============================================================================
 * 帧信息获取
 * ============================================================================ */

/**
 * @brief 获取帧PTS
 * @param frame 帧指针
 * @return PTS值
 */
int64_t media_frame_get_pts(const MediaFrame *frame);

/**
 * @brief 获取帧DTS
 * @param frame 帧指针
 * @return DTS值
 */
int64_t media_frame_get_dts(const MediaFrame *frame);

/**
 * @brief 获取帧持续时间
 * @param frame 帧指针
 * @return 持续时间
 */
int64_t media_frame_get_duration(const MediaFrame *frame);

/**
 * @brief 获取帧宽度
 * @param frame 帧指针
 * @return 宽度（像素）
 */
int32_t media_frame_get_width(const MediaFrame *frame);

/**
 * @brief 获取帧高度
 * @param frame 帧指针
 * @return 高度（像素）
 */
int32_t media_frame_get_height(const MediaFrame *frame);

/**
 * @brief 获取帧像素格式
 * @param frame 帧指针
 * @return 像素格式
 */
MediaPixelFormat media_frame_get_pixel_format(const MediaFrame *frame);

/**
 * @brief 获取帧采样率
 * @param frame 帧指针
 * @return 采样率
 */
int32_t media_frame_get_sample_rate(const MediaFrame *frame);

/**
 * @brief 获取帧声道数
 * @param frame 帧指针
 * @return 声道数
 */
int32_t media_frame_get_channels(const MediaFrame *frame);

/**
 * @brief 获取帧采样数
 * @param frame 帧指针
 * @return 采样数
 */
int32_t media_frame_get_nb_samples(const MediaFrame *frame);

/**
 * @brief 获取帧采样格式
 * @param frame 帧指针
 * @return 采样格式
 */
MediaSampleFormat media_frame_get_sample_format(const MediaFrame *frame);

/**
 * @brief 获取帧类型
 * @param frame 帧指针
 * @return 媒体类型
 */
MediaStreamType media_frame_get_type(const MediaFrame *frame);

/**
 * @brief 判断帧是否为关键帧
 * @param frame 帧指针
 * @return true表示是关键帧
 */
bool media_frame_is_key_frame(const MediaFrame *frame);

/* ============================================================================
 * 帧数据操作
 * ============================================================================ */

/**
 * @brief 裁剪帧数据
 * @param frame 帧指针
 * @param rect 裁剪区域
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_crop(MediaFrame *frame, MediaRect rect);

/**
 * @brief 缩放帧数据
 * @param frame 帧指针（会被修改）
 * @param new_width 新宽度
 * @param new_height 新高度
 * @param algorithm 缩放算法
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_scale(MediaFrame *frame, 
                                  int32_t new_width, int32_t new_height,
                                  MediaScaleAlgorithm algorithm);

/**
 * @brief 转换帧像素格式
 * @param frame 帧指针（会被修改）
 * @param dst_format 目标像素格式
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_convert_pixel_format(MediaFrame *frame, 
                                                 MediaPixelFormat dst_format);

/**
 * @brief 翻转帧数据
 * @param frame 帧指针
 * @param direction 翻转方向
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_flip(MediaFrame *frame, MediaFlipDirection direction);

/**
 * @brief 旋转帧数据
 * @param frame 帧指针
 * @param rotation 旋转角度
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_frame_rotate(MediaFrame *frame, MediaRotation rotation);

/* ============================================================================
 * 帧调试
 * ============================================================================ */

/**
 * @brief 打印帧信息
 * @param frame 帧指针
 * @param log_level 日志级别（0=quiet, 1=error, 2=warning, 3=info, 4=debug）
 */
void media_frame_dump(const MediaFrame *frame, int32_t log_level);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_FRAME_H */
