/**
 * @file media_types.c
 * @brief 媒体框架基础类型实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "core/media_types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * 有理数运算
 * ============================================================================ */

MediaRational media_rational_make(int32_t num, int32_t den)
{
    MediaRational r = {num, den};
    return r;
}

MediaRational media_rational_simplify(MediaRational r)
{
    if (r.num == 0 || r.den == 0) {
        return r;
    }
    
    /* 计算最大公约数 */
    int32_t a = abs(r.num);
    int32_t b = abs(r.den);
    
    while (b != 0) {
        int32_t t = b;
        b = a % b;
        a = t;
    }
    
    if (a > 1) {
        r.num /= a;
        r.den /= a;
    }
    
    /* 确保分母为正 */
    if (r.den < 0) {
        r.num = -r.num;
        r.den = -r.den;
    }
    
    return r;
}

MediaRational media_rational_add(MediaRational a, MediaRational b)
{
    MediaRational result;
    result.num = a.num * b.den + b.num * a.den;
    result.den = a.den * b.den;
    return media_rational_simplify(result);
}

MediaRational media_rational_sub(MediaRational a, MediaRational b)
{
    MediaRational result;
    result.num = a.num * b.den - b.num * a.den;
    result.den = a.den * b.den;
    return media_rational_simplify(result);
}

MediaRational media_rational_mul(MediaRational a, MediaRational b)
{
    MediaRational result;
    result.num = a.num * b.num;
    result.den = a.den * b.den;
    return media_rational_simplify(result);
}

MediaRational media_rational_div(MediaRational a, MediaRational b)
{
    if (b.num == 0) {
        return a; /* 避免除零 */
    }
    
    MediaRational result;
    result.num = a.num * b.den;
    result.den = a.den * b.num;
    return media_rational_simplify(result);
}

int media_rational_compare(MediaRational a, MediaRational b)
{
    int64_t left = (int64_t)a.num * b.den;
    int64_t right = (int64_t)b.num * a.den;
    
    if (left < right) return -1;
    if (left > right) return 1;
    return 0;
}

double media_rational_to_double(MediaRational r)
{
    if (r.den == 0) {
        return 0.0;
    }
    return (double)r.num / (double)r.den;
}

/* ============================================================================
 * 矩形操作
 * ============================================================================ */

MediaRect media_rect_make(int32_t x, int32_t y, int32_t width, int32_t height)
{
    MediaRect rect = {x, y, width, height};
    return rect;
}

/* ============================================================================
 * 格式名称查询
 * ============================================================================ */

const char* media_pixel_format_name(MediaPixelFormat format)
{
    switch (format) {
        case MEDIA_PIX_FMT_NONE:       return "none";
        case MEDIA_PIX_FMT_YUV420P:    return "yuv420p";
        case MEDIA_PIX_FMT_YUV422P:    return "yuv422p";
        case MEDIA_PIX_FMT_YUV444P:    return "yuv444p";
        case MEDIA_PIX_FMT_YUYV422:    return "yuyv422";
        case MEDIA_PIX_FMT_UYVY422:    return "uyvy422";
        case MEDIA_PIX_FMT_NV12:       return "nv12";
        case MEDIA_PIX_FMT_NV21:       return "nv21";
        case MEDIA_PIX_FMT_RGB24:      return "rgb24";
        case MEDIA_PIX_FMT_BGR24:      return "bgr24";
        case MEDIA_PIX_FMT_RGB32:      return "rgb32";
        case MEDIA_PIX_FMT_BGR32:      return "bgr32";
        case MEDIA_PIX_FMT_RGBA:       return "rgba";
        case MEDIA_PIX_FMT_BGRA:       return "bgra";
        case MEDIA_PIX_FMT_GRAY8:      return "gray8";
        case MEDIA_PIX_FMT_GRAY16:     return "gray16";
        case MEDIA_PIX_FMT_YUV410P:    return "yuv410p";
        case MEDIA_PIX_FMT_YUV411P:    return "yuv411p";
        case MEDIA_PIX_FMT_YUV440P:    return "yuv440p";
        case MEDIA_PIX_FMT_YUVJ420P:   return "yuvj420p";
        case MEDIA_PIX_FMT_YUVJ422P:   return "yuvj422p";
        case MEDIA_PIX_FMT_YUVJ444P:   return "yuvj444p";
        default:                       return "unknown";
    }
}

const char* media_sample_format_name(MediaSampleFormat format)
{
    switch (format) {
        case MEDIA_SAMPLE_FMT_NONE:    return "none";
        case MEDIA_SAMPLE_FMT_U8:      return "u8";
        case MEDIA_SAMPLE_FMT_S16:     return "s16";
        case MEDIA_SAMPLE_FMT_S32:     return "s32";
        case MEDIA_SAMPLE_FMT_S64:     return "s64";
        case MEDIA_SAMPLE_FMT_FLT:     return "flt";
        case MEDIA_SAMPLE_FMT_DBL:     return "dbl";
        case MEDIA_SAMPLE_FMT_U8P:     return "u8p";
        case MEDIA_SAMPLE_FMT_S16P:    return "s16p";
        case MEDIA_SAMPLE_FMT_S32P:    return "s32p";
        case MEDIA_SAMPLE_FMT_S64P:    return "s64p";
        case MEDIA_SAMPLE_FMT_FLTP:    return "fltp";
        case MEDIA_SAMPLE_FMT_DBLP:    return "dblp";
        default:                       return "unknown";
    }
}

const char* media_codec_name(MediaCodecID codec_id)
{
    switch (codec_id) {
        /* 视频编码器 */
        case MEDIA_CODEC_ID_H264:      return "h264";
        case MEDIA_CODEC_ID_H265:      return "hevc";
        case MEDIA_CODEC_ID_VP8:       return "vp8";
        case MEDIA_CODEC_ID_VP9:       return "vp9";
        case MEDIA_CODEC_ID_AV1:       return "av1";
        case MEDIA_CODEC_ID_MPEG2:     return "mpeg2video";
        case MEDIA_CODEC_ID_MPEG4:     return "mpeg4";
        case MEDIA_CODEC_ID_MJPEG:     return "mjpeg";
        case MEDIA_CODEC_ID_THEORA:    return "theora";
        
        /* 音频编码器 */
        case MEDIA_CODEC_ID_AAC:       return "aac";
        case MEDIA_CODEC_ID_MP3:       return "mp3";
        case MEDIA_CODEC_ID_OPUS:      return "opus";
        case MEDIA_CODEC_ID_VORBIS:    return "vorbis";
        case MEDIA_CODEC_ID_FLAC:      return "flac";
        case MEDIA_CODEC_ID_WAV:       return "pcm_s16le";
        case MEDIA_CODEC_ID_AC3:       return "ac3";
        case MEDIA_CODEC_ID_EAC3:      return "eac3";
        case MEDIA_CODEC_ID_AMR_NB:    return "amr_nb";
        case MEDIA_CODEC_ID_AMR_WB:    return "amr_wb";
        case MEDIA_CODEC_ID_SPEEX:     return "speex";
        
        default:                       return "unknown";
    }
}

const char* media_container_name(MediaContainerFormat container)
{
    switch (container) {
        case MEDIA_CONTAINER_NONE:     return "none";
        case MEDIA_CONTAINER_MP4:      return "mp4";
        case MEDIA_CONTAINER_AVI:      return "avi";
        case MEDIA_CONTAINER_MKV:      return "matroska";
        case MEDIA_CONTAINER_MOV:      return "mov";
        case MEDIA_CONTAINER_FLV:      return "flv";
        case MEDIA_CONTAINER_WEBM:     return "webm";
        case MEDIA_CONTAINER_TS:       return "mpegts";
        case MEDIA_CONTAINER_MP3:      return "mp3";
        case MEDIA_CONTAINER_AAC:      return "adts";
        case MEDIA_CONTAINER_WAV:      return "wav";
        case MEDIA_CONTAINER_FLAC:     return "flac";
        case MEDIA_CONTAINER_OGG:      return "ogg";
        case MEDIA_CONTAINER_M4A:      return "ipod";
        case MEDIA_CONTAINER_3GP:      return "3gp";
        case MEDIA_CONTAINER_MXF:      return "mxf";
        default:                       return "unknown";
    }
}

const char* media_protocol_name(MediaProtocol protocol)
{
    switch (protocol) {
        case MEDIA_PROTOCOL_NONE:      return "none";
        case MEDIA_PROTOCOL_RTMP:      return "rtmp";
        case MEDIA_PROTOCOL_RTSP:      return "rtsp";
        case MEDIA_PROTOCOL_HTTP:      return "http";
        case MEDIA_PROTOCOL_HTTPS:     return "https";
        case MEDIA_PROTOCOL_HLS:       return "hls";
        case MEDIA_PROTOCOL_UDP:       return "udp";
        case MEDIA_PROTOCOL_TCP:       return "tcp";
        case MEDIA_PROTOCOL_RTP:       return "rtp";
        case MEDIA_PROTOCOL_SRT:       return "srt";
        case MEDIA_PROTOCOL_FILE:      return "file";
        default:                       return "unknown";
    }
}

/* ============================================================================
 * 编码器类型判断
 * ============================================================================ */

bool media_codec_is_video(MediaCodecID codec_id)
{
    switch (codec_id) {
        case MEDIA_CODEC_ID_H264:
        case MEDIA_CODEC_ID_H265:
        case MEDIA_CODEC_ID_VP8:
        case MEDIA_CODEC_ID_VP9:
        case MEDIA_CODEC_ID_AV1:
        case MEDIA_CODEC_ID_MPEG2:
        case MEDIA_CODEC_ID_MPEG4:
        case MEDIA_CODEC_ID_MJPEG:
        case MEDIA_CODEC_ID_THEORA:
            return true;
        default:
            return false;
    }
}

bool media_codec_is_audio(MediaCodecID codec_id)
{
    switch (codec_id) {
        case MEDIA_CODEC_ID_AAC:
        case MEDIA_CODEC_ID_MP3:
        case MEDIA_CODEC_ID_OPUS:
        case MEDIA_CODEC_ID_VORBIS:
        case MEDIA_CODEC_ID_FLAC:
        case MEDIA_CODEC_ID_WAV:
        case MEDIA_CODEC_ID_AC3:
        case MEDIA_CODEC_ID_EAC3:
        case MEDIA_CODEC_ID_AMR_NB:
        case MEDIA_CODEC_ID_AMR_WB:
        case MEDIA_CODEC_ID_SPEEX:
            return true;
        default:
            return false;
    }
}

/* ============================================================================
 * 格式猜测
 * ============================================================================ */

MediaContainerFormat media_guess_container_format(const char *filename)
{
    if (!filename) {
        return MEDIA_CONTAINER_NONE;
    }
    
    /* 查找扩展名 */
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return MEDIA_CONTAINER_NONE;
    }
    ext++; /* 跳过点号 */
    
    /* 根据扩展名判断 */
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "m4v") == 0) {
        return MEDIA_CONTAINER_MP4;
    }
    if (strcasecmp(ext, "avi") == 0) {
        return MEDIA_CONTAINER_AVI;
    }
    if (strcasecmp(ext, "mkv") == 0) {
        return MEDIA_CONTAINER_MKV;
    }
    if (strcasecmp(ext, "mov") == 0 || strcasecmp(ext, "qt") == 0) {
        return MEDIA_CONTAINER_MOV;
    }
    if (strcasecmp(ext, "flv") == 0) {
        return MEDIA_CONTAINER_FLV;
    }
    if (strcasecmp(ext, "webm") == 0) {
        return MEDIA_CONTAINER_WEBM;
    }
    if (strcasecmp(ext, "ts") == 0 || strcasecmp(ext, "m2ts") == 0) {
        return MEDIA_CONTAINER_TS;
    }
    if (strcasecmp(ext, "mp3") == 0) {
        return MEDIA_CONTAINER_MP3;
    }
    if (strcasecmp(ext, "aac") == 0 || strcasecmp(ext, "adts") == 0) {
        return MEDIA_CONTAINER_AAC;
    }
    if (strcasecmp(ext, "wav") == 0) {
        return MEDIA_CONTAINER_WAV;
    }
    if (strcasecmp(ext, "flac") == 0) {
        return MEDIA_CONTAINER_FLAC;
    }
    if (strcasecmp(ext, "ogg") == 0 || strcasecmp(ext, "oga") == 0) {
        return MEDIA_CONTAINER_OGG;
    }
    if (strcasecmp(ext, "m4a") == 0) {
        return MEDIA_CONTAINER_M4A;
    }
    if (strcasecmp(ext, "3gp") == 0 || strcasecmp(ext, "3g2") == 0) {
        return MEDIA_CONTAINER_3GP;
    }
    if (strcasecmp(ext, "mxf") == 0) {
        return MEDIA_CONTAINER_MXF;
    }
    
    return MEDIA_CONTAINER_NONE;
}

MediaCodecID media_codec_id_from_name(const char *name)
{
    if (!name) {
        return MEDIA_CODEC_ID_NONE;
    }
    
    /* 视频编码器 */
    if (strcasecmp(name, "h264") == 0 || strcasecmp(name, "libx264") == 0) {
        return MEDIA_CODEC_ID_H264;
    }
    if (strcasecmp(name, "hevc") == 0 || strcasecmp(name, "h265") == 0 || 
        strcasecmp(name, "libx265") == 0) {
        return MEDIA_CODEC_ID_H265;
    }
    if (strcasecmp(name, "vp8") == 0 || strcasecmp(name, "libvpx") == 0) {
        return MEDIA_CODEC_ID_VP8;
    }
    if (strcasecmp(name, "vp9") == 0 || strcasecmp(name, "libvpx-vp9") == 0) {
        return MEDIA_CODEC_ID_VP9;
    }
    if (strcasecmp(name, "av1") == 0 || strcasecmp(name, "libaom-av1") == 0) {
        return MEDIA_CODEC_ID_AV1;
    }
    if (strcasecmp(name, "mpeg2") == 0 || strcasecmp(name, "mpeg2video") == 0) {
        return MEDIA_CODEC_ID_MPEG2;
    }
    if (strcasecmp(name, "mpeg4") == 0) {
        return MEDIA_CODEC_ID_MPEG4;
    }
    if (strcasecmp(name, "mjpeg") == 0) {
        return MEDIA_CODEC_ID_MJPEG;
    }
    
    /* 音频编码器 */
    if (strcasecmp(name, "aac") == 0) {
        return MEDIA_CODEC_ID_AAC;
    }
    if (strcasecmp(name, "mp3") == 0 || strcasecmp(name, "libmp3lame") == 0) {
        return MEDIA_CODEC_ID_MP3;
    }
    if (strcasecmp(name, "opus") == 0 || strcasecmp(name, "libopus") == 0) {
        return MEDIA_CODEC_ID_OPUS;
    }
    if (strcasecmp(name, "vorbis") == 0 || strcasecmp(name, "libvorbis") == 0) {
        return MEDIA_CODEC_ID_VORBIS;
    }
    if (strcasecmp(name, "flac") == 0) {
        return MEDIA_CODEC_ID_FLAC;
    }
    if (strcasecmp(name, "wav") == 0 || strcasecmp(name, "pcm_s16le") == 0) {
        return MEDIA_CODEC_ID_WAV;
    }
    if (strcasecmp(name, "ac3") == 0) {
        return MEDIA_CODEC_ID_AC3;
    }
    if (strcasecmp(name, "eac3") == 0) {
        return MEDIA_CODEC_ID_EAC3;
    }
    
    return MEDIA_CODEC_ID_NONE;
}
