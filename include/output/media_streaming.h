/**
 * @file media_streaming.h
 * @brief 流媒体处理接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了流媒体推流和拉流的接口。
 * 支持RTMP、RTSP、HTTP-FLV、HLS等协议。
 */

#ifndef MEDIA_STREAMING_H
#define MEDIA_STREAMING_H

#include "core/media_types.h"
#include "core/media_frame.h"
#include "core/media_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 流媒体服务器结构体（不透明指针）
 */
typedef struct MediaStreamer MediaStreamer;

/**
 * @brief 流媒体状态
 */
typedef enum {
    MEDIA_STREAMER_STATE_IDLE = 0,
    MEDIA_STREAMER_STATE_CONNECTING = 1,
    MEDIA_STREAMER_STATE_CONNECTED = 2,
    MEDIA_STREAMER_STATE_STREAMING = 3,
    MEDIA_STREAMER_STATE_RECONNECTING = 4,
    MEDIA_STREAMER_STATE_ERROR = 5,
    MEDIA_STREAMER_STATE_CLOSED = 6,
} MediaStreamerState;

/**
 * @brief 流媒体方向
 */
typedef enum {
    MEDIA_STREAM_DIRECTION_PUSH = 1,    ///< 推流
    MEDIA_STREAM_DIRECTION_PULL = 2,    ///< 拉流
    MEDIA_STREAM_DIRECTION_DUPLEX = 3,  ///< 双向
} MediaStreamDirection;

/**
 * @brief 流媒体配置
 */
typedef struct {
    MediaProtocol protocol;         ///< 协议类型
    MediaStreamDirection direction; ///< 方向
    char url[1024];                 ///< 服务器URL
    int32_t reconnect;              ///< 自动重连
    int32_t reconnect_interval;     ///< 重连间隔（毫秒）
    int32_t reconnect_count;        ///< 最大重连次数
    int32_t timeout;                ///< 超时时间（毫秒）
    int32_t buffer_size;            ///< 缓冲区大小
    int32_t low_latency;            ///< 低延迟模式
    int32_t realtime;               ///< 实时模式
    void *user_data;                ///< 用户数据
} MediaStreamerConfig;

/**
 * @brief HLS配置
 */
typedef struct {
    int32_t segment_duration;       ///< 分片时长（秒）
    int32_t segment_count;          ///< 分片数量
    int32_t delete_segments;        ///< 删除旧分片
    int32_t hls_version;            ///< HLS版本
    int32_t allow_cache;            ///< 允许缓存
    char playlist_name[256];        ///< 播放列表名称
    char segment_name[256];         ///< 分片名称模板
    char segment_dir[256];          ///< 分片目录
} HLSConfig;

/**
 * @brief RTMP配置
 */
typedef struct {
    char app[256];                  ///< 应用名称
    char stream_key[256];           ///< 流密钥
    char tc_url[1024];              ///< TC URL
    char page_url[1024];            ///< 页面URL
    char flash_ver[256];            ///< Flash版本
    int32_t publish;                ///< 发布模式
    int32_t live;                   ///< 直播模式
    int32_t buffer_time;            ///< 缓冲时间（毫秒）
} RTMPConfig;

/**
 * @brief RTSP配置
 */
typedef struct {
    char user[256];                 ///< 用户名
    char password[256];             ///< 密码
    int32_t port;                   ///< 端口
    int32_t rtsp_transport;         ///< 传输协议 (0=udp, 1=tcp, 2=auto)
    int32_t multicast;              ///< 组播模式
    int32_t buffer_size;            ///< 缓冲区大小
} RTSPConfig;

/**
 * @brief 流状态回调
 * @param streamer 流媒体器指针
 * @param state 新状态
 * @param user_data 用户数据
 */
typedef void (*MediaStreamStateCallback)(MediaStreamer *streamer, 
                                          MediaStreamerState state, 
                                          void *user_data);

/**
 * @brief 数据包回调
 * @param streamer 流媒体器指针
 * @param packet 数据包
 * @param user_data 用户数据
 */
typedef void (*MediaStreamPacketCallback)(MediaStreamer *streamer, 
                                           const MediaPacket *packet, 
                                           void *user_data);

/**
 * @brief 帧回调
 * @param streamer 流媒体器指针
 * @param frame 帧
 * @param user_data 用户数据
 */
typedef void (*MediaStreamFrameCallback)(MediaStreamer *streamer, 
                                          const MediaFrame *frame, 
                                          void *user_data);

/* ============================================================================
 * 流媒体器创建与销毁
 * ============================================================================ */

/**
 * @brief 创建流媒体器
 * @return 流媒体器指针，失败返回NULL
 */
MediaStreamer* media_streamer_create(void);

/**
 * @brief 创建带配置的流媒体器
 * @param config 配置结构体
 * @return 流媒体器指针，失败返回NULL
 */
MediaStreamer* media_streamer_create_with_config(const MediaStreamerConfig *config);

/**
 * @brief 释放流媒体器
 * @param streamer 流媒体器指针
 */
void media_streamer_free(MediaStreamer *streamer);

/**
 * @brief 重置配置为默认值
 * @param config 配置结构体指针
 */
void media_streamer_config_default(MediaStreamerConfig *config);

/* ============================================================================
 * 连接管理
 * ============================================================================ */

/**
 * @brief 连接服务器
 * @param streamer 流媒体器指针
 * @param url 服务器URL
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_connect(MediaStreamer *streamer, const char *url);

/**
 * @brief 断开连接
 * @param streamer 流媒体器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_disconnect(MediaStreamer *streamer);

/**
 * @brief 检查是否已连接
 * @param streamer 流媒体器指针
 * @return true表示已连接
 */
bool media_streamer_is_connected(MediaStreamer *streamer);

/**
 * @brief 获取状态
 * @param streamer 流媒体器指针
 * @return 状态值
 */
MediaStreamerState media_streamer_get_state(MediaStreamer *streamer);

/**
 * @brief 获取连接URL
 * @param streamer 流媒体器指针
 * @return URL字符串
 */
const char* media_streamer_get_url(MediaStreamer *streamer);

/* ============================================================================
 * 推流功能
 * ============================================================================ */

/**
 * @brief 开始推流
 * @param streamer 流媒体器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_start_push(MediaStreamer *streamer);

/**
 * @brief 停止推流
 * @param streamer 流媒体器指针
 */
void media_streamer_stop_push(MediaStreamer *streamer);

/**
 * @brief 推送数据包
 * @param streamer 流媒体器指针
 * @param packet 数据包
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_push_packet(MediaStreamer *streamer, 
                                           const MediaPacket *packet);

/**
 * @brief 推送帧
 * @param streamer 流媒体器指针
 * @param frame 帧
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_push_frame(MediaStreamer *streamer, 
                                          const MediaFrame *frame, 
                                          int32_t stream_index);

/**
 * @brief 编码并推送帧
 * @param streamer 流媒体器指针
 * @param frame 帧
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_encode_and_push(MediaStreamer *streamer, 
                                               const MediaFrame *frame, 
                                               int32_t stream_index);

/**
 * @brief 设置推流视频参数
 * @param streamer 流媒体器指针
 * @param params 视频参数
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_set_push_video_params(MediaStreamer *streamer, 
                                                     const VideoParams *params);

/**
 * @brief 设置推流音频参数
 * @param streamer 流媒体器指针
 * @param params 音频参数
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_set_push_audio_params(MediaStreamer *streamer, 
                                                     const AudioParams *params);

/* ============================================================================
 * 拉流功能
 * ============================================================================ */

/**
 * @brief 开始拉流
 * @param streamer 流媒体器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_start_pull(MediaStreamer *streamer);

/**
 * @brief 停止拉流
 * @param streamer 流媒体器指针
 */
void media_streamer_stop_pull(MediaStreamer *streamer);

/**
 * @brief 拉取数据包
 * @param streamer 流媒体器指针
 * @param packet 数据包（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_pull_packet(MediaStreamer *streamer, 
                                           MediaPacket *packet);

/**
 * @brief 拉取帧
 * @param streamer 流媒体器指针
 * @param frame 帧（输出）
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_pull_frame(MediaStreamer *streamer, 
                                          MediaFrame *frame, 
                                          int32_t stream_index);

/**
 * @brief 解码并拉取帧
 * @param streamer 流媒体器指针
 * @param frame 帧（输出）
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_decode_and_pull(MediaStreamer *streamer, 
                                               MediaFrame *frame, 
                                               int32_t stream_index);

/**
 * @brief 获取拉流视频参数
 * @param streamer 流媒体器指针
 * @param params 视频参数（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_get_pull_video_params(MediaStreamer *streamer, 
                                                     VideoParams *params);

/**
 * @brief 获取拉流音频参数
 * @param streamer 流媒体器指针
 * @param params 音频参数（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_get_pull_audio_params(MediaStreamer *streamer, 
                                                     AudioParams *params);

/* ============================================================================
 * 回调设置
 * ============================================================================ */

/**
 * @brief 设置状态回调
 * @param streamer 流媒体器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_streamer_set_state_callback(MediaStreamer *streamer, 
                                        MediaStreamStateCallback callback, 
                                        void *user_data);

/**
 * @brief 设置数据包回调
 * @param streamer 流媒体器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_streamer_set_packet_callback(MediaStreamer *streamer, 
                                         MediaStreamPacketCallback callback, 
                                         void *user_data);

/**
 * @brief 设置帧回调
 * @param streamer 流媒体器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_streamer_set_frame_callback(MediaStreamer *streamer, 
                                        MediaStreamFrameCallback callback, 
                                        void *user_data);

/* ============================================================================
 * 协议特定配置
 * ============================================================================ */

/**
 * @brief 配置HLS
 * @param streamer 流媒体器指针
 * @param config HLS配置
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_configure_hls(MediaStreamer *streamer, 
                                             const HLSConfig *config);

/**
 * @brief 配置RTMP
 * @param streamer 流媒体器指针
 * @param config RTMP配置
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_configure_rtmp(MediaStreamer *streamer, 
                                              const RTMPConfig *config);

/**
 * @brief 配置RTSP
 * @param streamer 流媒体器指针
 * @param config RTSP配置
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_streamer_configure_rtsp(MediaStreamer *streamer, 
                                              const RTSPConfig *config);

/**
 * @brief 重置HLS配置为默认值
 * @param config 配置结构体指针
 */
void media_streamer_hls_config_default(HLSConfig *config);

/**
 * @brief 重置RTMP配置为默认值
 * @param config 配置结构体指针
 */
void media_streamer_rtmp_config_default(RTMPConfig *config);

/**
 * @brief 重置RTSP配置为默认值
 * @param config 配置结构体指针
 */
void media_streamer_rtsp_config_default(RTSPConfig *config);

/* ============================================================================
 * 统计信息
 * ============================================================================ */

/**
 * @brief 获取已发送字节数
 * @param streamer 流媒体器指针
 * @return 字节数
 */
int64_t media_streamer_get_bytes_sent(MediaStreamer *streamer);

/**
 * @brief 获取已接收字节数
 * @param streamer 流媒体器指针
 * @return 字节数
 */
int64_t media_streamer_get_bytes_received(MediaStreamer *streamer);

/**
 * @brief 获取已发送帧数
 * @param streamer 流媒体器指针
 * @return 帧数
 */
int64_t media_streamer_get_frames_sent(MediaStreamer *streamer);

/**
 * @brief 获取已接收帧数
 * @param streamer 流媒体器指针
 * @return 帧数
 */
int64_t media_streamer_get_frames_received(MediaStreamer *streamer);

/**
 * @brief 获取当前比特率
 * @param streamer 流媒体器指针
 * @return 比特率（bps）
 */
int32_t media_streamer_get_bitrate(MediaStreamer *streamer);

/**
 * @brief 获取延迟
 * @param streamer 流媒体器指针
 * @return 延迟（毫秒）
 */
int32_t media_streamer_get_latency(MediaStreamer *streamer);

/**
 * @brief 获取丢包数
 * @param streamer 流媒体器指针
 * @return 丢包数
 */
int64_t media_streamer_get_packets_lost(MediaStreamer *streamer);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_STREAMING_H */
