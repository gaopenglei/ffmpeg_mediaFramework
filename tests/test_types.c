/**
 * @file test_types.c
 * @brief 基础类型测试
 */

#include <stdio.h>
#include <assert.h>
#include "core/media_types.h"

void test_rational(void)
{
    printf("Testing rational numbers...\n");
    
    /* 创建有理数 */
    MediaRational r1 = media_rational_make(1, 2);
    assert(r1.num == 1);
    assert(r1.den == 2);
    
    /* 简化 */
    MediaRational r2 = media_rational_make(4, 8);
    r2 = media_rational_simplify(r2);
    assert(r2.num == 1);
    assert(r2.den == 2);
    
    /* 加法 */
    MediaRational r3 = media_rational_make(1, 3);
    MediaRational sum = media_rational_add(r1, r3);
    assert(sum.num == 5);
    assert(sum.den == 6);
    
    /* 乘法 */
    MediaRational prod = media_rational_mul(r1, r3);
    assert(prod.num == 1);
    assert(prod.den == 6);
    
    /* 转换为浮点数 */
    double d = media_rational_to_double(r1);
    assert(d == 0.5);
    
    printf("  Rational tests passed!\n");
}

void test_rect(void)
{
    printf("Testing rectangles...\n");
    
    MediaRect rect = media_rect_make(10, 20, 100, 200);
    assert(rect.x == 10);
    assert(rect.y == 20);
    assert(rect.width == 100);
    assert(rect.height == 200);
    
    printf("  Rectangle tests passed!\n");
}

void test_format_names(void)
{
    printf("Testing format names...\n");
    
    /* 像素格式 */
    const char *yuv420p = media_pixel_format_name(MEDIA_PIXEL_FORMAT_YUV420P);
    assert(yuv420p != NULL);
    
    /* 采样格式 */
    const char *s16 = media_sample_format_name(MEDIA_SAMPLE_FORMAT_S16);
    assert(s16 != NULL);
    
    /* 编码器 */
    const char *h264 = media_codec_name(MEDIA_CODEC_ID_H264);
    assert(h264 != NULL);
    
    /* 容器格式 */
    const char *mp4 = media_container_name(MEDIA_CONTAINER_MP4);
    assert(mp4 != NULL);
    
    /* 协议 */
    const char *rtmp = media_protocol_name(MEDIA_PROTOCOL_RTMP);
    assert(rtmp != NULL);
    
    printf("  Format name tests passed!\n");
}

void test_codec_id_from_name(void)
{
    printf("Testing codec ID from name...\n");
    
    MediaCodecID id;
    
    id = media_codec_id_from_name("h264");
    assert(id == MEDIA_CODEC_ID_H264);
    
    id = media_codec_id_from_name("H265");
    assert(id == MEDIA_CODEC_ID_H265);
    
    id = media_codec_id_from_name("aac");
    assert(id == MEDIA_CODEC_ID_AAC);
    
    id = media_codec_id_from_name("mp3");
    assert(id == MEDIA_CODEC_ID_MP3);
    
    id = media_codec_id_from_name("unknown");
    assert(id == MEDIA_CODEC_ID_NONE);
    
    printf("  Codec ID tests passed!\n");
}

void test_guess_container(void)
{
    printf("Testing container format guessing...\n");
    
    MediaContainerFormat fmt;
    
    fmt = media_guess_container_format("video.mp4");
    assert(fmt == MEDIA_CONTAINER_MP4);
    
    fmt = media_guess_container_format("video.mkv");
    assert(fmt == MEDIA_CONTAINER_MKV);
    
    fmt = media_guess_container_format("video.avi");
    assert(fmt == MEDIA_CONTAINER_AVI);
    
    fmt = media_guess_container_format("audio.mp3");
    assert(fmt == MEDIA_CONTAINER_MP3);
    
    fmt = media_guess_container_format("audio.wav");
    assert(fmt == MEDIA_CONTAINER_WAV);
    
    printf("  Container format guessing tests passed!\n");
}

int main(void)
{
    printf("=== Media Types Tests ===\n\n");
    
    test_rational();
    test_rect();
    test_format_names();
    test_codec_id_from_name();
    test_guess_container();
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
