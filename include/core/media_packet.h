/**
 * @file media_packet.h
 * @brief 媒体包管理接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了媒体包的创建、销毁和操作接口。
 * MediaPacket用于存储编码后的数据，在输入输出模块间传递。
 */

#ifndef MEDIA_PACKET_H
#define MEDIA_PACKET_H

#include "media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 包创建与销毁
 * ============================================================================ */

/**
 * @brief 创建空包
 * @return 包指针，失败返回NULL
 */
MediaPacket* media_packet_create(void);

/**
 * @brief 创建指定大小的包
 * @param size 数据大小
 * @return 包指针，失败返回NULL
 */
MediaPacket* media_packet_create_with_size(int32_t size);

/**
 * @brief 创建包并复制数据
 * @param data 数据指针
 * @param size 数据大小
 * @return 包指针，失败返回NULL
 */
MediaPacket* media_packet_create_with_data(const uint8_t *data, int32_t size);

/**
 * @brief 引用包（增加引用计数）
 * @param packet 包指针
 * @return 新的包引用，失败返回NULL
 */
MediaPacket* media_packet_ref(MediaPacket *packet);

/**
 * @brief 释放包引用（减少引用计数）
 * @param packet 包指针
 */
void media_packet_unref(MediaPacket *packet);

/**
 * @brief 释放包（释放所有引用）
 * @param packet 包指针
 */
void media_packet_free(MediaPacket **packet);

/**
 * @brief 复制包属性（不复制数据）
 * @param dst 目标包
 * @param src 源包
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_packet_copy_props(MediaPacket *dst, const MediaPacket *src);

/**
 * @brief 克隆包（完整复制）
 * @param packet 源包
 * @return 新包指针，失败返回NULL
 */
MediaPacket* media_packet_clone(const MediaPacket *packet);

/* ============================================================================
 * 包数据操作
 * ============================================================================ */

/**
 * @brief 获取包数据指针
 * @param packet 包指针
 * @return 数据指针
 */
uint8_t* media_packet_get_data(MediaPacket *packet);

/**
 * @brief 获取包数据大小
 * @param packet 包指针
 * @return 数据大小（字节）
 */
int32_t media_packet_get_size(const MediaPacket *packet);

/**
 * @brief 设置包数据
 * @param packet 包指针
 * @param data 数据指针
 * @param size 数据大小
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_packet_set_data(MediaPacket *packet, 
                                      const uint8_t *data, int32_t size);

/**
 * @brief 追加数据到包
 * @param packet 包指针
 * @param data 数据指针
 * @param size 数据大小
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_packet_append_data(MediaPacket *packet, 
                                         const uint8_t *data, int32_t size);

/**
 * @brief 调整包数据大小
 * @param packet 包指针
 * @param new_size 新大小
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_packet_resize(MediaPacket *packet, int32_t new_size);

/**
 * @brief 清空包数据
 * @param packet 包指针
 */
void media_packet_clear_data(MediaPacket *packet);

/* ============================================================================
 * 包属性设置
 * ============================================================================ */

/**
 * @brief 设置包时间戳
 * @param packet 包指针
 * @param pts 显示时间戳
 * @param dts 解码时间戳
 */
void media_packet_set_timestamp(MediaPacket *packet, int64_t pts, int64_t dts);

/**
 * @brief 设置包持续时间
 * @param packet 包指针
 * @param duration 持续时间
 */
void media_packet_set_duration(MediaPacket *packet, int64_t duration);

/**
 * @brief 设置包流索引
 * @param packet 包指针
 * @param stream_index 流索引
 */
void media_packet_set_stream_index(MediaPacket *packet, int32_t stream_index);

/**
 * @brief 设置包为关键帧
 * @param packet 包指针
 * @param key_frame 是否为关键帧
 */
void media_packet_set_key_frame(MediaPacket *packet, int32_t key_frame);

/**
 * @brief 设置包文件位置
 * @param packet 包指针
 * @param pos 文件位置
 */
void media_packet_set_pos(MediaPacket *packet, int64_t pos);

/* ============================================================================
 * 包属性获取
 * ============================================================================ */

/**
 * @brief 获取包显示时间戳
 * @param packet 包指针
 * @return 显示时间戳
 */
int64_t media_packet_get_pts(const MediaPacket *packet);

/**
 * @brief 获取包解码时间戳
 * @param packet 包指针
 * @return 解码时间戳
 */
int64_t media_packet_get_dts(const MediaPacket *packet);

/**
 * @brief 获取包持续时间
 * @param packet 包指针
 * @return 持续时间
 */
int64_t media_packet_get_duration(const MediaPacket *packet);

/**
 * @brief 获取包流索引
 * @param packet 包指针
 * @return 流索引
 */
int32_t media_packet_get_stream_index(const MediaPacket *packet);

/**
 * @brief 判断包是否为关键帧
 * @param packet 包指针
 * @return true表示是关键帧
 */
bool media_packet_is_key_frame(const MediaPacket *packet);

/**
 * @brief 获取包文件位置
 * @param packet 包指针
 * @return 文件位置
 */
int64_t media_packet_get_pos(const MediaPacket *packet);

/**
 * @brief 获取包时间戳（秒）
 * @param packet 包指针
 * @param time_base 时间基
 * @return 时间戳（秒）
 */
double media_packet_get_timestamp_seconds(const MediaPacket *packet, 
                                           MediaRational time_base);

/* ============================================================================
 * 包调试
 * ============================================================================ */

/**
 * @brief 打印包信息
 * @param packet 包指针
 * @param log_level 日志级别
 */
void media_packet_dump(const MediaPacket *packet, int32_t log_level);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_PACKET_H */
